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
#include "JSObjectStructs/JSEventHandlerStruct.hpp"
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
   EmersonMessagingManager(ho->context()->ioService),
   mParent(ho),
   mHandlingEvent(false),
   mResetting(false),
   mKilling(false),
   mCreateEntityPort(NULL),
   presenceToken(HostedObject::DEFAULT_PRESENCE_TOKEN +1),
   emHttpPtr(EmersonHttpPtr(new EmersonHttpManager(ho->context()->ioService)))
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
    mEventHandlers.clear();
    mQueuedHandlerEventsAdd.clear();
    mQueuedHandlerEventsDelete.clear();
    mImportedFiles.clear();
    mContext->struct_rootReset();
    flushQueuedHandlerEvents();


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

}

v8::Persistent<v8::Object> EmersonScript::createVisiblePersistent(const SpaceObjectReference& visibleObj,JSProxyData* addParams, v8::Handle<v8::Context> ctx)
{
    JSVisibleStruct* jsvis =  createVisStruct(visibleObj,addParams);
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
   // Invoke user callback
    PresenceMapIter iter = mPresences.find(proxTo);
    if (iter == mPresences.end())
    {
        JSLOG(error,"No presence associated with sporef "<<proxTo<<" exists in presence mapping when getting notifyProximate.  Taking no action.");
        return;
    }

    if (mContext->proxAddedFunc.IsEmpty())
    {
        JSLOG(debug,"No prox added func to execute");
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

    mParent->destroy();
}


void EmersonScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name, HostedObject::PresenceToken token)
{
    //register underlying visible manager to listen for proxy creation events on hostedobjectproxymanager
    mParent->getProxyManager(name.space(),name.object())->addListener(this);
    
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
        callbackUnconnected(name,token);
    }
}


void EmersonScript::callbackUnconnected(const SpaceObjectReference& name, HostedObject::PresenceToken token)
{
    for (PresenceVec::iterator iter = mUnconnectedPresences.begin(); iter != mUnconnectedPresences.end(); ++iter)
    {
        if (token == (*iter)->getPresenceToken())
        {

            JSPresenceStruct* pstruct = *iter;
            mPresences[name] = pstruct;
            //before the presence was connected, didn't have a sporef, and
            //had to set presence's position listener with a blank proxyptr.
            //can now set it with a real one that listens as it moves/changes in
            //the world.
            JSProxyPtr proxPtr  =  createProxyPtr(name, nullProxyPtr);

            mUnconnectedPresences.erase(iter);
            // Make sure this call is last since it invokes a callback which
            // could in turn call connect requests and modify mUnconnectedPresences.
            pstruct->connect(name,proxPtr);
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
    JSPresenceStruct* jspres = findPresence(name);
    //deregister underlying visible manager from listen for proxy creation events on hostedobjectproxymanager
    mParent->getProxyManager(name.space(),name.object())->removeListener(this);

    EmersonMessagingManager::presenceDisconnected(name);
    
    if (jspres != NULL)
        jspres->disconnectCalledFromObjScript();
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
    if (mContext != NULL)
    {
        v8::HandleScope handle_scope;
        mContext->clear();
        delete mContext;
        mContext = NULL;
    }
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



v8::Handle<v8::Value> EmersonScript::create_timeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared, JSContextStruct* jscont)
{
    Network::IOService* ioserve = mParent->getIOService();
    JSTimerStruct* jstimer = new JSTimerStruct(this,Duration::seconds(period),cb,jscont,ioserve,contID, timeRemaining,isSuspended,isCleared);

    v8::HandleScope handle_scope;

    //create an object
    v8::Persistent<v8::Object> returner = v8::Persistent<v8::Object>::New(mManager->mTimerTemplate->NewInstance());
    returner->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(jstimer));
    returner->SetInternalField(TYPEID_FIELD, External::New(new String("timer")));

    returner.MakeWeak(NULL,&JSTimerStruct::timerWeakReferenceCleanup);

    return handle_scope.Close(returner);
}

v8::Handle<v8::Value> EmersonScript::create_timeout(double period, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont)
{
    return create_timeout(period,cb,jscont->getContextID(),0,false,false,jscont);
}


//third arg may be null to evaluate in global context
void EmersonScript::handleTimeoutContext(v8::Persistent<v8::Function> cb, JSContextStruct* jscontext)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope(jscontext->mContext);
    TryCatch try_catch;
    invokeCallback( (jscontext == NULL ? mContext : jscontext), cb);
}

//calls funcToCall in jscont, binding jspres bound as first arg.
//mostly used for contexts and presences to execute their callbacks on
//connection and disconnection events
void EmersonScript::handlePresCallback( v8::Handle<v8::Function> funcToCall,JSContextStruct* jscont, JSPresenceStruct* jspres)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope(jscont->mContext);
    TryCatch try_catch;
    v8::Handle<v8::Value> js_pres =wrapPresence(jspres,&(jscont->mContext));
    invokeCallback(jscont, funcToCall, 1,&js_pres);
}

//tries to add the handler struct to the list of event handlers.
//if am in the middle of processing an event handler, defers the
//addition to after the event has finished.
void EmersonScript::registerHandler(JSEventHandlerStruct* jsehs)
{
    if ( mHandlingEvent)
    {
        //means that we're in the process of handling an event, and therefore
        //cannot push onto the event handlers list.  instead, add it to another
        //vector, which are additional changes to make after we've tried to
        //match all events.
        mQueuedHandlerEventsAdd.push_back(jsehs);
    }
    else
        mEventHandlers.push_back(jsehs);

}


//for debugging
void EmersonScript::printAllHandlers()
{
    std::cout<<"\nDEBUG: printing all handlers\n";
    for (int s=0; s < (int) mEventHandlers.size(); ++s)
        mEventHandlers[s]->printHandler();

    std::cout<<"\n\n\n";
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



bool EmersonScript::handleScriptCommRead(const SpaceObjectReference& src, const SpaceObjectReference& dst, const std::string& payload)
{
    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromString(payload);

    if (! parsed)
        return false;
    
    return deserializeMsgAndDispatch(src,dst,js_msg);
}




bool EmersonScript::deserializeMsgAndDispatch(const SpaceObjectReference& src, const SpaceObjectReference& dst, Sirikata::JS::Protocol::JSMessage js_msg)
{

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext->mContext);
    v8::Local<v8::Object> obj = v8::Object::New();

    v8::Handle<v8::Object> msgSender = createVisiblePersistent(SpaceObjectReference(src.space(),src.object()),NULL,mContext->mContext);

    //try deserialization
    bool deserializeWorks = JSSerializer::deserializeObject( this, js_msg,obj);

    if (! deserializeWorks)
    {
        JSLOG(error, "Deserialization Failed!!");
        return false;
    }

    
    // Checks if matches some handler.  Try to dispatch the message
    bool matchesSomeHandler = false;
    //cannot affect the event handlers when we are executing event handlers.
    mHandlingEvent = true;

    for (int s=0; s < (int) mEventHandlers.size(); ++s)
    {
        if ((mResetting) || (mKilling))
            break;
        
        if (mEventHandlers[s]->matches(obj,src,dst))
        {
            // Adding support for the knowing the message properties too
            int argc = 3;
            Handle<Value> argv[3] = { obj, msgSender, v8::String::New (dst.toString().c_str(), dst.toString().size()) };
            TryCatch try_catch;
            invokeCallback(mContext, NULL, mEventHandlers[s]->cb, argc, argv);

            matchesSomeHandler = true;
        }
    }
    mHandlingEvent = false;
    flushQueuedHandlerEvents();

    //if one of the actions that your handler took was to call reset, then reset
    //the entire script.
    if (mResetting)
        resetScript();

    if (mKilling)
        killScript();

    if (!matchesSomeHandler) {
        JSLOG(detailed,"Message did not match any handler patterns");
    }

    return true;
}


/**
   Used for unreliable messages.
 */
void EmersonScript::handleScriptCommUnreliable (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    SpaceObjectReference to  (dst.space(), dst.object());
    SpaceObjectReference from(src.space(), src.object());
    
    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromArray(payload.data(), payload.size());

    deserializeMsgAndDispatch(from,to,js_msg);
}


//This function takes care of all of the event handling changes that were queued
//while we were trying to match event happenings.
//adds all outstanding changes and then deletes all outstanding in that order.
void EmersonScript::flushQueuedHandlerEvents()
{

    //Adding
    for (int s=0; s < (int)mQueuedHandlerEventsAdd.size(); ++s)
    {
        //add handlers requested to be added during matching of handlers
        mEventHandlers.push_back(mQueuedHandlerEventsAdd[s]);
    }
    mQueuedHandlerEventsAdd.clear();


    //deleting
    for (int s=0; s < (int)mQueuedHandlerEventsDelete.size(); ++s)
    {
        //remove handlers requested to be deleted during matching of handlers
        removeHandler(mQueuedHandlerEventsDelete[s]);
    }


    for (int s=0; s < (int) mQueuedHandlerEventsDelete.size(); ++s)
    {
        //actually delete the patterns
        //have to do this sort of tortured structure with comparing against
        //nulls in order to prevent deleting something twice (a user may have
        //tried to get rid of this handler multiple times).
        if (mQueuedHandlerEventsDelete[s] != NULL)
        {
            deleteHandler(mQueuedHandlerEventsDelete[s]);
            mQueuedHandlerEventsDelete[s] = NULL;
        }
    }
    mQueuedHandlerEventsDelete.clear();
}



void EmersonScript::removeHandler(JSEventHandlerStruct* toRemove)
{
    JSEventHandlerList::iterator iter = mEventHandlers.begin();
    while (iter != mEventHandlers.end())
    {
        if ((*iter) == toRemove)
        {
            (*iter)->clear();
            iter = mEventHandlers.erase(iter);
        }
        else
            ++iter;
    }
}

//takes in an event handler, if not currently handling an event, removes the
//handler from the vector and deletes it.  Otherwise, adds the handler for
//removal and deletion later.
void EmersonScript::deleteHandler(JSEventHandlerStruct* toDelete)
{
    //if the handler is already in the process of being cleared, do not
    //something else will already delete it.  To avoid double-delete, return
    //here.
    if (toDelete->getIsCleared())
        return;

    if (mHandlingEvent)
    {
        mQueuedHandlerEventsDelete.push_back(toDelete);
        return;
    }

    removeHandler(toDelete);
    toDelete->clear();
    delete toDelete;
    toDelete = NULL;
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
    for (PresenceMapIter pIter = mPresences.begin(); pIter != mPresences.end(); ++pIter)
    {
        if (pIter->second == toDelete)
        {
            mPresences.erase(pIter);
            break;
        }
    }

    SpaceObjectReference sporefToDelete = toDelete->getSporef();
    mParent->disconnectFromSpace(sporefToDelete.space(),sporefToDelete.object());
    delete toDelete;
}



//This function takes in a jseventhandler, and wraps a javascript object with
//it.  The function is called by registerEventHandler in JSSystem, which returns
//the js object this function creates to the user.
v8::Handle<v8::Object> EmersonScript::makeEventHandlerObject(JSEventHandlerStruct* evHand, JSContextStruct* jscs)
{
    v8::Handle<v8::Context> ctx = (jscs == NULL) ? mContext->mContext : jscs->mContext;
    v8::Context::Scope context_scope(ctx);
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mHandlerTemplate->NewInstance();

    returner->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD, External::New(evHand));
    returner->SetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD, External::New(this));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String (JSHANDLER_TYPEID_STRING)));

    return handle_scope.Close(returner);
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
    if (jsctx != mContext)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Can only restore presence from root context.")) );

    v8::Context::Scope context_scope(jsctx->mContext);

    //get location
    Vector3f newPos            = psrp.mTmv3f->extrapolate(mParent->currentLocalTime()).position();
    Quaternion newOrient       = psrp.mTmq->extrapolate(mParent->currentLocalTime()).position();
    Vector3f newVel            = psrp.mTmv3f->velocity();
    Quaternion orientVel       = psrp.mTmq->velocity();

    Vector3f newAngAxis;
    float newAngVel;
    orientVel.toAngleAxis(newAngVel,newAngAxis);

    Vector3d newPosD(newPos.x,newPos.y,newPos.z);
    Location newLoc(newPosD,newOrient,newVel, newAngAxis,newAngVel);


    //get bounding sphere
    BoundingSphere3f bs = BoundingSphere3f(newPos, *psrp.mScale);

    HostedObject::PresenceToken presToke = incrementPresenceToken();
    JSPresenceStruct* jspres = new JSPresenceStruct(this,psrp,newPos,presToke,jsctx);

    if (*psrp.mIsConnected)
    {
        mParent->connect(psrp.mSporef->space(),
            newLoc,
            bs,
            *psrp.mMesh,
            "",
            *psrp.mQuery,
            UUID::null(),
            psrp.mSporef->object(),
            presToke);

        mUnconnectedPresences.push_back(jspres);

        return v8::Null();
    }
    //if is unconnected, return presence now.
    v8::HandleScope handle_scope;
    return handle_scope.Close(wrapPresence(jspres,&(jsctx->mContext)));
}


//takes in a string corresponding to the new presence's mesh and a function
//callback to run when the presence is connected.
v8::Handle<v8::Value> EmersonScript::create_presence(const String& newMesh, v8::Handle<v8::Function> callback, JSContextStruct* jsctx, const Vector3d& poser, const SpaceID& spaceToCreateIn)
{
    if (jsctx == NULL)
        jsctx = mContext;

    v8::Context::Scope context_scope(jsctx->mContext);

    //presuming that we are connecting to the same space;
    //arbitrarily saying that we'll just be on top of the root object.
    Location startingLoc(poser,Quaternion::identity(),Vector3f(0,0,0),Vector3f(0,1,0),0);

    //Arbitrarily saying that we're just going to use a simple bounding sphere.
    BoundingSphere3f bs = BoundingSphere3f(Vector3f::nil(), 1);

    HostedObject::PresenceToken presToke = incrementPresenceToken();
    mParent->connect(spaceToCreateIn,startingLoc,bs, newMesh, "", SolidAngle::Max,UUID::null(),ObjectReference::null(),presToke);



    //create a presence object associated with this presence and return it;
    JSPresenceStruct* presToAdd = new JSPresenceStruct(this,callback,jsctx,presToke);

    //v8::Persistent<v8::Object>js_pres = jsctx->addToPresencesArray(presToAdd);
    mUnconnectedPresences.push_back(presToAdd);

    return v8::Undefined();
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
