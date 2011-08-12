/*  Sirikata
 *  JSObjectScript.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sirikata/oh/Platform.hpp>

#include <sirikata/core/util/KnownServices.hpp>


#include "JSObjectScript.hpp"
#include "EmersonScript.hpp"
#include "JSObjectScriptManager.hpp"
#include "JSLogging.hpp"

#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSVec3.hpp"
#include "JSObjects/JSQuaternion.hpp"
#include "JSObjects/JSInvokableObject.hpp"

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/Random.hpp>

#include <sirikata/core/odp/Defs.hpp>
#include <vector>
#include <set>
#include "JSObjects/JSFields.hpp"
#include "JS_JSMessage.pbj.hpp"
#include "emerson/EmersonUtil.h"
#include "emerson/EmersonException.h"
#include "emerson/Util.h"
#include "JSSystemNames.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSTimerStruct.hpp"
#include "JSObjects/JSObjectsUtils.hpp"
#include "JSObjectStructs/JSUtilStruct.hpp"
#include <boost/lexical_cast.hpp>
#include "EmersonMessagingManager.hpp"
#include "EmersonHttpManager.hpp"

using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {


EmersonScript::EmersonScript(HostedObjectPtr ho, const String& args, const String& script, JSObjectScriptManager* jMan)
 : JSObjectScript(jMan, ho->getObjectHost()->getStorage(), ho->getObjectHost()->getPersistedObjectSet(), ho->id()),
   JSVisibleManager(this),
   EmersonMessagingManager(ho->context()),
   mParent(ho),
   mHandlingEvent(false),
   mResetting(false),
   mKilling(false),
   mCreateEntityPort(NULL),
   presenceToken(HostedObject::DEFAULT_PRESENCE_TOKEN +1),
   emHttpPtr(EmersonHttpManager::construct<EmersonHttpManager> (ho->context()))
{
    int32 resourceMax = mManager->getOptions()->referenceOption("emer-resource-max")->as<int32> ();
    JSObjectScript::initialize(args, script,resourceMax);

    // Subscribe for session events
    mParent->addListener((SessionEventListener*)this);
    // And notify the script of existing ones
    HostedObject::SpaceObjRefVec spaceobjrefs;
    mParent->getSpaceObjRefs(spaceobjrefs);
    if (spaceobjrefs.size() > 1)
        JSLOG(fatal,"Error: Connected to more than one space.  Only enabling scripting for one space.");

    //default connections.
    for(HostedObject::SpaceObjRefVec::const_iterator space_it = spaceobjrefs.begin(); space_it != spaceobjrefs.end(); space_it++)
        onConnected(mParent, *space_it, HostedObject::DEFAULT_PRESENCE_TOKEN);
}



//Here's how resetting works.  system reqeusts a reset.  At this point,
//JSContextStruct calls the below function (requestReset).  If reset was
//requested by root context, then set mResetting to true.  In the check handlers
//function, if mResetting is true, stops comparing event against handlers.
//Then, call resetScript.  resetScript tears down the rest of the script.
v8::Handle<v8::Value> EmersonScript::requestReset(JSContextStruct* jscont,const std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & proxSetVis)
{
    if (jscont != mContext)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  Cannot call reset unless within root context.")));

    mResetting = true;


    //need to load all elements of each presence's prox result set into
    //resetVisiblesResultSet.  That way, still hold references to them
    //even after all the emerson objects associated with them get disposed/gc-ed
    //during reset.
    for (std::map<SpaceObjectReference,std::vector<SpaceObjectReference> >::const_iterator presIter = proxSetVis.begin();
         presIter != proxSetVis.end(); ++presIter)
    {
        for (std::vector<SpaceObjectReference>::const_iterator proxSetIter = presIter->second.begin();
             proxSetIter != presIter->second.end();
             ++proxSetIter)
        {
            resettingVisiblesResultSet[presIter->first].push_back(createVisStruct(*proxSetIter));
        }
    }
    return v8::Undefined();
}

void EmersonScript::resetScript()
{
    //before cler presences, take all
    mResetting = false;
    mPresences.clear();
    mImportedFiles.clear();
    mContext->struct_rootReset();

    //replay all prox sets to each presence
    for (std::map<SpaceObjectReference, std::vector<JSVisibleStruct*> >::iterator presIter = resettingVisiblesResultSet.begin();
         presIter != resettingVisiblesResultSet.end(); ++presIter)
    {
        for (std::vector<JSVisibleStruct*>::iterator proxSetIter = presIter->second.begin();
             proxSetIter != presIter->second.end(); ++proxSetIter)
        {
            //issue user-callback that jsvisiblestruct contined in proxSetIter
            //is a member of the proximity result set for the presence that has
            //sporef presIter->first.
            notifyProximate(*proxSetIter, presIter->first);
        }
    }

    //note: do not expressly delete the allocated JSVisibleStructs here.  The
    //underlying JSVisibleManager takes care of this.
    resettingVisiblesResultSet.clear();
}

/**
   This function adds jspresStruct to mPresences map.  Then, calls the
   onPresence added function.
 */
void EmersonScript::resetPresence(JSPresenceStruct* jspresStruct)
{
    mPresences[jspresStruct->getSporef()] = jspresStruct;
}



//this is the callback that fires when proximateObject no longer receives
//updates from loc (ie the object in the world associated with proximate object
//is outside of querier's standing query registered to pinto).
void  EmersonScript::notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    if (isStopped()) {
        JSLOG(warn, "Ignoring proximity removal callback after shutdown request.");
        return;
    }

    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" went out of query of "<<querier<<".  Mostly just ignoring it.");


    PresenceMapIter iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"Error.  Received a notification that a proximate object left query set for querier "<<querier<<".  However, querier has no associated presence in array.  Aborting now");
        return;
    }

    if (mContext->proxRemovedFunc.IsEmpty())
    {
        JSLOG(info,"No proximity removal function");
        return;
    }

    JSVisibleStruct* jsvis =  createVisStruct(proximateObject->getObjectReference());

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);

    //jswrap the object
    //should be in context from createVisibleObject call
    v8::Handle<v8::Object> outOfRangeObject = createVisiblePersistent(jsvis,mContext->mContext);

    TryCatch try_catch;

    int argc = 2;
    String sporefVisTo = iter->first.toString();
    v8::Handle<v8::Value> argv[2] = { outOfRangeObject, v8::String::New( sporefVisTo.c_str()  ) };

    //FIXME: Potential memory leak: when will removedProxObj's
    //SpaceObjectReference field be garbage collected and deleted?
    JSLOG(detailed,"Issuing user callback for proximate object gone.  Argument passed");
    invokeCallback(mContext,mContext->proxRemovedFunc,argc,argv);

    if (try_catch.HasCaught()) {
        printException(try_catch);
    }

    postCallbackChecks();
}

v8::Persistent<v8::Object> EmersonScript::createVisiblePersistent(const SpaceObjectReference& visibleObj, JSProxyPtr addParams, v8::Handle<v8::Context> ctx)
{
    JSVisibleStruct* jsvis = createVisStruct(visibleObj, addParams);
    return createVisiblePersistent(jsvis,ctx);
}


v8::Persistent<v8::Object> EmersonScript::createVisiblePersistent(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctxToCreateIn);

    v8::Local<v8::Object> returner = mManager->mVisibleTemplate->GetFunction()->NewInstance();
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(jsvis));
    returner->SetInternalField(TYPEID_FIELD,v8::External::New(new String(VISIBLE_TYPEID_STRING)));

    v8::Persistent<v8::Object> returnerPers = v8::Persistent<v8::Object>::New(returner);
    returnerPers.MakeWeak(NULL,&JSVisibleStruct::visibleWeakReferenceCleanup);
    return returnerPers;
}




//if can't find visible, will just return self object
//this is a mess of a function to get things to work again.
//this function will actually need to be super-cleaned up
v8::Handle<v8::Value> EmersonScript::findVisible(const SpaceObjectReference& proximateObj)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);

    JSVisibleStruct* jsvis = createVisStruct(proximateObj);
    v8::Persistent<v8::Object> returnerPers =createVisiblePersistent(jsvis, mContext->mContext);
    return returnerPers;
}

//debugging code to output the sporefs of all the presences that I have in mPresences
void EmersonScript::printMPresences()
{
    std::cout<<"\n\n";
    std::cout<<"Printing mPresences with size: "<< mPresences.size()<<"\n";
    for (PresenceMapIter iter = mPresences.begin(); iter != mPresences.end(); ++iter)
        std::cout<<"pres: "<<iter->first<<"\n";

    std::cout<<"\n\n";
}




void EmersonScript::notifyProximate(JSVisibleStruct* proxVis, const SpaceObjectReference& proxTo)
{
    if (isStopped()) {
        JSLOG(warn, "Ignoring proximity addition callback after shutdown request.");
        return;
    }

    // Invoke user callback
    PresenceMapIter iter = mPresences.find(proxTo);
    if (iter == mPresences.end())
    {
        JSLOG(error,"No presence associated with sporef "<<proxTo<<" exists in presence mapping when getting notifyProximate.  Taking no action.");
        return;
    }

    if (mContext->proxAddedFunc.IsEmpty())
    {
        JSLOG(detailed,"No prox added func to execute");
        return;
    }

    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> newVisibleObj = createVisiblePersistent(proxVis, mContext->mContext);

    v8::Context::Scope context_scope(mContext->mContext);
    TryCatch try_catch;

    int argc = 2;
    String sporefVisTo = iter->first.toString();
    v8::Handle<v8::Value> argv[2] = { newVisibleObj, v8::String::New( sporefVisTo.c_str()  ) };


    //FIXME: Potential memory leak: when will newAddrObj's
    //SpaceObjectReference field be garbage collected and deleted?
    JSLOG(detailed,"Issuing user callback for proximate object.");
    invokeCallback(mContext,mContext->proxAddedFunc, argc, argv);


    if (try_catch.HasCaught()) {
        printException(try_catch);
    }

    postCallbackChecks();
}


//Gets called by notifier when PINTO states that proximateObject originally
//satisfies the solid angle query registered by querier
void  EmersonScript::notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" is within query of "<<querier<<".");
    JSVisibleStruct* jsvis = JSVisibleManager::createVisStruct(proximateObject->getObjectReference());
    notifyProximate(jsvis,querier);
}



JSInvokableObject::JSInvokableObjectInt* EmersonScript::runSimulation(const SpaceObjectReference& sporef, const String& simname)
{
    TimeSteppedSimulation* sim = mParent->runSimulation(sporef,simname);

    return new JSInvokableObject::JSInvokableObjectInt(sim);
}

//requested by scripters.
v8::Handle<v8::Value> EmersonScript::killEntity(JSContextStruct* jscont)
{
    if (jscont != rootContext())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Can only killEntity from root context.")) );


    mKilling = true;
    return v8::Null();
}


//requested internally after break out of execution loop.
void EmersonScript::killScript()
{
    {
        // Kill the persistent copy of this object since it shouldn't be
        // restored after being explicitly killed.
        v8::HandleScope handle_scope;
        v8::Persistent<v8::Function>emptyCB;

        //last two args as "" means that we remove the restore script from
        //storage.
        setRestoreScript(mContext,"",emptyCB);
    }
    stop();
    mParent->destroy();
}

void EmersonScript::postCallbackChecks() {
    //if one of the actions that your handler took was to call reset, then reset
    //the entire script.
    if (mResetting)
        resetScript();

    if (mKilling)
        killScript();
}

void EmersonScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name, HostedObject::PresenceToken token)
{
    //register underlying visible manager to listen for proxy creation events on
    //hostedobjectproxymanager
    ProxyManagerPtr proxy_manager = mParent->getProxyManager(name.space(),name.object());
    proxy_manager->addListener(this);
    // Proxies for the object connected are created before this occurs, so we
    // need to manually notify of it:
    ProxyObjectPtr self_proxy = proxy_manager->getProxyObject(name);
    this->onCreateProxy(self_proxy);

    //register for scripting messages from user
    SpaceID space_id = name.space();
    ObjectReference obj_refer = name.object();

    v8::HandleScope handle_scope;

    //register port for messaging
    ODP::Port* msgPort = mParent->bindODPPort(space_id, obj_refer, Services::COMMUNICATION);
    if (msgPort != NULL)
    {
        mMessagingPortMap[SpaceObjectReference(space_id,obj_refer)] = msgPort;
        msgPort->receive( std::tr1::bind(&EmersonScript::handleScriptCommUnreliable, this, _1, _2, _3));
    }

    if (!mCreateEntityPort)
        mCreateEntityPort = mParent->bindODPPort(space_id,obj_refer, Services::CREATE_ENTITY);

    //set up reliable messages for the connected presence
    EmersonMessagingManager::presenceConnected(name);


    //check for callbacks associated with presence connection

    //means that this is the first presence that has been added to the space
    if (token == HostedObject::DEFAULT_PRESENCE_TOKEN)
    {
        JSPresenceStruct* jspres = addConnectedPresence(name,token);
        mContext->checkContextConnectCallback(jspres);
    }
    else
    {
        //means that we've connected a presence and should finish by calling
        //connection callback
        callbackUnconnected(self_proxy,token);
    }
}


void EmersonScript::callbackUnconnected(ProxyObjectPtr proxy, HostedObject::PresenceToken token)
{
    for (PresenceVec::iterator iter = mUnconnectedPresences.begin(); iter != mUnconnectedPresences.end(); ++iter)
    {
        if (token == (*iter)->getPresenceToken())
        {
            JSPresenceStruct* pstruct = *iter;
            mPresences[proxy->getObjectReference()] = pstruct;

            mUnconnectedPresences.erase(iter);
            // Make sure this call is last since it invokes a callback which
            // could in turn call connect requests and modify mUnconnectedPresences.
            pstruct->connect(proxy->getObjectReference());
            return;
        }
    }
    JSLOG(error,"Error, received a finished connection with token "<<token<<" that we do not have an unconnected presence struct for.");
}


//called by JSPresenceStruct.  requests the parent HostedObject disconnect
//the presence associated with jspres
void EmersonScript::requestDisconnect(JSPresenceStruct* jspres)
{
    SpaceObjectReference sporef = (jspres->getSporef());
    mParent->disconnectFromSpace(sporef.space(), sporef.object());
}

void EmersonScript::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name)
{
    // We need to mark disconnection here so we don't request
    // disconnection twice, but the callback has to be deferred until later
    PresenceMap::iterator internal_it = mPresences.find(name);
    if (internal_it == mPresences.end()) {
        JSLOG(error,"Got onDisconnected for presence not being tracked");
    }
    else {
        JSPresenceStruct* jspres = internal_it->second;
        jspres->markDisconnected();
    }

    // Unregister from ProxyManager events. We maintain the presence data until
    // it is truly deleted (at destruction or gc) since we might still get
    // requests for its data
    unsubscribePresenceEvents(name);

    EmersonMessagingManager::presenceDisconnected(name);

    // Defer processing of the disconnect for the presence, making sure that the
    // disconnection event is executed separately. This is necessary because
    // this entire process may have been triggered from within a currently
    // executing callback, resulting in weird results, e.g. if you disconnected,
    // got a disconnect callback, and in that callback killed the entity. This
    // could cause conflicting uses of data structures (e.g. live presences)
    // resulting in double disconnects, etc.
    mParent->context()->mainStrand->post(std::tr1::bind(&EmersonScript::finishOnDisconnected, this, name));
}

void EmersonScript::finishOnDisconnected(const SpaceObjectReference& name) {
    PresenceMap::iterator internal_it = mPresences.find(name);
    // Because of the delay inprocessing, we may not have the presence anymore.
    if (internal_it == mPresences.end()) return;
    JSPresenceStruct* jspres = internal_it->second;
    if (jspres != NULL)
        jspres->handleDisconnectedCallback();
}


void EmersonScript::create_entity(EntityCreateInfo& eci)
{
    HostedObjectPtr obj = mParent->getObjectHost()->createObject(eci.scriptType, eci.scriptOpts, eci.scriptContents);

    obj->connect(eci.space,
        eci.loc,
        BoundingSphere3f(Vector3f::nil(), eci.scale),
        eci.mesh,
        eci.physics,
        eci.solid_angle,
        UUID::null(),
        ObjectReference::null()
    );
}

EmersonScript::~EmersonScript()
{
}


void EmersonScript::start() {
    JSObjectScript::start();
}

void EmersonScript::stop() {
    // Clean up ProxyCreationListeners. We subscribe for each presence in
    // onConnected, so we need to run through all presences (stored in the
    // HostedObject) and clear out ourselfs as a listener. Note that we have to
    // use our own list of presences (don't use HostedObject::getSpaceObjRefs)
    // because we track presences *after* space-stream connection whereas the
    // HostedObject tracks them after the initial connected reply message from
    // the space.
    //
    // NOTE: We put this before JSObjectScript::stop because that triggers the
    // presences to get removed from mPresences but not unsubscribed.
    for (PresenceMap::const_iterator it = mPresences.begin(); it != mPresences.end(); it++)
        unsubscribePresenceEvents(it->first);

    JSObjectScript::stop();

    mParent->removeListener((SessionEventListener*)this);

    mPresences.clear();
}

bool EmersonScript::valid() const
{
    return (mParent);
}



void EmersonScript::sendMessageToEntityUnreliable(const SpaceObjectReference& sporef, const SpaceObjectReference& from, const std::string& msgBody)
{
    std::map<SpaceObjectReference, ODP::Port*>::iterator iter = mMessagingPortMap.find(from);
    if (iter == mMessagingPortMap.end())
    {
        JSLOG(error,"Trying to send from a sporef that does not exist");
        return;
    }

    ODP::Endpoint dest (sporef.space(),sporef.object(),Services::COMMUNICATION);
    MemoryReference toSend(msgBody);

    iter->second->send(dest,toSend);
}


Time EmersonScript::getHostedTime()
{
    return mParent->currentLocalTime();
}


v8::Handle<v8::Value> EmersonScript::create_event(v8::Persistent<v8::Function>& cb, JSContextStruct* jscont) {
    if (mParent->context()->stopped()) {
        JSLOG(warn, "Not creating event because shutdown was requested.");
        return v8::Boolean::New(false);
    }

    mParent->context()->mainStrand->post(
        std::tr1::bind(&EmersonScript::invokeCallbackInContext, this, livenessToken(), cb, jscont)
    );
    return v8::Boolean::New(true);
}


v8::Handle<v8::Value> EmersonScript::create_timeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared, JSContextStruct* jscont)
{
    JSTimerStruct* jstimer = new JSTimerStruct(this,Duration::seconds(period),cb,jscont,mParent->context(),contID, timeRemaining,isSuspended,isCleared);

    v8::HandleScope handle_scope;

    //create an object
    v8::Persistent<v8::Object> returner = v8::Persistent<v8::Object>::New(mManager->mTimerTemplate->NewInstance());
    returner->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(jstimer));
    returner->SetInternalField(TYPEID_FIELD, External::New(new String("timer")));

    returner.MakeWeak(NULL,&JSTimerStruct::timerWeakReferenceCleanup);

    //timer requires a handle to its persistent object so can handle cleanup
    //correctly.
    jstimer->setPersistentObject(returner);


    return handle_scope.Close(returner);
}

v8::Handle<v8::Value> EmersonScript::create_timeout(double period, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont)
{
    return create_timeout(period,cb,jscont->getContextID(),0,false,false,jscont);
}



//third arg may be null to evaluate in global context
void EmersonScript::invokeCallbackInContext(Liveness::Token alive, v8::Persistent<v8::Function> cb, JSContextStruct* jscontext)
{
    if (!alive) return;

    v8::HandleScope handle_scope;
    v8::Context::Scope(jscontext->mContext);
    TryCatch try_catch;
    invokeCallback( (jscontext == NULL ? mContext : jscontext), cb);
    postCallbackChecks();
}

//calls funcToCall in jscont, binding jspres bound as first arg.
//mostly used for contexts and presences to execute their callbacks on
//connection and disconnection events
void EmersonScript::handlePresCallback( v8::Handle<v8::Function> funcToCall,JSContextStruct* jscont, JSPresenceStruct* jspres)
{
    if (isStopped()) {
        JSLOG(warn, "Ignoring presence callback after shutdown request.");
        return;
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope(jscont->mContext);
    TryCatch try_catch;
    v8::Handle<v8::Value> js_pres =wrapPresence(jspres,&(jscont->mContext));
    invokeCallback(jscont, funcToCall, 1,&js_pres);
    postCallbackChecks();
}





/*
 * From the odp::endpoint & src and destination, checks if the corresponding
 * visible object already existed in the visible array.  If it does, return the
 * associated visible object.  If it doesn't, then return a new visible object
 * with stillVisible false associated with this object.
 */
v8::Handle<v8::Object> EmersonScript::getMessageSender(const ODP::Endpoint& src)
{
    v8::HandleScope handle_scope;
    SpaceObjectReference from(src.space(), src.object());

    JSVisibleStruct* jsvis = createVisStruct(from);
    v8::Persistent<v8::Object> returner =createVisiblePersistent(jsvis, mContext->mContext);

    return handle_scope.Close(returner);
}




void EmersonScript::registerFixupSuspendable(JSSuspendable* jssusp, uint32 contID)
{
    toFixup[contID].push_back(jssusp);
}




void EmersonScript::registerContextForClear(JSContextStruct* jscont)
{
    if (mHandlingEvent)
        contextsToClear.push_back(jscont);
    else
        finishContextClear(jscont);
}

void EmersonScript::finishContextClear(JSContextStruct* jscont)
{
    //tell it to finish clearing itself.
    jscont->finishClear();

    //also remove it from my list of sandboxes/contexts
    for (std::map<uint32, JSContextStruct*>::iterator findContIter = mContStructMap.begin();
         findContIter != mContStructMap.end();
         ++findContIter)
    {
        if (findContIter->second == jscont)
        {
            mContStructMap.erase(findContIter);
            break;
        }
    }
}

bool EmersonScript::handleScriptCommRead(const SpaceObjectReference& src, const SpaceObjectReference& dst, const String& payload)
{
    Sirikata::JS::Protocol::JSMessage jsMsg;
    Sirikata::JS::Protocol::JSFieldValue jsFieldVal;
    bool isJSMsg   = jsMsg.ParseFromString(payload);
    bool isJSField = false;
    if (!isJSMsg)
        isJSField = jsFieldVal.ParseFromString(payload);

    //if can't decode the payload as a jsmessage or
    //a jsfieldval, then return false;
    if (!(isJSMsg || isJSField))
        return false;
    

    if (isStopped()) {
        JSLOG(warn, "Ignoring message after shutdown request.");
        // Regardless of whether we can or not, just say we can't decode it.
        return false;
    }

    //cannot affect the event handlers when we are executing event handlers.
    mHandlingEvent = true;

    //creating a new context or destroying an old one would invalidate
    //mContStructMap iterator.  So when we receive a message, we first copy
    //existing contextstructs and then run through all of them, invoking their
    //callbacks.  Only after doing this do we delet contexts.
    std::vector<JSContextStruct*> currentContexts;
    for (std::map<uint32,JSContextStruct*>::iterator contIter = mContStructMap.begin();
         contIter != mContStructMap.end();++contIter)
    {
        currentContexts.push_back(contIter->second);
    }


    for (std::vector<JSContextStruct*>::iterator curContIter = currentContexts.begin();
         curContIter != currentContexts.end();
         ++curContIter)
    {
        JSContextStruct* receiver = *curContIter;
        if (receiver->canReceiveMessagesFor(dst))
        {
            //If callback for presence messages on receiver doesn't exist, then don't do
            //anything, just return.
            if (receiver->presenceMessageCallback.IsEmpty())
                continue;

            v8::HandleScope handle_scope;
            v8::Context::Scope context_scope (receiver->mContext);

            v8::Handle<v8::Object> msgSender =createVisiblePersistent(SpaceObjectReference(src.space(),src.object()), JSProxyPtr() ,receiver->mContext);

            //try deserialization
            bool deserializeWorks =false;

            v8::Handle<v8::Value> msgVal;
            if (isJSMsg)
            {
                //try to decode as object.
                msgVal = JSSerializer::deserializeObject( this, jsMsg,deserializeWorks);
            }
            else
            {
                //try to decode as a value.
                msgVal = JSSerializer::deserializeMessage(this,jsFieldVal,deserializeWorks);
            }
            if (! deserializeWorks)
            {
                JSLOG(error, "Deserialization Failed!!");
                mHandlingEvent =false;
                return false;
            }

            v8::Handle<v8::Value> argv[3];
            argv[0] =msgVal;
            argv[1] = msgSender;
            argv[2] = v8::String::New (dst.toString().c_str(), dst.toString().size());
            invokeCallback(receiver,receiver->presenceMessageCallback,3,argv);
        }
    }

    mHandlingEvent = false;

    for (std::vector<JSContextStruct*>::iterator toClearIter = contextsToClear.begin();
         toClearIter != contextsToClear.end();
         ++toClearIter)
    {
        finishContextClear(*toClearIter);
    }
    contextsToClear.clear();
    postCallbackChecks();

    return true;
}


/**
   Used for unreliable messages.
 */
void EmersonScript::handleScriptCommUnreliable (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    SpaceObjectReference to  (dst.space(), dst.object());
    SpaceObjectReference from(src.space(), src.object());

    JSLOG(error, "Error handling unreliable message.  Not currently parsing message");
    
    // Sirikata::JS::Protocol::JSMessage js_msg;
    // bool parsed = js_msg.ParseFromArray(payload.data(), payload.size());
    //    deserializeMsgAndDispatch(from,to,js_msg);
}


v8::Handle<v8::Value> EmersonScript::sendSandbox(const String& msgToSend, uint32 senderID, uint32 receiverID)
{
    //posting task so that still get asynchronous messages.
    mParent->getIOService()->post(std::tr1::bind(&EmersonScript::processSandboxMessage, this,msgToSend,senderID,receiverID));
    return v8::Undefined();
}

void EmersonScript::processSandboxMessage(const String& msgToSend, uint32 senderID, uint32 receiverID)
{
    //FIXME: there's a chance that when post was called in sendSandbox, the
    //sandbox sender was destroyed and then a new one created with the same
    //senderID.  That's exceptionally unlikely, but may want to fix just in
    //case.

    //check to ensure that sender and receiver still exist.  If either don't,
    //then drop the mesage.
    std::map<uint32,JSContextStruct*>::iterator senderFinder = mContStructMap.find(senderID);
    if (senderFinder == mContStructMap.end())
        return;

    JSContextStruct* sender = senderFinder->second;

    std::map<uint32,JSContextStruct*>::iterator receiverFinder = mContStructMap.find(receiverID);
    if (receiverFinder == mContStructMap.end())
        return;

    JSContextStruct* receiver = receiverFinder->second;

    //If callback for sandbox messages on receiver doesn't exist, then don't do
    //anything, just return.
    if (receiver->sandboxMessageCallback.IsEmpty())
        return;


    //deserialize the message to an object
    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromArray(msgToSend.data(), msgToSend.size());
    if (!parsed)
        return;

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(receiver->mContext);

    bool deserializeWorks;
    v8::Handle<v8::Object> msgObj = JSSerializer::deserializeObject( this, js_msg,deserializeWorks);
    if (! deserializeWorks)
        return;


    v8::Handle<v8::Value> argv[2];
    argv[0] =msgObj;

    if (receiver->mParentContext == sender)
        argv[1] =  v8::Null();
    else
    {
        v8::Local<v8::Object> senderObj =mManager->mContextTemplate->NewInstance();
        senderObj->SetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT, External::New(sender));
        senderObj->SetInternalField(TYPEID_FIELD,External::New(new String(CONTEXT_TYPEID_STRING)));
        argv[1] = senderObj;
    }


    invokeCallback(receiver,receiver->sandboxMessageCallback,2,argv);
    postCallbackChecks();
}


//takes in a presence struct to remove from mPresences map.  Additionally,
//requests the HostedObject to remove the presence.
void EmersonScript::deletePres(JSPresenceStruct* toDelete)
{
    //remove the presence from mUnconnectedPresences
    bool found= true;
    while (found)
    {
        found = false;
        for (PresenceVec::iterator iter = mUnconnectedPresences.begin();
             iter != mUnconnectedPresences.end(); ++iter)
        {
            if (*iter == toDelete)
            {
                mUnconnectedPresences.erase(iter);
                found = true;
                break;
            }
        }
    }

    //remove the presence from mPresences
    SpaceObjectReference sporefToDelete = toDelete->getSporef();

    // We might still need to disconnect the presence.
    if (toDelete->getIsConnected()) {
        mParent->disconnectFromSpace(sporefToDelete.space(),sporefToDelete.object());
    }

    removePresenceData(sporefToDelete);
    delete toDelete;
}


void EmersonScript::unsubscribePresenceEvents(const SpaceObjectReference& name) {
    PresenceMapIter pIter = mPresences.find(name);
    if (pIter != mPresences.end()) {
        ProxyManagerPtr proxy_manager = mParent->getProxyManager(name.space(), name.object());
        if (proxy_manager) {
            proxy_manager->removeListener(this);
        }
    }
}

void EmersonScript::removePresenceData(const SpaceObjectReference& sporefToDelete) {
    PresenceMapIter pIter = mPresences.find(sporefToDelete);
    if (pIter != mPresences.end())
        mPresences.erase(pIter);
}



//takes the c++ object jspres, creates a new visible object out of it, if we
//don't already have a c++ visible object associated with it (if we do, use
//that one), wraps that c++ object in v8, and returns it as a v8 object to
//user
v8::Persistent<v8::Object> EmersonScript::presToVis(JSPresenceStruct* jspres, JSContextStruct* jscont)
{
    JSVisibleStruct* jsvis = createVisStruct(jspres->getSporef());
    return createVisiblePersistent(jsvis, jscont->mContext);
}


JSPresenceStruct*  EmersonScript::addConnectedPresence(const SpaceObjectReference& sporef,HostedObject::PresenceToken token)
{
    JSPresenceStruct* presToAdd = new JSPresenceStruct(this, sporef,mContext,token);
    // Add to our internal map
    mPresences[sporef] = presToAdd;
    return presToAdd;
}



//should be called from something that already has declared a handlescope,
//wraps the presence in a v8 object and returns it.
v8::Local<v8::Object> EmersonScript::wrapPresence(JSPresenceStruct* presToWrap, v8::Persistent<v8::Context>* ctxToWrapIn)
{
    v8::HandleScope handle_scope;
    v8::Handle<v8::Context> ctx = (ctxToWrapIn == NULL) ? mContext->mContext : *ctxToWrapIn;
    v8::Context::Scope context_scope(ctx);

    Local<Object> js_pres = mManager->mPresenceTemplate->GetFunction()->NewInstance();
    js_pres->SetInternalField(PRESENCE_FIELD_PRESENCE,External::New(presToWrap));
    js_pres->SetInternalField(TYPEID_FIELD,External::New(new String(PRESENCE_TYPEID_STRING)));

    return handle_scope.Close(js_pres);
}



//looks through all previously connected presneces (located in mPresences).
//returns the corresponding jspresencestruct that has a spaceobjectreference
//that matches sporef.
JSPresenceStruct* EmersonScript::findPresence(const SpaceObjectReference& sporef)
{
    PresenceMap::iterator internal_it = mPresences.find(sporef);
    if (internal_it == mPresences.end())
    {
        JSLOG(error, "Got findPresence call for Presence that wasn't being tracked.");
        return NULL;
    }
    return internal_it->second;
}

v8::Handle<v8::Value> EmersonScript::restorePresence(PresStructRestoreParams& psrp,JSContextStruct* jsctx)
{
    v8::Context::Scope context_scope(jsctx->mContext);

    // Sometimes, we might call restore presence on a presence that already
    //exists and might even be connected, e.g. if system.reset() is called on a
    //script that tries to restore from object storage (the presence isn't
    //deleted by reset, but when the script finds the presence in storage it
    //invokes restore presence). Its not clear whether we should use the
    //existing settings or override them with the restored values, so for now,
    //if we find the presence we just return it directly.
    PresenceMapIter piter = mPresences.find(psrp.sporef);
    if (piter != mPresences.end()) {
        v8::HandleScope handle_scope;
        return handle_scope.Close(wrapPresence(piter->second,&(jsctx->mContext)));
    }

    MotionVector3f motVec(psrp.position,psrp.velocity);
    TimedMotionVector3f tMotVec(mParent->currentLocalTime(),motVec);
    if (!psrp.positionTime.isNull())
    {
        tMotVec = TimedMotionVector3f (psrp.positionTime.getValue(),motVec);
        psrp.position = tMotVec.extrapolate(mParent->currentLocalTime()).position();
    }
    Vector3d newPosD (psrp.position.x,psrp.position.y,psrp.position.z);


    MotionQuaternion motQuat(psrp.orient,psrp.orientVelocity);
    TimedMotionQuaternion tMotQuat (mParent->currentLocalTime(), motQuat);
    if (!psrp.orientTime.isNull())
    {
        tMotQuat = TimedMotionQuaternion(psrp.orientTime.getValue(), motQuat);
        psrp.orient = tMotQuat.extrapolate(mParent->currentLocalTime()).position();
    }

    Vector3f newAngAxis;
    float newAngVel;
    psrp.orientVelocity.toAngleAxis(newAngVel,newAngAxis);

    BoundingSphere3f bs = BoundingSphere3f(psrp.position, psrp.scale);

    Location newLoc(newPosD,psrp.orient,psrp.velocity, newAngAxis,newAngVel);


    HostedObject::PresenceToken presToke = incrementPresenceToken();
    JSPresenceStruct* jspres = new JSPresenceStruct(this,psrp,psrp.position,
        presToke,jsctx,tMotVec,tMotQuat);

    if (psrp.isConnected)
    {
        mParent->connect(psrp.sporef.space(),
            newLoc,
            bs,
            psrp.mesh,
            psrp.physics,
            psrp.query,
            UUID::null(),
            psrp.sporef.object(),
            presToke
        );

        mUnconnectedPresences.push_back(jspres);
        return v8::Null();
    }
    //if is unconnected, return presence now.
    v8::HandleScope handle_scope;
    return handle_scope.Close(wrapPresence(jspres,&(jsctx->mContext)));
}




//This function returns to you the current value of present token and incrmenets
//presenceToken so that get a unique one each time.  If presenceToken is equal
//to default_presence_token, increments one beyond it so that don't start inadvertently
//returning the DEFAULT_PRESENCE_TOKEN;
HostedObject::PresenceToken EmersonScript::incrementPresenceToken()
{
    HostedObject::PresenceToken returner = presenceToken++;
    if (returner == HostedObject::DEFAULT_PRESENCE_TOKEN)
        return incrementPresenceToken();

    return returner;
}


void EmersonScript::setOrientationVelFunction(const SpaceObjectReference sporef,const Quaternion& quat)
{
    mParent->requestOrientationVelocityUpdate(sporef.space(),sporef.object(),quat);
}

void EmersonScript::setPositionFunction(const SpaceObjectReference sporef, const Vector3f& posVec)
{
    mParent->requestPositionUpdate(sporef.space(),sporef.object(),posVec);
}


//velocity
void EmersonScript::setVelocityFunction(const SpaceObjectReference sporef, const Vector3f& velVec)
{
    mParent->requestVelocityUpdate(sporef.space(),sporef.object(),velVec);
}



//orientation
void  EmersonScript::setOrientationFunction(const SpaceObjectReference sporef, const Quaternion& quat)
{
    mParent->requestOrientationDirectionUpdate(sporef.space(),sporef.object(),quat);
}



//scale
void EmersonScript::setVisualScaleFunction(const SpaceObjectReference sporef, float newscale)
{
    BoundingSphere3f bnds = mParent->requestCurrentBounds(sporef.space(),sporef.object());
    bnds = BoundingSphere3f(bnds.center(), newscale);
    mParent->requestBoundsUpdate(sporef.space(),sporef.object(), bnds);
}



//mesh
//FIXME: May want to have an error handler for this function.
void  EmersonScript::setVisualFunction(const SpaceObjectReference sporef, const std::string& newMeshString)
{
    //FIXME: need to also pass in the object reference
    mParent->requestMeshUpdate(sporef.space(),sporef.object(),newMeshString);
}

//physics
v8::Handle<v8::Value> EmersonScript::getPhysicsFunction(const SpaceObjectReference sporef)
{
    String curphy = mParent->requestCurrentPhysics(sporef.space(),sporef.object());
    return v8::String::New(curphy.c_str(), curphy.size());
}

//FIXME: May want to have an error handler for this function.
void EmersonScript::setPhysicsFunction(const SpaceObjectReference sporef, const String& newPhyString)
{
    //FIXME: need to also pass in the object reference
    mParent->requestPhysicsUpdate(sporef.space(), sporef.object(), newPhyString);
}


//just sets the solid angle query for the object.
void EmersonScript::setQueryAngleFunction(const SpaceObjectReference sporef, const SolidAngle& sa)
{
    mParent->requestQueryUpdate(sporef.space(), sporef.object(), sa);
}


SolidAngle EmersonScript::getQueryAngle(const SpaceObjectReference sporef)
{
    SolidAngle returner = mParent->requestQueryAngle(sporef.space(),sporef.object());
    return returner;
}


} // namespace JS
} // namespace Sirikata
