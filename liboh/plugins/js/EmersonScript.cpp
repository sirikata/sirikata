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
#include <sirikata/proxyobject/ProxyManager.hpp>
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
#include "JSObjects/JSInvokableUtil.hpp"

using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {

#define EMERSON_UNRELIABLE_COMMUNICATION_PORT 12

EmersonScript::EmersonScript(HostedObjectPtr ho, const String& args,
    const String& script, JSObjectScriptManager* jMan,
    JSCtx* ctx)
 : JSObjectScript(jMan, ho->getObjectHost()->getStorage(),
     ho->getObjectHost()->getPersistedObjectSet(), ho->id(),
     ctx),
   EmersonMessagingManager(ho->context()),
   jsVisMan(ctx),
   mParent(ho),
   mHandlingEvent(false),
   mResetting(false),
   mKilling(false),
   presenceToken(HostedObject::DEFAULT_PRESENCE_TOKEN +1),
   emHttpPtr(EmersonHttpManager::construct<EmersonHttpManager> (ctx))
{
    //v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
    v8::Locker locker (mCtx->mIsolate);
    JSObjectScript::mCtx->mIsolate->Enter();

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
        iOnConnected(mParent, *space_it, HostedObject::DEFAULT_PRESENCE_TOKEN,true,Liveness::livenessToken());


    JSObjectScript::mCtx->mIsolate->Exit();
    JSObjectScript::mCtx->initialize();
}



//Here's how resetting works.  system reqeusts a reset.  At this point,
//JSContextStruct calls the below function (requestReset).  If reset was
//requested by root context, then set mResetting to true.  In the check handlers
//function, if mResetting is true, stops comparing event against handlers.
//Then, call resetScript.  resetScript tears down the rest of the script.
v8::Handle<v8::Value> EmersonScript::requestReset(JSContextStruct* jscont,const std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & proxSetVis)
{
    EMERSCRIPT_SERIAL_CHECK();
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
            resettingVisiblesResultSet[presIter->first].push_back(
                jsVisMan.createVisStruct(this, *proxSetIter));
        }
    }
    return v8::Undefined();
}

void EmersonScript::resetScript()
{
    EMERSCRIPT_SERIAL_CHECK();
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
            iNotifyProximateHelper(*proxSetIter, presIter->first);
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
    EMERSCRIPT_SERIAL_CHECK();
    mPresences[jspresStruct->getSporef()] = jspresStruct;
}


//this is the callback that fires when proximateObject no longer receives
//updates from loc (ie the object in the world associated with proximate object
//is outside of querier's standing query registered to pinto).
//should be called from mainStrand
void  EmersonScript::notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    if (isStopped()) {
        JSLOG(warn, "Ignoring proximity removal callback after shutdown request.");
        return;
    }

    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iNotifyProximateGone,this,
            proximateObject,querier,Liveness::livenessToken()),
        "EmersonScript::iNotifyProximateGone"
    );
}

void EmersonScript::iNotifyProximateGone(
    ProxyObjectPtr proximateObject, const SpaceObjectReference& querier,
    Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;

    EMERSCRIPT_SERIAL_CHECK();
    while(!JSObjectScript::mCtx->initialized())
    {}
    if(JSObjectScript::mCtx->stopped())
    {
        JSLOG(warn, "Ignoring proximity removal callback after shutdown request.");
        return;
    }

    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" went out of query of "<<querier<<".  Mostly just ignoring it.");
    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);

    //FIXME: we aren't ever freeing this memory
    //lkjs; what about freeing this memeory?;
    JSVisibleStruct* jsvis =
        jsVisMan.createVisStruct(this, proximateObject->getObjectReference());

    std::map<uint32, JSContextStruct*>::iterator contIter;
    for (contIter  =  mContStructMap.begin(); contIter != mContStructMap.end();
         ++contIter)
    {
        contIter->second->proximateEvent(querier, jsvis,true);
    }
}

//called after reset occurs from JSContextStruct.  Should be called from inside objStrand
void EmersonScript::fireProxEvent(const SpaceObjectReference& localPresSporef,
    JSVisibleStruct* jsvis, JSContextStruct* jscont, bool isGone)
{
    EMERSCRIPT_SERIAL_CHECK();
    
    if (mEvalContextStack.empty())
        assert(false);
    
    //this entire pre-amble is gross.
    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx,jscont);
    ScopedEvalContext sec(this,new_ctx);
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(jscont->mContext);

    //create an associated visible object in correct context
    v8::Local<v8::Object> visiblePres =
        createVisibleWeakPersistent(jsvis);

    TryCatch try_catch;

    int argc = 2;
    String sporefVisTo = localPresSporef.toString();

    v8::Handle<v8::Value> argv[2] = {visiblePres,
                                     v8::String::New( sporefVisTo.c_str()  ) };

    //FIXME: Potential memory leak: when will removedProxObj's
    //SpaceObjectReference field be garbage collected and deleted?
    JSLOG(detailed,"Issuing user callback for proximate object gone.  Argument passed");
    if (isGone)
        invokeCallback(jscont,jscont->proxRemovedFunc,argc,argv);
    else
        invokeCallback(jscont,jscont->proxAddedFunc,argc,argv);

    if (try_catch.HasCaught()) {
        printException(try_catch);
    }
    postCallbackChecks();

}

boost::any EmersonScript::invokeInvokable(
    std::vector<boost::any>& params,v8::Persistent<v8::Function> function_)
{
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iInvokeInvokable,this,
            params,function_,Liveness::livenessToken()),
        "EmersonScript::iInvokeInvokable"
    );

    return boost::any_cast<bool>(true);
}


void EmersonScript::iInvokeInvokable(
    std::vector<boost::any>& params,v8::Persistent<v8::Function> function_,
    Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;


    if (JSObjectScript::mCtx->stopped())
        return;

    while(!JSObjectScript::mCtx->initialized())
    {}


    EMERSCRIPT_SERIAL_CHECK();
    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
    int argc = params.size();

    /**
       FIXME: lkjs All invokables are executed from the root context.  This is
       incorrect.
     */
    v8::HandleScope handle_scope;
    v8::Context::Scope  context_scope(context());

    std::vector<v8::Handle<v8::Value> >argv(argc);

    for(uint32 i = 0; i < params.size(); i++)
        argv[i] = InvokableUtil::AnyToV8(this, params[i]);

    // We are currently executing in the global context of the entity
    // FIXME: need to take care fo the "this" pointer
    v8::Handle<v8::Value> result;
    if (argc > 0)
        result = invokeCallback(rootContext(), function_, argc, &argv[0]);
    else
        result = invokeCallback(rootContext(), function_);
}

//should already be in a context by the time this is called
v8::Local<v8::Object> EmersonScript::createVisibleWeakPersistent(const SpaceObjectReference& presID, const SpaceObjectReference& visibleObj, JSVisibleDataPtr addParams)
{
    return createVisibleWeakPersistent(visibleObj, addParams);
}

//should already be in a context by the time this is called
v8::Local<v8::Object> EmersonScript::createVisibleWeakPersistent(const SpaceObjectReference& visibleObj, JSVisibleDataPtr addParams)
{
    EMERSCRIPT_SERIAL_CHECK();
    v8::HandleScope handle_scope;
    JSVisibleStruct* jsvis =
        jsVisMan.createVisStruct(this, visibleObj, addParams);
    return handle_scope.Close(createVisibleWeakPersistent(jsvis));
}


//should already be in a context by the time this is called
v8::Local<v8::Object> EmersonScript::createVisibleWeakPersistent(JSVisibleStruct* jsvis)
{
    EMERSCRIPT_SERIAL_CHECK();
    v8::HandleScope handle_scope;
    v8::Local<v8::Object> returner = JSObjectScript::mCtx->mVisibleTemplate->GetFunction()->NewInstance();
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(jsvis));
    returner->SetInternalField(TYPEID_FIELD,v8::External::New(new String(VISIBLE_TYPEID_STRING)));

    v8::Persistent<v8::Object> returnerPers = v8::Persistent<v8::Object>::New(returner);

    //lkjs FIXME: may want to pass a liveness token through.
    returnerPers.MakeWeak(NULL,&JSVisibleStruct::visibleWeakReferenceCleanup);
    return handle_scope.Close(returner);
}




//if can't find visible, will just return self object
//this is a mess of a function to get things to work again.
//this function will actually need to be super-cleaned up
v8::Handle<v8::Value> EmersonScript::findVisible(const SpaceObjectReference& proximateObj)
{
    EMERSCRIPT_SERIAL_CHECK();
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);

    JSVisibleStruct* jsvis =
        jsVisMan.createVisStruct(this, proximateObj);
    v8::Local<v8::Object> returnerPers =createVisibleWeakPersistent(jsvis);
    return handle_scope.Close(returnerPers);
}



//Gets called by notifier when PINTO states that proximateObject originally
//satisfies the solid angle query registered by querier
void  EmersonScript::notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{


    if (JSObjectScript::mCtx->stopped())
    {
        JSLOG(warn, "Ignoring proximity addition callback after shutdown request.");
        return;
    }

    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iNotifyProximate,this,
            proximateObject,querier,Liveness::livenessToken()),
        "EmersonScript::iNotifyProximate"
    );
}


void  EmersonScript::iNotifyProximate(
    ProxyObjectPtr proximateObject, const SpaceObjectReference& querier,
    Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;


    EMERSCRIPT_SERIAL_CHECK();
    while(!JSObjectScript::mCtx->initialized())
    {}

    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
    if (JSObjectScript::mCtx->stopped())
    {
        JSLOG(warn, "Ignoring proximity addition callback after shutdown request.");
        return;
    }

    JSVisibleStruct* jsvis =
        jsVisMan.createVisStruct(this, proximateObject->getObjectReference());
    iNotifyProximateHelper(jsvis,querier);
}


void EmersonScript::iNotifyProximateHelper(
    JSVisibleStruct* proxVis, const SpaceObjectReference& proxTo)
{
    std::map<uint32, JSContextStruct*>::iterator contIter;
    for (contIter  =  mContStructMap.begin(); contIter != mContStructMap.end();
         ++contIter)
    {
        contIter->second->proximateEvent(proxTo, proxVis,false);
    }
}


JSInvokableObject::JSInvokableObjectInt* EmersonScript::runSimulation(
    const SpaceObjectReference& sporef, const String& simname)
{
    EMERSCRIPT_SERIAL_CHECK();
    
    Simulation* sim =
        mParent->runSimulation(sporef,simname,JSObjectScript::mCtx->objStrand);


    if (sim == NULL) return NULL;

    mSimulations.push_back(
        std::pair<String,SpaceObjectReference>(simname,sporef));
    
    return new JSInvokableObject::JSInvokableObjectInt(sim);
}

//requested by scripters.
v8::Handle<v8::Value> EmersonScript::killEntity(JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (jscont != rootContext())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Can only killEntity from root context.")) );

    mKilling = true;
    return v8::Null();
}


//requested internally after break out of execution loop.
void EmersonScript::killScript()
{
    EMERSCRIPT_SERIAL_CHECK();
    {
        // Kill the persistent copy of this object since it shouldn't be
        // restored after being explicitly killed.
        v8::HandleScope handle_scope;
        v8::Persistent<v8::Function>emptyCB;

        //last two args as "" means that we remove the restore script from
        //storage.
        setRestoreScript(mContext,"",emptyCB);
    }
    iStop(false);
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::postDestroy,this,
            livenessToken()),
        "EmersonScript::postDestroy"
    );
}


void EmersonScript::postDestroy(Liveness::Token alive)
{
    //lkjs; FIXME: I feel as though this should be a lock, but then interferes
    //with letDie call in destructors.
    if (!alive)
        return;
    mParent->destroy();
}


void EmersonScript::postCallbackChecks()
{
    EMERSCRIPT_SERIAL_CHECK();
    //if one of the actions that your handler took was to call reset, then reset
    //the entire script.
    if (mResetting)
        resetScript();

    if (mKilling)
        killScript();
}


//called from mainStrand during object initialization
void EmersonScript::onConnected(SessionEventProviderPtr from,
    const SpaceObjectReference& name, HostedObject::PresenceToken token)
{
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iOnConnected,this,
            from,name,token,false,Liveness::livenessToken()),
        "EmersonScript::iOnConnected"
    );
}

void EmersonScript::iOnConnected(SessionEventProviderPtr from,
    const SpaceObjectReference& name, HostedObject::PresenceToken token,
    bool duringInit,Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;


    if (JSObjectScript::mCtx->stopped())
        return;

    while ((!JSObjectScript::mCtx->initialized()) && (! duringInit))
    {}

    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);

    //adding this here because don't want to call onConnected while objStrand is
    //executing.
    EMERSCRIPT_SERIAL_CHECK();
    //register underlying visible manager to listen for proxy creation events on
    //hostedobjectproxymanager
    ProxyManagerPtr proxy_manager = mParent->getProxyManager(name.space(),name.object());
    proxy_manager->addListener(&jsVisMan);
    // Proxies for the object connected are created before this occurs, so we
    // need to manually notify of it:
    ProxyObjectPtr self_proxy = proxy_manager->getProxyObject(name);
    // But we call iOnCreateProxy because we want it to be synchronous
    // and we're already in the correct strand
    jsVisMan.iOnCreateProxy(self_proxy);


    //register for scripting messages from user
    SpaceID space_id = name.space();
    ObjectReference obj_refer = name.object();

    v8::HandleScope handle_scope;

    //register port for messaging
    ODP::Port* msgPort = mParent->bindODPPort(space_id, obj_refer, EMERSON_UNRELIABLE_COMMUNICATION_PORT);
    if (msgPort != NULL)
    {
        mMessagingPortMap[SpaceObjectReference(space_id,obj_refer)] = msgPort;
        msgPort->receive( std::tr1::bind(&EmersonScript::handleScriptCommUnreliable, this, _1, _2, _3));
    }

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

//should be within mainStrand.
void EmersonScript::callbackUnconnected(ProxyObjectPtr proxy, HostedObject::PresenceToken token)
{
    EMERSCRIPT_SERIAL_CHECK();
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
//should only be called from within mStrand
void EmersonScript::requestDisconnect(JSPresenceStruct* jspres)
{
    EMERSCRIPT_SERIAL_CHECK();
    SpaceObjectReference sporef = (jspres->getSporef());
    mParent->disconnectFromSpace(sporef.space(), sporef.object());
}


void EmersonScript::onDisconnected(
    SessionEventProviderPtr from, const SpaceObjectReference& name)
{
    //post message
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iOnDisconnected,this,
            from,name,Liveness::livenessToken()),
        "EmersonScript::iOnDisconnected"
    );
}

//should be called from mStrand
void EmersonScript::iOnDisconnected(
    SessionEventProviderPtr from, const SpaceObjectReference& name,
    Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;

    EMERSCRIPT_SERIAL_CHECK();
    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
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


    // Because of the delay inprocessing, we may not have the presence anymore.
    if (internal_it == mPresences.end())
        return;

    JSPresenceStruct* jspres = internal_it->second;
    if (jspres != NULL)
        jspres->handleDisconnectedCallback();
}

//from mStrand to mainStrand
void EmersonScript::create_entity(EntityCreateInfo& eci)
{
    EMERSCRIPT_SERIAL_CHECK();
    ObjectHost* oh =mParent->getObjectHost();

    //note: calling main strand, not mStrand: want actual connection to happen
    //on mainStrand so that object creation does not interfere with other
    //operations on the oh.
    JSObjectScript::mCtx->mainStrand->post(std::tr1::bind(
            &EmersonScript::eCreateEntityFinish,this,oh,eci),
        "EmersonScript::eCreateEntityFinish"
    );
}

//called from within mainStrand
void EmersonScript::eCreateEntityFinish(ObjectHost* oh,EntityCreateInfo& eci)
{
    HostedObjectPtr obj =
        oh->createObject(eci.scriptType, eci.scriptOpts, eci.scriptContents);

    obj->connect(eci.space,
        eci.loc,
        BoundingSphere3f(Vector3f::zero(), eci.scale),
        eci.mesh,
        eci.physics,
        eci.query
    );
}

EmersonScript::~EmersonScript()
{
    if (Liveness::livenessAlive())
        Liveness::letDie();
}

//called from main strand
void EmersonScript::start() {
    JSObjectScript::start();
}

//called from mainstrand
void EmersonScript::stop()
{
    JSObjectScript::mCtx->stop();
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iStop,this,true),
        "EmersonScript::iStop"
    );
}

//called from mStrand
void EmersonScript::iStop(bool letDie)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (letDie)
        Liveness::letDie();

    
    JSObjectScript::mCtx->stop();
    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);

    for (SimVec::iterator svIt = mSimulations.begin();
         svIt != mSimulations.end(); ++svIt)
    {
        mParent->killSimulation(svIt->second,svIt->first);
    }
    mSimulations.clear();
    
    
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

    JSObjectScript::iStop(letDie);

    mParent->removeListener((SessionEventListener*)this);

    mPresences.clear();

    // Clear out references to visible data. This is currently very
    // important because they hold on to references to ProxyObjects
    // which hold references up to the HostedObject, resulting in
    // circular references that never get cleared. This makes sure
    // they can get cleaned up.
    jsVisMan.clearVisibles();

    // Delete messaging ports
    for(MessagingPortMap::iterator messaging_it = mMessagingPortMap.begin();
        messaging_it != mMessagingPortMap.end();
        messaging_it++)
        delete messaging_it->second;
    mMessagingPortMap.clear();
}

bool EmersonScript::valid() const
{
    return (mParent);
}


//called from mStrand
void EmersonScript::sendMessageToEntityUnreliable(
    const SpaceObjectReference& sporef, const SpaceObjectReference& from,
    const std::string& msgBody)
{
    EMERSCRIPT_SERIAL_CHECK();
    std::map<SpaceObjectReference, ODP::Port*>::iterator iter = mMessagingPortMap.find(from);
    if (iter == mMessagingPortMap.end())
    {
        JSLOG(error,"Trying to send from a sporef that does not exist");
        return;
    }

    ODP::Endpoint dest (sporef.space(),sporef.object(),EMERSON_UNRELIABLE_COMMUNICATION_PORT);
    MemoryReference toSend(msgBody);

    iter->second->send(dest,toSend);
}


Time EmersonScript::getHostedTime()
{
    return mParent->currentLocalTime();
}


v8::Handle<v8::Value> EmersonScript::create_event(
    v8::Persistent<v8::Function>& cb, JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();

    if (mParent->context()->stopped()) {
        JSLOG(warn, "Not creating event because shutdown was requested.");
        return v8::Boolean::New(false);
    }



    /**
       lkjs;
       FIXME: what happens if jscont gets deleted before
       invokeCallbackInContext is called; or suspended in between;
       probably should pass context id through invokeCallbackInContext;
     */
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::invokeCallbackInContext, this,
            livenessToken(), cb, jscont),
        "EmersonScript::invokeCallbackInContext"
    );
    return v8::Boolean::New(true);
}


v8::Handle<v8::Value> EmersonScript::create_timeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared, JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();

    /**
       lkjs;
       FIXME: need to update JSTimerStruct to use JSCtx and objstrand.
     */
    JSTimerStruct* jstimer = new JSTimerStruct(
        this,Duration::seconds(period),cb,jscont,
        contID, timeRemaining,isSuspended,isCleared,
        JSObjectScript::mCtx);

    v8::HandleScope handle_scope;

    //create an object
    v8::Local<v8::Object> localReturner = JSObjectScript::mCtx->mTimerTemplate->NewInstance();

    v8::Persistent<v8::Object> returner = v8::Persistent<v8::Object>::New(localReturner);

    returner->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(jstimer));
    returner->SetInternalField(TYPEID_FIELD, External::New(new String("timer")));

    JSTimerLivenessHolder* jstlh = new JSTimerLivenessHolder(jstimer);
    returner.MakeWeak(jstlh,&JSTimerStruct::timerWeakReferenceCleanup);



    //timer requires a handle to its persistent object so can handle cleanup
    //correctly.
    jstimer->setPersistentObject(returner);
    return handle_scope.Close(localReturner);
}

v8::Handle<v8::Value> EmersonScript::create_timeout(double period, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();
    return create_timeout(period,cb,jscont->getContextID(),0,false,false,jscont);
}


//third arg may be null to evaluate in global context
void EmersonScript::invokeCallbackInContext(
    Liveness::Token alive, v8::Persistent<v8::Function> cb, JSContextStruct* jscontext)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;

    while(!JSObjectScript::mCtx->initialized())
    {}

    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
    v8::HandleScope handle_scope;
    v8::Context::Scope(jscontext->mContext);
    TryCatch try_catch;
    invokeCallback( (jscontext == NULL ? mContext : jscontext), cb);
    postCallbackChecks();
}

//calls funcToCall in jscont, binding jspres bound as first arg.
//mostly used for contexts and presences to execute their callbacks on
//connection and disconnection events
//shoudl be called within mStrand
void EmersonScript::handlePresCallback(
    v8::Handle<v8::Function> funcToCall,JSContextStruct* jscont, JSPresenceStruct* jspres)
{
    EMERSCRIPT_SERIAL_CHECK();

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



void EmersonScript::registerFixupSuspendable(JSSuspendable* jssusp, uint32 contID)
{
    EMERSCRIPT_SERIAL_CHECK();
    toFixup[contID].push_back(jssusp);
}




void EmersonScript::registerContextForClear(JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (mHandlingEvent)
        contextsToClear.push_back(jscont);
    else
        finishContextClear(jscont);
}

void EmersonScript::finishContextClear(JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();

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


//should be called from within mStrand
bool EmersonScript::handleScriptCommRead(
    const SpaceObjectReference& src, const SpaceObjectReference& dst, const String& payload)
{
    if (JSObjectScript::mCtx->stopped())
        return true;

    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iHandleScriptCommRead,this,
            src,dst,payload,Liveness::livenessToken()),
        "EmersonScript::iHandleScriptCommRead"
    );
    return true;
}


void EmersonScript::iHandleScriptCommRead(
    const SpaceObjectReference& src, const SpaceObjectReference& dst,
    const String& payload, Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;

    EMERSCRIPT_SERIAL_CHECK();
    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
    if (JSObjectScript::mCtx->stopped())
        return;


    /**
       lkjs;
       FIXME: May want to check liveness here as well.
     */
    Sirikata::JS::Protocol::JSMessage jsMsg;
    Sirikata::JS::Protocol::JSFieldValue jsFieldVal;
    bool isJSMsg   = jsMsg.ParseFromString(payload);
    if (! isJSMsg)
        isJSMsg = jsMsg.ParseFromArray(payload.data(),payload.size());

    bool isJSField = false;
    if (!isJSMsg)
    {
        isJSField = jsFieldVal.ParseFromString(payload);
        if (!isJSField)
            isJSField = jsFieldVal.ParseFromArray(payload.data(), payload.size());
    }

    //if can't decode the payload as a jsmessage or
    //a jsfieldval, then return false;
    if (!(isJSMsg || isJSField))
        return;

    if (isStopped()) {
        JSLOG(warn, "Ignoring message after shutdown request.");
        // Regardless of whether we can or not, just say we can't decode it.
        return;
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

            //stack maintenance
            mEvalContextStack.push(EvalContext(receiver));
            v8::HandleScope handle_scope;
            v8::Context::Scope context_scope (receiver->mContext);

            v8::Local<v8::Object> msgSender = createVisibleWeakPersistent(
                SpaceObjectReference(dst.space(),dst.object()),
                SpaceObjectReference(src.space(),src.object()),
                JSVisibleDataPtr()
            );

            //try deserialization
            bool deserializeWorks =false;

            std::vector< v8::Persistent<v8::Object> > visiblesToMakeWeak;

            v8::Handle<v8::Value> msgVal;
            if (isJSMsg)
            {
                //try to decode as object.
                msgVal = JSSerializer::deserializeObject( this, jsMsg,
                    deserializeWorks);
            }
            else
            {
                //try to decode as a value.
                msgVal = JSSerializer::deserializeMessage(this,jsFieldVal,
                    deserializeWorks);
            }
            if (! deserializeWorks)
            {
                JSLOG(error, "Deserialization Failed!!");
                mHandlingEvent =false;

                //match removing the context that we appended to stack.
                mEvalContextStack.pop();
                return;
            }

            v8::Handle<v8::Value> argv[3];
            argv[0] =msgVal;
            argv[1] = msgSender;
            argv[2] = v8::String::New (dst.toString().c_str(), dst.toString().size());
            invokeCallback(receiver,receiver->presenceMessageCallback,3,argv);

            //match removing the context that we appended to stack.
            mEvalContextStack.pop();
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

}


/**
   Used for unreliable messages.
 */
void EmersonScript::handleScriptCommUnreliable (
    const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    if (isStopped())
    {
        JSLOG(warn, "Ignoring message after shutdown request.");
        return;
    }

    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::iHandleScriptCommUnreliable,this,
            src,dst,payload,Liveness::livenessToken()),
        "EmersonScript::iHandleScriptCommUnreliable"
    );
}

//called from within mStrand
void EmersonScript::iHandleScriptCommUnreliable(
    const ODP::Endpoint& src, const ODP::Endpoint& dst,
    MemoryReference payload,Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;


    EMERSCRIPT_SERIAL_CHECK();
    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
    if (isStopped())
    {
        JSLOG(warn, "Ignoring message after shutdown request.");
        return;
    }

    SpaceObjectReference to  (dst.space(), dst.object());
    SpaceObjectReference from(src.space(), src.object());
    handleScriptCommRead(from,to,String((const char*) payload.data(), payload.size()));
}

//called from within mStrand
v8::Handle<v8::Value> EmersonScript::sendSandbox(const String& msgToSend, uint32 senderID, uint32 receiverID)
{
    EMERSCRIPT_SERIAL_CHECK();

    //posting task so that still get asynchronous messages.
    JSObjectScript::mCtx->objStrand->post(
        std::tr1::bind(&EmersonScript::processSandboxMessage, this,
            msgToSend,senderID,receiverID,Liveness::livenessToken()),
        "EmersonScript::processSandboxMessage"
    );

    return v8::Undefined();
}

//called from within mStrand
void EmersonScript::processSandboxMessage(
    const String& msgToSend, uint32 senderID, uint32 receiverID,
    Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;

    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
    {
        JSLOG(warn,"Ignoring sandbox message after shutdown request");
        return;
    }

    while(!JSObjectScript::mCtx->initialized())
    {}

    v8::Locker locker (mCtx->mIsolate);
    v8::Isolate::Scope iscope(JSObjectScript::mCtx->mIsolate);
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
        v8::Local<v8::Object> senderObj =JSObjectScript::mCtx->mContextTemplate->NewInstance();
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
    EMERSCRIPT_SERIAL_CHECK();
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
    EMERSCRIPT_SERIAL_CHECK();
    PresenceMapIter pIter = mPresences.find(name);
    if (pIter != mPresences.end()) {
        ProxyManagerPtr proxy_manager = mParent->getProxyManager(name.space(), name.object());
        if (proxy_manager) {
            proxy_manager->removeListener(&jsVisMan);
        }
    }
}



void EmersonScript::removePresenceData(const SpaceObjectReference& sporefToDelete) {
    EMERSCRIPT_SERIAL_CHECK();
    PresenceMapIter pIter = mPresences.find(sporefToDelete);
    if (pIter != mPresences.end())
        mPresences.erase(pIter);

    // Remove the ODP::Port used for unreliable messaging
    MessagingPortMap::iterator messaging_it = mMessagingPortMap.find(sporefToDelete);
    if (messaging_it != mMessagingPortMap.end()) {
        delete messaging_it->second;
        mMessagingPortMap.erase(messaging_it);
    }
}



//takes the c++ object jspres, creates a new visible object out of it, if we
//don't already have a c++ visible object associated with it (if we do, use
//that one), wraps that c++ object in v8, and returns it as a v8 object to
//user
v8::Local<v8::Object> EmersonScript::presToVis(JSPresenceStruct* jspres, JSContextStruct* jscont)
{
    EMERSCRIPT_SERIAL_CHECK();
    JSVisibleStruct* jsvis =
        jsVisMan.createVisStruct(this, jspres->getSporef());
    v8::Local<v8::Object> returner = createVisibleWeakPersistent(jsvis);
    return returner;
}

JSPresenceStruct*  EmersonScript::addConnectedPresence(
    const SpaceObjectReference& sporef,HostedObject::PresenceToken token)
{
    EMERSCRIPT_SERIAL_CHECK();
    JSPresenceStruct* presToAdd =
        new JSPresenceStruct(this, sporef,mContext,token,JSObjectScript::mCtx);

    // Add to our internal map
    mPresences[sporef] = presToAdd;
    return presToAdd;
}


//should be called from something that already has declared a handlescope,
//wraps the presence in a v8 object and returns it.
//should be called within mStrand
v8::Local<v8::Object> EmersonScript::wrapPresence(
    JSPresenceStruct* presToWrap, v8::Persistent<v8::Context>* ctxToWrapIn)
{
    EMERSCRIPT_SERIAL_CHECK();
    v8::HandleScope handle_scope;
    v8::Handle<v8::Context> ctx = (ctxToWrapIn == NULL) ? mContext->mContext : *ctxToWrapIn;
    v8::Context::Scope context_scope(ctx);

    Local<Object> js_pres = JSObjectScript::mCtx->mPresenceTemplate->GetFunction()->NewInstance();
    js_pres->SetInternalField(PRESENCE_FIELD_PRESENCE,External::New(presToWrap));
    js_pres->SetInternalField(TYPEID_FIELD,External::New(new String(PRESENCE_TYPEID_STRING)));

    return handle_scope.Close(js_pres);
}



//called from within mStrand
v8::Handle<v8::Value> EmersonScript::restorePresence(PresStructRestoreParams& psrp,JSContextStruct* jsctx)
{
    EMERSCRIPT_SERIAL_CHECK();
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

    BoundingSphere3f bs = BoundingSphere3f(Vector3f(0,0,0), psrp.scale);

    Location newLoc(newPosD,psrp.orient,psrp.velocity, newAngAxis,newAngVel);


    HostedObject::PresenceToken presToke = incrementPresenceToken();
    JSPresenceStruct* jspres = new JSPresenceStruct(this,psrp,psrp.position,
        presToke,jsctx,tMotVec,tMotQuat,JSObjectScript::mCtx);

    if (psrp.isConnected)
    {
        JSObjectScript::mCtx->mainStrand->post(
            std::tr1::bind(&EmersonScript::mainStrandCompletePresConnect,this,
                newLoc,bs,psrp,presToke,Liveness::livenessToken()),
            "EmersonScript::mainStrandCompletePresConnect"
        );

        mUnconnectedPresences.push_back(jspres);
        return v8::Null();
    }
    //if is unconnected, return presence now.
    v8::HandleScope handle_scope;
    return handle_scope.Close(wrapPresence(jspres,&(jsctx->mContext)));
}



void EmersonScript::mainStrandCompletePresConnect(
    Location newLoc,BoundingSphere3f bs,
    PresStructRestoreParams psrp,HostedObject::PresenceToken presToke,
    Liveness::Token alive)
{
    if (!alive) return;
    Liveness::Lock locked(alive);
    if (!locked) return;

    mParent->connect(psrp.sporef.space(),
        newLoc,
        bs,
        psrp.mesh,
        psrp.physics,
        psrp.query,
        psrp.sporef.object(),
        presToke
    );
}

//This function returns to you the current value of present token and incrmenets
//presenceToken so that get a unique one each time.  If presenceToken is equal
//to default_presence_token, increments one beyond it so that don't start inadvertently
//returning the DEFAULT_PRESENCE_TOKEN;
//called from within mStrand
HostedObject::PresenceToken EmersonScript::incrementPresenceToken()
{
    EMERSCRIPT_SERIAL_CHECK();

    HostedObject::PresenceToken returner = presenceToken++;
    if (returner == HostedObject::DEFAULT_PRESENCE_TOKEN)
        return incrementPresenceToken();

    return returner;
}


void EmersonScript::setLocation(const SpaceObjectReference sporef, const TimedMotionVector3f& loc)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
        return;
    mParent->requestLocationUpdate(sporef.space(), sporef.object(), loc);
}

void  EmersonScript::setOrientation(
    const SpaceObjectReference sporef, const TimedMotionQuaternion& orient)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
        return;
    mParent->requestOrientationUpdate(sporef.space(), sporef.object(), orient);
}

void EmersonScript::setBounds(
    const SpaceObjectReference sporef, const BoundingSphere3f& bnds)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
        return;

    mParent->requestBoundsUpdate(sporef.space(),sporef.object(), bnds);
}

//mesh
void  EmersonScript::setVisual(
    const SpaceObjectReference sporef, const std::string& newMeshString)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
        return;
    mParent->requestMeshUpdate(sporef.space(),sporef.object(),newMeshString);
}

//FIXME: May want to have an error handler for this function.
void EmersonScript::setPhysicsFunction(
    const SpaceObjectReference sporef, const String& newPhyString)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
        return;
    mParent->requestPhysicsUpdate(sporef.space(), sporef.object(), newPhyString);
}


void EmersonScript::setQueryFunction(
    const SpaceObjectReference sporef, const String& query
)
{
    EMERSCRIPT_SERIAL_CHECK();
    if (JSObjectScript::mCtx->stopped())
        return;

    mParent->requestQueryUpdate(
        sporef.space(), sporef.object(), query
    );
}

String EmersonScript::getQuery(const SpaceObjectReference& sporef) const {
    return mParent->requestQuery(sporef.space(),sporef.object());
}

} // namespace JS
} // namespace Sirikata
