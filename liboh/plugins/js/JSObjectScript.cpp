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
#include "lexWhenPred/LexWhenPredUtil.h"
#include "emerson/Util.h"
#include "JSSystemNames.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectStructs/JSContextStruct.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSTimerStruct.hpp"
#include "JSObjects/JSObjectsUtils.hpp"
#include "JSObjectStructs/JSWhenStruct.hpp"
#include "JSObjectStructs/JSUtilStruct.hpp"
#include "JSObjectStructs/JSQuotedStruct.hpp"
#include "JSObjectStructs/JSWhenWatchedItemStruct.hpp"
#include "JSObjectStructs/JSWhenWatchedListStruct.hpp"
#include <boost/lexical_cast.hpp>
#include "JSVisibleStructMonitor.hpp"


#define FIXME_GET_SPACE_OREF() \
    HostedObject::SpaceObjRefVec spaceobjrefs;              \
    mParent->getSpaceObjRefs(spaceobjrefs);                 \
    SpaceID space = (spaceobjrefs.begin())->space(); \
    ObjectReference oref = (spaceobjrefs.begin())->object();


using namespace v8;
using namespace std;
namespace Sirikata {
namespace JS {

namespace {

void printException(v8::TryCatch& try_catch) {
    stringstream os;

    v8::HandleScope handle_scope;
    v8::String::Utf8Value exception(try_catch.Exception());
    const char* exception_string = ToCString(exception);
    v8::Handle<v8::Message> message = try_catch.Message();

    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    os << filename_string << ':' << linenum << ": " << exception_string << "\n";
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    os << sourceline_string << "\n";
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++)
        os << " ";
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++)
        os << "^";
    os << "\n";
    v8::String::Utf8Value stack_trace(try_catch.StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      os << stack_trace_string << "\n";
    }
    JSLOG(error, "Uncaught exception:\n" << os.str());
}


v8::Handle<v8::Value> ProtectedJSCallback(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb, int argc, v8::Handle<v8::Value> argv[]) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    Handle<Value> result;
    bool targetGiven = false;
    if (target!=NULL)
    {
        if (((*target)->IsNull() || (*target)->IsUndefined()))
        {
            JSLOG(insane,"ProtectedJSCallback with target given.");
            result = cb->Call(*target, argc, argv);
            targetGiven = true;
        }
    }

    if (!targetGiven)
    {
        JSLOG(insane,"ProtectedJSCallback without target given.");
        result = cb->Call(ctx->Global(), argc, argv);
    }


    if (result.IsEmpty())
    {
        // FIXME what should we do with this exception?
        printException(try_catch);
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Uncaught exception in ProtectedJSCallback.  Result is empty.")) );
    }
    return result;

}



v8::Handle<v8::Value> ProtectedJSCallback(v8::Handle<v8::Context> ctx, v8::Handle<v8::Object> *target, v8::Handle<v8::Function> cb) {
    const int argc =
#ifdef _WIN32
		1
#else
		0
#endif
		;
    Handle<Value> argv[argc] = { };
    return ProtectedJSCallback(ctx, target, cb, argc, argv);
}

}

JSObjectScript::EvalContext::EvalContext()
 : currentScriptDir(),
   currentOutputStream(&std::cout)
{}

JSObjectScript::EvalContext::EvalContext(const EvalContext& rhs)
 : currentScriptDir(rhs.currentScriptDir),
   currentOutputStream(rhs.currentOutputStream)
{}


JSObjectScript::ScopedEvalContext::ScopedEvalContext(JSObjectScript* _parent, const EvalContext& _ctx)
 : parent(_parent)
{
    parent->mEvalContextStack.push(_ctx);
}

JSObjectScript::ScopedEvalContext::~ScopedEvalContext() {
    assert(!parent->mEvalContextStack.empty());
    parent->mEvalContextStack.pop();
}




JSObjectScript::JSObjectScript(HostedObjectPtr ho, const String& args, JSObjectScriptManager* jMan)
 : mParent(ho),
   mManager(jMan),
   presenceToken(HostedObject::DEFAULT_PRESENCE_TOKEN +1),
   hiddenObjectCount(0)
{

    OptionValue* init_script;
    InitializeClassOptions(
        "jsobjectscript", this,
        // Default value allows us to use std libs in the build tree, starting
        // from build/cmake
        init_script = new OptionValue("init-script","",OptionValueType<String>(),"Script to run on startup."),
        NULL
    );

    OptionSet* options = OptionSet::getOptions("jsobjectscript", this);
    options->parse(args);

    // By default, our eval context has:
    // 1. Empty currentScriptDir, indicating it should only use explicitly
    //    specified search paths.
    mEvalContextStack.push(EvalContext());

    v8::HandleScope handle_scope;
    mContext = v8::Context::New(NULL, mManager->mGlobalTemplate);


    Local<Object> global_obj = mContext->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
    global_proto->SetInternalField(0, External::New(this));
    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));
    system_obj->SetInternalField(SYSTEM_TEMPLATE_JSOBJSCRIPT_FIELD,External::New(this));
    system_obj->SetInternalField(TYPEID_FIELD,External::New(new String(SYSTEM_TYPEID_STRING)));


    JSUtilStruct* utilObjStruct = new JSUtilStruct(NULL,this);
    Local<Object> util_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME)));
    util_obj->SetInternalField(UTIL_TEMPLATE_UTILSTRUCT_FIELD,External::New(utilObjStruct));
    util_obj->SetInternalField(TYPEID_FIELD,External::New(new String(UTIL_TYPEID_STRING)));



    //hangs math, presences, and addressable off of system_obj
    initializePresences(system_obj);

    mHandlingEvent = false;


    //Always load the shim layer.
    // This is required. So do NOT remove. It is
    // not the same as libraray
    // TODO: hardcoded
    import("std/shim.em");

    String script_name = init_script->as<String>();
    if (script_name.empty()) {
        JSLOG(info,"Importing default script.");
        import(mManager->defaultScript());
    }
    else {
        JSLOG(info,"Have an initial script to import from " + script_name );
        import(script_name);
    }

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

    mParent->getObjectHost()->persistEntityState(String("scene.persist"));

}

//checks to see if the associated space object reference exists in the script.
//if it does, then make the position listener a subscriber to its pos updates.
bool JSObjectScript::registerPosListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl,TimedMotionVector3f* loc, TimedMotionQuaternion* orient)
{
    ProxyObjectPtr p;
    bool succeeded = false;
    if (sporef ==NULL)
    {
        //trying to set to one of my own presence's postion listeners
        JSLOG(insane,"attempting to register position listener for one of my own presences with sporef "<<*ownPres);
        succeeded = mParent->getProxy(ownPres,p);
    }
    else
    {
        //trying to get a non-local proxy object
        JSLOG(insane,"attempting to register position listener for a visible object with sporef "<<*sporef);
        succeeded = mParent->getProxyObjectFrom(ownPres,sporef,p);
    }

    //if actually had associated proxy object, then update loc and orientation.
    if (succeeded)
    {
        p->PositionProvider::addListener(pl);
        if (loc != NULL)
            *loc = p->getTimedMotionVector();
        if (orient != NULL)
            *orient = p->getTimedMotionQuaternion();
    }
    else
        JSLOG(error,"error registering to be a position listener. could not find associated object in hosted object.");

    return succeeded;

}



//deregisters position listening for an arbitrary proxy object visible to
//ownPres and with spaceobject reference sporef.
bool JSObjectScript::deRegisterPosListener(SpaceObjectReference* sporef, SpaceObjectReference* ownPres,PositionListener* pl)
{
    ProxyObjectPtr p;
    bool succeeded = false;

    if (sporef == NULL)
    {
        //de-regestering pl from position listening to one of my own presences.
        JSLOG(insane,"attempting to de-register position listener for one of my presences with sporef "<<*ownPres);
        succeeded = mParent->getProxy(ownPres,p);
    }
    else
    {
        //de-registering pl from position listening to an arbitrary proxy object
        JSLOG(insane,"attempting to de-register position listener for visible object with sporef "<<*sporef);
        succeeded =  mParent->getProxyObjectFrom(ownPres,sporef,p);
    }

    if (succeeded)
        p->PositionProvider::removeListener(pl);
    else
        JSLOG(error,"error de-registering to be a position listener.  could not find associated object in hosted object.");

    return succeeded;

}




v8::Handle<v8::Value> JSObjectScript::createWhenWatchedItem(v8::Handle<v8::Array> itemArray)
{
    v8::HandleScope handle_scope;

    if (itemArray->Length() == 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in ScriptCreateWhenWatchedItem: requires a single argument (an array of strings) that lists a variable's name.  For instance, var x.y.z would give array ['x','y','z']")));


    JSWhenWatchedItemStruct * wwis = new JSWhenWatchedItemStruct(itemArray,this);
    return createWhenWatchedItem(wwis);
}

v8::Handle<v8::Value> JSObjectScript::createWhenWatchedItem(JSWhenWatchedItemStruct* wwis)
{
    v8::HandleScope handle_scope;

    v8::Local<v8::Object> whenWatchedItemObj_local = mManager->mWhenWatchedItemTemplate->NewInstance();
    v8::Persistent<v8::Object> whenWatchedItemObj  = v8::Persistent<v8::Object>::New(whenWatchedItemObj_local);

    whenWatchedItemObj->SetInternalField(TYPEID_FIELD,v8::External::New(new String(WHEN_WATCHED_ITEM_TYPEID_STRING)));
    whenWatchedItemObj->SetInternalField(WHEN_WATCHED_ITEM_TEMPLATE_FIELD,v8::External::New(wwis));

    return whenWatchedItemObj;
}


JSObjectScript* JSObjectScript::decodeSystemObject(v8::Handle<v8::Value> toDecode, String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of system object in JSObjectScript.cpp.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != SYSTEM_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of system object in JSObjectScript.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsObjectScript field
    v8::Local<v8::External> wrapJSObjScript;
    wrapJSObjScript = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(SYSTEM_TEMPLATE_JSOBJSCRIPT_FIELD));
    void* ptr = wrapJSObjScript->Value();

    JSObjectScript* returner;
    returner = static_cast<JSObjectScript*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of system object in JSObjectScript.cpp.  Internal field of object given cannot be casted to a JSObjectScript.";

    return returner;
}


//this is the callback that fires when proximateObject no longer receives
//updates from loc (ie the object in the world associated with proximate object
//is outside of querier's standing query registered to pinto).
void  JSObjectScript::notifyProximateGone(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{
    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" went out of query of "<<querier<<".  Mostly just ignoring it.");

    //notifies the underlying struct associated with this object (if any exist)
    //that the proxy object is no longer visible.
    JSVisibleStructMonitor::checkNotifyNowNotVis(proximateObject->getObjectReference(),querier);

    PresenceMapIter iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"Error.  Received a notification that a proximate object left query set for querier "<<querier<<".  However, querier has no associated presence in array.  Aborting now");
        return;
    }

    //now see if must issue callback for this object's going out of scope
    if ( !iter->second->mOnProxRemovedEventHandler.IsEmpty() && !iter->second->mOnProxRemovedEventHandler->IsUndefined() && !iter->second->mOnProxRemovedEventHandler->IsNull())
    {
        JSVisibleStruct* jsvis =  checkVisStructExists(proximateObject->getObjectReference(),querier);
        if (jsvis == NULL)
        {
            JSLOG(error, "Error in notifyProximateGone of JSObjectScript.  Before receiving a notification that an object is no longer visible, should have received a notification that it was originally visible.  Error on object: "<<proximateObject->getObjectReference()<<" for querier: "<<querier<<".  Aborting call now.");
            return;
        }


        //jswrap the object
        //should be in context from createVisibleObject call
        v8::Handle<v8::Object> outOfRangeObject = createVisibleObject(jsvis,mContext);

        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(mContext);

        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { outOfRangeObject };
        //FIXME: Potential memory leak: when will removedProxObj's
        //SpaceObjectReference field be garbage collected and deleted?
        JSLOG(info,"Issuing user callback for proximate object gone.  Argument passed");
        ProtectedJSCallback(mContext, NULL, iter->second->mOnProxRemovedEventHandler, argc, argv);
    }
}


//creates a js object associated with the jsvisiblestruct
//will enter and exit the context passed in to make the object before returning
v8::Local<v8::Object> JSObjectScript::createVisibleObject(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn)
{
    v8::HandleScope handle_scope;
    ctxToCreateIn->Enter();

    v8::Local<v8::Object> returner = mManager->mVisibleTemplate->NewInstance();
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(jsvis));
    returner->SetInternalField(TYPEID_FIELD,v8::External::New(new String(VISIBLE_TYPEID_STRING)));


    //v8::Persistent<v8::Object> returnerPers = v8::Persistent<v8::Object>::New(returner);
    ctxToCreateIn->Exit();

    return returner;
}

//attempts to make a new jsvisible struct...may be returned an existing one.
//then wraps it as v8 object.
v8::Persistent<v8::Object> JSObjectScript::createVisiblePersistent(const SpaceObjectReference& visibleObj,const SpaceObjectReference& visibleTo,bool isVisible, v8::Handle<v8::Context> ctx)
{
    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this, visibleObj, visibleTo, isVisible);
    return createVisiblePersistent(jsvis,ctx);
}

v8::Persistent<v8::Object> JSObjectScript::createVisiblePersistent(JSVisibleStruct* jsvis, v8::Handle<v8::Context> ctxToCreateIn)
{
    v8::HandleScope handle_scope;
    ctxToCreateIn->Enter();

    v8::Local<v8::Object> returner = mManager->mVisibleTemplate->NewInstance();
    returner->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD,v8::External::New(jsvis));
    returner->SetInternalField(TYPEID_FIELD,v8::External::New(new String(VISIBLE_TYPEID_STRING)));


    v8::Persistent<v8::Object> returnerPers = v8::Persistent<v8::Object>::New(returner);

    ctxToCreateIn->Exit();

    return returnerPers;
}


//attempts to make a new jsvisible struct...may be returned an existing one.
//then wraps it as v8 object.
v8::Local<v8::Object> JSObjectScript::createVisibleObject(const SpaceObjectReference& visibleObj,const SpaceObjectReference& visibleTo,bool isVisible, v8::Handle<v8::Context> ctx)
{
    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this, visibleObj, visibleTo, isVisible);
    return createVisibleObject(jsvis,ctx);
}


//if can't find visible, will just return self object
//this is a mess of a function to get things to work again.
//this function will actually need to be super-cleaned up
v8::Handle<v8::Value> JSObjectScript::findVisible(const SpaceObjectReference& proximateObj)
{
    //return createVisibleObject(proximateObj,SpaceObjectReference::null(),false,mContext);
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    String errorMessage = "Error decoding visible object struct associated with self.  ";
    v8::Handle<v8::Object> sysObj = getSystemObject();
    v8::Local<v8::Value> self_obj = sysObj->Get(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME));
    JSVisibleStruct* self_vis = JSVisibleStruct::decodeVisible(self_obj,errorMessage);

    if (self_vis ==NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));


    JSVisibleStruct* jsvis = JSVisibleStructMonitor::checkVisStructExists(proximateObj,*(self_vis->getToListenTo()));

    if (jsvis != NULL)
    {
        v8::Persistent<v8::Object> returnerPers =createVisiblePersistent(jsvis, mContext);
        return returnerPers;
    }

    //otherwise just return self
    return self_obj;
    //createVisibleObject(self_vis, mContext);
}




//Gets called by notifier when PINTO states that proximateObject originally
//satisfies the solid angle query registered by querier
void  JSObjectScript::notifyProximate(ProxyObjectPtr proximateObject, const SpaceObjectReference& querier)
{

    JSLOG(detailed,"Notified that object "<<proximateObject->getObjectReference()<<" is within query of "<<querier<<".");
    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this, proximateObject->getObjectReference(), querier, true);

    // Invoke user callback
    PresenceMapIter iter = mPresences.find(querier);
    if (iter == mPresences.end())
    {
        JSLOG(error,"No presence associated with sporef "<<querier<<" exists in presence mapping when getting notifyProximate.  Taking no action.");
        return;
    }


    //check callbacks
    if ( !iter->second->mOnProxAddedEventHandler.IsEmpty() && !iter->second->mOnProxAddedEventHandler->IsUndefined() && !iter->second->mOnProxAddedEventHandler->IsNull())
    {
        v8::Handle<v8::Object> newVisibleObj = createVisiblePersistent(jsvis, mContext);

        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(mContext);

        int argc = 1;
        v8::Handle<v8::Value> argv[1] = { newVisibleObj };
        //FIXME: Potential memory leak: when will newAddrObj's
        //SpaceObjectReference field be garbage collected and deleted?
        JSLOG(info,"Issuing user callback for proximate object.");
        ProtectedJSCallback(mContext, NULL, iter->second->mOnProxAddedEventHandler, argc, argv);
    }
}


JSInvokableObject::JSInvokableObjectInt* JSObjectScript::runSimulation(const SpaceObjectReference& sporef, const String& simname)
{
    TimeSteppedSimulation* sim = mParent->runSimulation(sporef,simname);

    return new JSInvokableObject::JSInvokableObjectInt(sim);
}




void JSObjectScript::onConnected(SessionEventProviderPtr from, const SpaceObjectReference& name, int token)
{
    //register for scripting messages from user
    SpaceID space_id = name.space();
    ObjectReference obj_refer = name.object();

    v8::HandleScope handle_scope;


    Handle<Object> system_obj = getSystemObject();

    //register port for messaging
    mMessagingPort = mParent->bindODPPort(space_id, obj_refer, Services::COMMUNICATION);
    if (mMessagingPort)
        mMessagingPort->receive( std::tr1::bind(&JSObjectScript::handleCommunicationMessageNewProto, this, _1, _2, _3));

    mCreateEntityPort = mParent->bindODPPort(space_id,obj_refer, Services::CREATE_ENTITY);


    // For some reason, this *must* come before addPresence(). It can also be
    // called anywhere *inside* addPresence, so there must be some issue with
    // the Context::Scope or HandleScope.
    addSelfField(name);



    //check for callbacks associated with presence connection

    //means that this is the first presence that has been added to the space
    if (token == HostedObject::DEFAULT_PRESENCE_TOKEN)
    {
        // Add to system.presences array
        v8::Handle<v8::Object> new_pres = addConnectedPresence(name,token);

        // Invoke user callback
        if ( !mOnPresenceConnectedHandler.IsEmpty() && !mOnPresenceConnectedHandler->IsUndefined() && !mOnPresenceConnectedHandler->IsNull() )
        {
            int argc = 1;
            v8::Handle<v8::Value> argv[1] = { new_pres };
            ProtectedJSCallback(mContext, NULL, mOnPresenceConnectedHandler, argc, argv);
        }
    }
    else
    {
        //means that we've connected a presence and should finish by calling
        //connection callback
        callbackUnconnected(name,token);
    }
}


void JSObjectScript::callbackUnconnected(const SpaceObjectReference& name, int token)
{

    for (PresenceVec::iterator iter = mUnconnectedPresences.begin(); iter != mUnconnectedPresences.end(); ++iter)
    {
        if (token == (*iter)->getPresenceToken())
        {
            mPresences[name] = *iter;
            (*iter)->connect(name);
            mUnconnectedPresences.erase(iter);
            return;
        }
    }
    JSLOG(error,"Error, received a finished connection with token "<<token<<" that we do not have an unconnected presence struct for.");
}


void JSObjectScript::addSelfField(const SpaceObjectReference& myName)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    JSLOG(info,"Adding self field with sporef "<<myName<<" to world");
    v8::Handle<v8::Object> selfVisObj = createVisiblePersistent(myName, myName, true,mContext);

    v8::Handle<v8::Object> sysObj = getSystemObject();
    sysObj->Set(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME), selfVisObj);
}



void JSObjectScript::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    v8::Handle<v8::Object> sysObj = getSystemObject();
    // Remove Self variable
    if (sysObj->Has(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME)))
        sysObj->Delete(v8::String::New(JSSystemNames::VISIBLE_SELF_NAME));


    // Remove from system.presences array
    removePresence(name);

    //FIXME: need to have separate disconnection handlers for each presence.

    // FIXME this should get the presence but its already been deleted
    if ( !mOnPresenceDisconnectedHandler.IsEmpty() && !mOnPresenceDisconnectedHandler->IsUndefined() && !mOnPresenceDisconnectedHandler->IsNull() )
        ProtectedJSCallback(mContext, NULL, mOnPresenceDisconnectedHandler);
}




void JSObjectScript::create_entity(EntityCreateInfo& eci)
{
    FIXME_GET_SPACE_OREF();

    HostedObjectPtr obj = HostedObject::construct<HostedObject>(mParent->context(), mParent->getObjectHost(), UUID::random());
    if (eci.scriptType != "")
        obj->initializeScript(eci.scriptType, eci.scriptOpts);


    obj->connect(space,
        eci.loc,
        BoundingSphere3f(Vector3f::nil(), eci.scale),
        eci.mesh,
        eci.solid_angle,
        UUID::null(),
        NULL
    );
}


void JSObjectScript::reboot()
{
  // Need to delete the existing context? v8 garbage collects?

  v8::HandleScope handle_scope;
  mContext = v8::Context::New(NULL, mManager->mGlobalTemplate);
  Local<Object> global_obj = mContext->Global();
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  global_proto->SetInternalField(0, External::New(this));
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));

  populateSystemObject(system_obj);

  //delete all handlers
  for (int s=0; s < (int) mEventHandlers.size(); ++s)
      delete mEventHandlers[s];

  mEventHandlers.clear();

}

void JSObjectScript::debugPrintString(std::string cStrMsgBody) const
{
    JSLOG(debug,"Is it working: " << cStrMsgBody);
}


JSObjectScript::~JSObjectScript()
{
    for(JSEventHandlerList::iterator handler_it = mEventHandlers.begin(); handler_it != mEventHandlers.end(); handler_it++)
        delete *handler_it;

    mEventHandlers.clear();

    mContext.Dispose();
}


bool JSObjectScript::valid() const
{
    return (mParent);
}




void JSObjectScript::sendMessageToEntity(SpaceObjectReference* sporef, SpaceObjectReference* from, const std::string& msgBody) const
{
    ODP::Endpoint dest (sporef->space(),sporef->object(),Services::COMMUNICATION);
    MemoryReference toSend(msgBody);

    mMessagingPort->send(dest,toSend);
}

//take in whenPredAsString that should look something like: " x<3 && y > 2"
//and then translates to [ util.create_when_watched_item(['x']), util.create_when_watched_item(['y'])  ]
String JSObjectScript::tokenizeWhenPred(const String& whenPredAsString)
{
    //even though using lexWhenPred grammar, still must make call to emerson init
    emerson_init();
    return string(lexWhenPred_compile(whenPredAsString.c_str()));
}


v8::Handle<v8::Value> JSObjectScript::createWhenWatchedList(std::vector<JSWhenWatchedItemStruct*> wwisVec)
{
    JSWhenWatchedListStruct* jswwl = new JSWhenWatchedListStruct(wwisVec,this);

    v8::HandleScope handle_scope;


    v8::Handle<v8::Object> whenWatchedList = mManager->mWhenWatchedListTemplate->NewInstance();
    whenWatchedList->SetInternalField(TYPEID_FIELD,v8::External::New(new String(WHEN_WATCHED_LIST_TYPEID_STRING)));
    whenWatchedList->SetInternalField(WHEN_WATCHED_LIST_TEMPLATE_FIELD,v8::External::New(jswwl));

    return whenWatchedList;
}


Time JSObjectScript::getHostedTime()
{
    return mParent->currentLocalTime();
}

//Will compile and run code in the context ctx whose source is em_script_str.
v8::Handle<v8::Value>JSObjectScript::internalEval(v8::Persistent<v8::Context>ctx, const String& em_script_str, v8::ScriptOrigin* em_script_name)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    // Special casing emerson compilation
    #ifdef EMERSON_COMPILE

    String em_script_str_new = em_script_str;

    if(em_script_str.empty())
        return v8::Undefined();

    if(em_script_str.at(em_script_str.size() -1) != '\n')
    {
        em_script_str_new.push_back('\n');
    }

    emerson_init();

    String js_script_str = string(emerson_compile(em_script_str_new.c_str()));
    JSLOG(insane, " Compiled JS script = \n" <<js_script_str);



    v8::Handle<v8::String> source = v8::String::New(js_script_str.c_str(), js_script_str.size());
    #else


    //assume the input string to be a valid js rather than emerson
    v8::Handle<v8::String> source = v8::String::New(em_script_str.c_str(), em_script_str.size());

    #endif


    //v8::Handle<v8::String> source = v8::String::New(em_script_str.c_str(), em_script_str.size());


    // Compile
    //note, because using compile command, will run in the mContext context
    v8::Handle<v8::Script> script = v8::Script::Compile(source, em_script_name);
    if (try_catch.HasCaught()) {
        v8::String::Utf8Value error(try_catch.Exception());
        String uncaught( *error);
        uncaught = "Uncaught excpetion " + uncaught + "\nwhen trying to run script: "+ em_script_str;
        JSLOG(error, uncaught);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(uncaught.c_str())));
    }


    if (script.IsEmpty()) {
        v8::String::Utf8Value error(try_catch.Exception());
        std::string msg = std::string("Compile error: ") + std::string(*error);
        JSLOG(error, msg);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(msg.c_str())) );
    }


    TryCatch try_catch2;
    // Execute
    v8::Handle<v8::Value> result = script->Run();

    if (try_catch2.HasCaught()) {
        printException(try_catch2);
        return try_catch2.Exception();
    }


    if (!result.IsEmpty() && !result->IsUndefined()) {
        v8::String::AsciiValue ascii(result);
        JSLOG(detailed, "Script result: " << *ascii);
    }


    checkWhens();
    return result;
}


//there's sort of an open question as to whether one when condition should be
//able to trigger another.  As structured, the answer is yes.  May do something
//to prevent this.
//this function runs through all the when statements associated with script, it
//checks their predicates and runs them if the predicates evaluate to true.
void JSObjectScript::checkWhens()
{
    for (WhenMapIter iter = mWhens.begin(); iter!= mWhens.end(); ++iter)
        iter->first->checkPredAndRun();
}

//defaults to internalEvaling with mContext, and does a ScopedEvalContext.
v8::Handle<v8::Value> JSObjectScript::protectedEval(const String& em_script_str, v8::ScriptOrigin* em_script_name, const EvalContext& new_ctx)
{
    ScopedEvalContext sec(this, new_ctx);
    return internalEval(mContext, em_script_str, em_script_name);
}



/*
  This function takes the string associated with cb, re-compiles it in the
  passed-in context, and then passes it back out.  (Note, enclose the function
  string in parentheses and semi-colon to get around v8 idiosyncracy associated with
  compiling anonymous functions.  Ie, if didn't add those in, it may not compile
  anonymous functions.)
 */
v8::Handle<v8::Value> JSObjectScript::compileFunctionInContext(v8::Persistent<v8::Context>ctx, v8::Handle<v8::Function>&cb)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;

    String errorMessage= "Cannot interpret callback function as string while executing in context.  ";
    String v8Source;
    bool stringDecodeSuccessful = decodeString(cb->ToString(),v8Source,errorMessage);
    if (! stringDecodeSuccessful)
    {
        JSLOG(error, errorMessage);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
    }

    v8Source = "(" + v8Source;
    v8Source += ");";

    ScriptOrigin cb_origin = cb->GetScriptOrigin();
    v8::Handle<v8::Value> compileFuncResult = internalEval(ctx, v8Source, &cb_origin);

    if (! compileFuncResult->IsFunction())
    {
        String errorMessage = "Uncaught exception: function passed in did not compile to a function";
        JSLOG(error, errorMessage);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
    }

    //check if any errors occurred during compile.
    if (! try_catch.HasCaught())
    {
        String exception;
        String errorMessage = "  Error in CompileFunctionInContext.  Could not decode the exception as string when compiling function.  ";
        bool decodedException = decodeString(try_catch.Exception(), exception, errorMessage);
        if (! decodedException)
            JSLOG(error, errorMessage);

        String returnedError = "Uncaught exception when compiling function in context.  " + exception;
        JSLOG(error, returnedError);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(returnedError.c_str(), returnedError.length())));
    }

    JSLOG(insane, "Successfully compiled function in context.  Passing back function object.");
    return compileFuncResult;
}

/*
  This function grabs the string associated with cb, and recompiles it in the
  context ctx.  It then calls the newly recompiled function from within ctx with
  args specified by argv and argc.
 */
v8::Handle<v8::Value> JSObjectScript::ProtectedJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Object>* target, v8::Handle<v8::Function>& cb, int argc, v8::Handle<v8::Value> argv[])
{

    v8::Handle<v8::Value> compiledFunc = compileFunctionInContext(ctx,cb);
    if (! compiledFunc->IsFunction())
    {
        JSLOG(error, "Callback did not compile to a function in ProtectedJSFunctionInContext.  Aborting execution.");
        //will be error message, or whatever else it compiled to.
        return compiledFunc;
    }

    v8::Handle<v8::Function> funcInCtx = v8::Handle<v8::Function>::Cast(compiledFunc);
    return executeJSFunctionInContext(ctx,funcInCtx,argc,target,argv);
}


/*
  This function takes the result of compileFunctionInContext, and executes it
  within context ctx.
 */
v8::Handle<v8::Value> JSObjectScript::executeJSFunctionInContext(v8::Persistent<v8::Context> ctx, v8::Handle<v8::Function> funcInCtx,int argc, v8::Handle<v8::Object>*target, v8::Handle<v8::Value> argv[])
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx);

    TryCatch try_catch;


    v8::Handle<v8::Value> result;
    bool targetGiven = false;
    if (target!=NULL)
    {
        if (((*target)->IsNull() || (*target)->IsUndefined()))
        {
            JSLOG(insane,"ProtectedJSCallback with target given.");
            result = funcInCtx->Call(*target, argc, argv);
            targetGiven = true;
        }
    }

    if (!targetGiven)
    {
        JSLOG(insane,"ProtectedJSCallback without target given.");
        result = funcInCtx->Call(ctx->Global(), argc, argv);
    }


    //check if any errors have occurred.
    if (! try_catch.HasCaught())
    {
        String exception;
        String errorMessage = "  Error in executeJSFunctionInContext.  Could not decode the exception as string when compiling function.  ";
        bool decodedException = decodeString(try_catch.Exception(), exception, errorMessage);
        if (! decodedException)
            JSLOG(error, errorMessage);

        String returnedError = "Uncaught exception when compiling function in context.  " + exception;
        JSLOG(error, returnedError);
        return v8::ThrowException( v8::Exception::Error(v8::String::New(returnedError.c_str(), returnedError.length())));
    }

    return result;
}



/*
  executeInContext takes in a context, that you want to execute the function
  funcToCall in.  argv are the arguments to funcToCall from the current context,
  and are counted by argc.
 */
v8::Handle<v8::Value>JSObjectScript::executeInContext(v8::Persistent<v8::Context> &contExecIn, v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    JSLOG(insane, "executing script in alternate context");

    std::vector<v8::Handle<v8::Context> >contextVec;
    while(v8::Context::InContext())
    {
        JSLOG(insane, "peeling away previous context I was in.");
        contextVec.push_back(v8::Context::GetCurrent());
        v8::Context::GetCurrent()->Exit();
    }


    //entering new context associated with
    JSLOG(insane, "entering new context associated with JSContextStruct.");
    contExecIn->Enter();


    JSLOG(insane, "Evaluating function in context associated with JSContextStruct.");
    ProtectedJSFunctionInContext(contExecIn, NULL,funcToCall, argc, argv);


    JSLOG(insane, "Exiting new context associated with JSContextStruct.");
    contExecIn->Exit();


    //restore previous contexts
    std::vector<v8::Handle<v8::Context> >::reverse_iterator revIt;
    for (revIt= contextVec.rbegin(); revIt != contextVec.rend(); ++revIt)
    {
        JSLOG(insane, "restoring previous context I was in.");
        (*revIt)->Enter();
    }

    JSLOG(insane, "execution in alternate context complete");
    return v8::Undefined();
}




void JSObjectScript::print(const String& str) {
    assert(!mEvalContextStack.empty());
    std::ostream* os = mEvalContextStack.top().currentOutputStream;
    assert(os != NULL);
    (*os) << str;
}


v8::Handle<v8::Value> JSObjectScript::create_timeout(const Duration& dur, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb,JSContextStruct* jscont)
{
    //create timerstruct
    Network::IOService* ioserve = mParent->getIOService();
    JSTimerStruct* jstimer = new JSTimerStruct(this,dur,target,cb,jscont,ioserve);

    v8::HandleScope handle_scope;

    //create an object
    v8::Handle<v8::Object> returner  = mManager->mTimerTemplate->NewInstance();

    returner->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(jstimer));
    returner->SetInternalField(TYPEID_FIELD, External::New(new String("timer")));


    return returner;
}




//third arg may be null to evaluate in global context
v8::Handle<v8::Value> JSObjectScript::handleTimeoutContext(v8::Handle<v8::Object> target, v8::Persistent<v8::Function> cb,JSContextStruct* jscontext)
{
    if (jscontext == NULL)
        return ProtectedJSCallback(mContext, &target, cb);


    return ProtectedJSCallback(jscontext->mContext, &target, cb);
}


//v8::Handle<v8::Value>
//JSObjectScript::handleTimeoutContext(v8::Persistent<v8::Function>
//cb,JSContextStruct* jscontext)
v8::Handle<v8::Value> JSObjectScript::handleTimeoutContext(v8::Persistent<v8::Function> cb,v8::Handle<v8::Context>* jscontext)
{
    if (jscontext == NULL)
        return ProtectedJSCallback(mContext, NULL,cb);

    return ProtectedJSCallback(*jscontext,NULL, cb);
}



v8::Handle<v8::Value> JSObjectScript::eval(const String& contents, v8::ScriptOrigin* origin) {
    EvalContext& ctx = mEvalContextStack.top();
    EvalContext new_ctx(ctx);
    return protectedEval(contents, origin, new_ctx);
}

v8::Handle<v8::Value> JSObjectScript::import(const String& filename) {
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    using namespace boost::filesystem;

    // Search through the import paths to find the file to import, searching the
    // current directory first if it is non-empty.
    path full_filename;
    path filename_as_path(filename);
    assert(!mEvalContextStack.empty());
    EvalContext& ctx = mEvalContextStack.top();
    if (!ctx.currentScriptDir.empty()) {
        path fq = ctx.currentScriptDir / filename_as_path;
        if (boost::filesystem::exists(fq))
            full_filename = fq;
    }
    if (full_filename.empty()) {
        std::list<String> search_paths = mManager->getOptions()->referenceOption("import-paths")->as< std::list<String> >();
        // Always search the current directory as a last resort
        search_paths.push_back(".");
        for (std::list<String>::iterator pit = search_paths.begin(); pit != search_paths.end(); pit++) {
            path fq = path(*pit) / filename_as_path;
            if (boost::filesystem::exists(fq)) {
                full_filename = fq;
                break;
            }
        }
    }

    // If we still haven't filled this in, we just can't find the file.
    if (full_filename.empty())
    {
        std::string errorMessage("Couldn't find file for import named");
        errorMessage+=filename;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())) );
    }

    // Now try to read in and run the file.
    FILE * pFile;
    long lSize;
    char * buffer;
    long result;

    pFile = fopen (full_filename.string().c_str(), "r" );
    if (pFile == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Couldn't open file for import.")) );

    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);

    std::string contents(lSize, '\0');

    result = fread (&(contents[0]), 1, lSize, pFile);
    fclose (pFile);

    if (result != lSize)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Failure reading file for import.")) );

    EvalContext new_ctx(ctx);
    new_ctx.currentScriptDir = full_filename.parent_path().string();
    ScriptOrigin origin(v8::String::New(filename.c_str()));
    return protectedEval(contents, &origin, new_ctx);
}




// need to ensure that the sender object is an visible of type spaceobject reference rather than just having an object reference;
JSEventHandlerStruct* JSObjectScript::registerHandler(const PatternList& pattern, v8::Persistent<v8::Object>& target, v8::Persistent<v8::Function>& cb, v8::Persistent<v8::Object>& sender)
{
    JSEventHandlerStruct* new_handler= new JSEventHandlerStruct(pattern, target, cb,sender);

    if ( mHandlingEvent)
    {
        //means that we're in the process of handling an event, and therefore
        //cannot push onto the event handlers list.  instead, add it to another
        //vector, which are additional changes to make after we've tried to
        //match all events.
        mQueuedHandlerEventsAdd.push_back(new_handler);
    }
    else
        mEventHandlers.push_back(new_handler);

    return new_handler;
}


//for debugging
void JSObjectScript::printAllHandlerLocations()
{
    std::cout<<"\nPrinting event handlers for "<<this<<": \n";
    for (int s=0; s < (int) mEventHandlers.size(); ++s)
        std::cout<<"\t"<<mEventHandlers[s];

    std::cout<<"\n\n\n";
}




/*
 * From the odp::endpoint & src and destination, checks if the corresponding
 * visible object already existed in the visible array.  If it does, return the
 * associated visible object.  If it doesn't, then return a new visible object
 * with stillVisible false associated with this object.
 */
v8::Handle<v8::Object> JSObjectScript::getMessageSender(const ODP::Endpoint& src, const ODP::Endpoint& dst)
{
    SpaceObjectReference from(src.space(), src.object());
    SpaceObjectReference to  (dst.space(), dst.object());

    JSVisibleStruct* jsvis = JSVisibleStructMonitor::createVisStruct(this,from,to,false);
    v8::Handle<v8::Object> returner =createVisiblePersistent(jsvis, mContext);

    return returner;
}


void JSObjectScript::handleCommunicationMessageNewProto (const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Object> obj = v8::Object::New();


    v8::Handle<v8::Object> msgSender = getMessageSender(src,dst);
    //try deserialization

    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromArray(payload.data(), payload.size());

   //bftm:clean all this up later

    if (! parsed)
    {
        JSLOG(error,"Cannot parse the message that I received on this port");
        assert(false);
    }

    bool deserializeWorks = JSSerializer::deserializeObject( this, js_msg,obj);

    if (! deserializeWorks)
    {
        JSLOG(error, "Deserialization Failed!!");
        return;
    }

    // Checks if matches some handler.  Try to dispatch the message
    bool matchesSomeHandler = false;


    //cannot affect the event handlers when we are executing event handlers.
    mHandlingEvent = true;

    for (int s=0; s < (int) mEventHandlers.size(); ++s)
    {
        if (mEventHandlers[s]->matches(obj,msgSender))
        {
            // Adding support for the knowing the message properties too
            int argc = 2;

            Handle<Value> argv[2] = { obj, msgSender };
            ProtectedJSCallback(mContext, &mEventHandlers[s]->target, mEventHandlers[s]->cb, argc, argv);

            matchesSomeHandler = true;
        }
    }
    mHandlingEvent = false;
    flushQueuedHandlerEvents();

    /*
      FIXME: What should I do if the message that I receive does not match any handler?
     */
    if (!matchesSomeHandler) {
        JSLOG(info,"Message did not match any files");
    }
}



//This function takes care of all of the event handling changes that were queued
//while we were trying to match event happenings.
//adds all outstanding changes and then deletes all outstanding in that order.
void JSObjectScript::flushQueuedHandlerEvents()
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
            delete mQueuedHandlerEventsDelete[s];
            mQueuedHandlerEventsDelete[s] = NULL;
        }
    }
    mQueuedHandlerEventsDelete.clear();
}



void JSObjectScript::removeHandler(JSEventHandlerStruct* toRemove)
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
void JSObjectScript::deleteHandler(JSEventHandlerStruct* toDelete)
{
    if (mHandlingEvent)
    {
        mQueuedHandlerEventsDelete.push_back(toDelete);
        return;
    }

    removeHandler(toDelete);
    delete toDelete;
    toDelete = NULL;
}


//This function takes in a jseventhandler, and wraps a javascript object with
//it.  The function is called by registerEventHandler in JSSystem, which returns
//the js object this function creates to the user.
v8::Handle<v8::Object> JSObjectScript::makeEventHandlerObject(JSEventHandlerStruct* evHand)
{
    v8::Context::Scope context_scope(mContext);
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mHandlerTemplate->NewInstance();

    returner->SetInternalField(JSHANDLER_JSEVENTHANDLER_FIELD, External::New(evHand));
    returner->SetInternalField(JSHANDLER_JSOBJSCRIPT_FIELD, External::New(this));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String (JSHANDLER_TYPEID_STRING)));

    return returner;
}



Handle<Object> JSObjectScript::getGlobalObject()
{
  // we really don't need a persistent handle
  //Local handles should work
  return mContext->Global();
}


Handle<Object> JSObjectScript::getSystemObject()
{
  HandleScope handle_scope;
  Local<Object> global_obj = mContext->Global();
  // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
  // The template actually generates the root objects prototype, not the root
  // itself.
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  // And we add an internal field to the system object as well to make it
  // easier to find the pointer in different calls. Note that in this case we
  // don't use the prototype -- non-global objects work as we would expect.
  Local<Object> system_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));

  Persistent<Object> ret_obj = Persistent<Object>::New(system_obj);
  return ret_obj;
}



//called to build the presences array as well as to build the presence keyword
void JSObjectScript::initializePresences(Handle<Object>& system_obj)
{
    v8::Context::Scope context_scope(mContext);
    // Create the space for the presences, they get filled in by
    // onConnected/onDisconnected calls
    v8::Local<v8::Array> arrayObj = v8::Array::New();
    system_obj->Set(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME), arrayObj);
}

v8::Handle<v8::Object> JSObjectScript::addConnectedPresence(const SpaceObjectReference& sporef,int token)
{
    JSPresenceStruct* presToAdd = new JSPresenceStruct(this, sporef,token);
    // Add to our internal map
    mPresences[sporef] = presToAdd;
    return addPresence(presToAdd);
}

v8::Handle<v8::Object> JSObjectScript::addPresence(JSPresenceStruct* presToAdd)
{
    HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    // Get the presences array
    v8::Local<v8::Array> presences_array =
        v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME)));
    uint32 new_pos = presences_array->Length();

    // Create the object for the new presence

    Local<Object> js_pres = mManager->mPresenceTemplate->GetFunction()->NewInstance();
    js_pres->SetInternalField(PRESENCE_FIELD_PRESENCE,External::New(presToAdd));
    js_pres->SetInternalField(TYPEID_FIELD,External::New(new String(PRESENCE_TYPEID_STRING)));

    // Insert into the presences array
    presences_array->Set(v8::Number::New(new_pos), js_pres);
    return js_pres;
}



void JSObjectScript::addWhen(JSWhenStruct* whenToAdd)
{
    JSLOG(insane, "registering when to object script");
    mWhens[whenToAdd] = true;
}

void JSObjectScript::removeWhen(JSWhenStruct* whenToRemove)
{
    WhenMapIter iter = mWhens.find(whenToRemove);
    if (iter == mWhens.end())
    {
        JSLOG(error,"could not remove when from object script because when was not already registered here");
        return;
    }

    JSLOG(insane, "removing previously registered when from object script");
    mWhens.erase(iter);
}




//presAssociatedWith: who the messages that this context's fakeroot sends will
//be from
//canMessage: who you can always send messages to.
//sendEveryone creates fakeroot that can send messages to everyone besides just
//who created you.
//recvEveryone means that you can receive messages from everyone besides just
//who created you.
//proxQueries means that you can issue proximity queries yourself, and latch on
//callbacks for them.
v8::Handle<v8::Value> JSObjectScript::createContext(JSPresenceStruct* presAssociatedWith,SpaceObjectReference* canMessage,bool sendEveryone, bool recvEveryone, bool proxQueries)
{
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mContextTemplate->NewInstance();
    JSContextStruct* internalContextField = new JSContextStruct(this,presAssociatedWith,canMessage,sendEveryone,recvEveryone,proxQueries, mManager->mContextGlobalTemplate);

    returner->SetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT, External::New(internalContextField));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String(CONTEXT_TYPEID_STRING)));


    return returner;
}



void JSObjectScript::removePresence(const SpaceObjectReference& sporef)
{
    // Find and remove from internal storage
    PresenceMap::iterator internal_it = mPresences.find(sporef);
    if (internal_it == mPresences.end()) {
        JSLOG(error, "Got removePresence call for Presence that wasn't being tracked.");
        return;
    }
    JSPresenceStruct* presence = internal_it->second;
    delete presence;
    mPresences.erase(internal_it);

    // Get presences array, find and remove from that array.
    HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);
    v8::Local<v8::Array> presences_array
        = v8::Local<v8::Array>::Cast(getSystemObject()->Get(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME)));
    // FIXME this is really annoying and completely unsafe. system.presences
    // shouldn't really be an array
    int32 remove_idx = -1;
    for(uint32 i = 0; i < presences_array->Length(); i++) {
        v8::Local<v8::Object> pres_i =
            v8::Local<v8::Object>::Cast(presences_array->Get(i));
        v8::Local<v8::External> pres_handle =
            v8::Local<v8::External>::Cast(pres_i->GetInternalField(PRESENCE_FIELD_PRESENCE));
        if (((JSPresenceStruct*)pres_handle->Value()) == presence) {
            remove_idx = i;
            break;
        }
    }
    if (remove_idx == -1) {
        JSLOG(error, "Couldn't find presence in system.presences array.");
        return;
    }
    for(uint32 i = remove_idx; i < presences_array->Length()-1; i++)
        presences_array->Set(i, presences_array->Get(i+1));
    presences_array->Delete(presences_array->Length()-1);
}

//this function can be called to re-initialize the system object's state
void JSObjectScript::populateSystemObject(Handle<Object>& system_obj)
{
    DEPRECATED(js);
}


v8::Handle<v8::Function> JSObjectScript::functionValue(const String& em_script_str)
{
  v8::HandleScope handle_scope;

  static int32_t counter;
  std::stringstream sstream;
  sstream <<  " __emerson_deserialized_function_" << counter << "__ = " << em_script_str << ";";

  //const std::string new_code = std::string("(function () { return ") + em_script_str + "}());";
  // The function name is not required. It is being put in because emerson is not compiling "( function() {} )"; correctly
  const std::string new_code = sstream.str();
  counter++;

  v8::ScriptOrigin origin(v8::String::New("(deserialized)"));
  v8::Local<v8::Value> v = v8::Local<v8::Value>::New(internalEval(mContext, new_code, &origin));
  v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(v);
  v8::Persistent<v8::Function> pf = v8::Persistent<v8::Function>::New(f);
  return pf;
}


//takes in a string corresponding to the new presence's mesh and a function
//callback to run when the presence is connected.
v8::Handle<v8::Value> JSObjectScript::create_presence(const String& newMesh, v8::Handle<v8::Function> callback )
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);


    //presuming that we are connecting to the same space;
    FIXME_GET_SPACE_OREF();
    //"space" now contains the SpaceID we want to connect to.

    //arbitrarily saying that we'll just be on top of the root object.
    Location startingLoc = mParent->getLocation(space,oref);
    //Arbitrarily saying that we're just going to use a simple bounding sphere.
    BoundingSphere3f bs = BoundingSphere3f(Vector3f::nil(), 1);


    int presToke = presenceToken;
    ++presenceToken;
    mParent->connect(space,startingLoc,bs, newMesh,UUID::null(),NULL,presToke);


    //create a presence object associated with this presence and return it;

    JSPresenceStruct* presToAdd = new JSPresenceStruct(this, callback,presToke);
    v8::Handle<v8::Object> js_pres = addPresence(presToAdd);
    mUnconnectedPresences.push_back(presToAdd);
    return js_pres;
}


void JSObjectScript::setOrientationVelFunction(const SpaceObjectReference* sporef,const Quaternion& quat)
{
    mParent->requestOrientationVelocityUpdate(sporef->space(),sporef->object(),quat);
}




void JSObjectScript::setPositionFunction(const SpaceObjectReference* sporef, const Vector3f& posVec)
{
    mParent->requestPositionUpdate(sporef->space(),sporef->object(),posVec);
}



//velocity
void JSObjectScript::setVelocityFunction(const SpaceObjectReference* sporef, const Vector3f& velVec)
{
    mParent->requestVelocityUpdate(sporef->space(),sporef->object(),velVec);
}



//orientation
void  JSObjectScript::setOrientationFunction(const SpaceObjectReference* sporef, const Quaternion& quat)
{
    mParent->requestOrientationDirectionUpdate(sporef->space(),sporef->object(),quat);
}



//scale
v8::Handle<v8::Value> JSObjectScript::getVisualScaleFunction(const SpaceObjectReference* sporef)
{
    float curscale = mParent->requestCurrentBounds(sporef->space(),sporef->object()).radius();
    return CreateJSResult(mContext, curscale);
}

void JSObjectScript::setVisualScaleFunction(const SpaceObjectReference* sporef, float newscale)
{
    BoundingSphere3f bnds = mParent->requestCurrentBounds(sporef->space(),sporef->object());
    bnds = BoundingSphere3f(bnds.center(), newscale);
    mParent->requestBoundsUpdate(sporef->space(),sporef->object(), bnds);
}


//mesh
v8::Handle<v8::Value> JSObjectScript::getVisualFunction(const SpaceObjectReference* sporef)
{
    Transfer::URI uri_returner;
    bool hasMesh = mParent->requestMeshUri(sporef->space(),sporef->object(),uri_returner);

    if (! hasMesh)
        return v8::Undefined();

    std::string string_returner = uri_returner.toString();
    return v8::String::New(string_returner.c_str(), string_returner.size());
}



//FIXME: May want to have an error handler for this function.
void  JSObjectScript::setVisualFunction(const SpaceObjectReference* sporef, const std::string& newMeshString)
{
    //FIXME: need to also pass in the object reference
    mParent->requestMeshUpdate(sporef->space(),sporef->object(),newMeshString);
}


//just sets the solid angle query for the object.
void JSObjectScript::setQueryAngleFunction(const SpaceObjectReference* sporef, const SolidAngle& sa)
{
    mParent->requestQueryUpdate(sporef->space(), sporef->object(), sa);
}


/*
  This function takes the value passed in as val, and creates a variable in ctx
  corresponding to it.  The variable name that it creates is guaranteed to be
  non-colliding with other variables that already exist.

  The function then returns the name of this new value as a string.
 */
String JSObjectScript::createNewValueInContext(v8::Handle<v8::Value> val, v8::Handle<v8::Context> ctx)
{
    String newName = "$_"+ boost::lexical_cast<String>(hiddenObjectCount);

    v8::HandleScope handle_scope;

    v8::Local<v8::Object> ctx_global_obj = ctx->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    v8::Handle<v8::Object> ctx_global_proto = Handle<Object>::Cast(ctx_global_obj->GetPrototype());

    ctx_global_proto->Set(v8::String::New(newName.c_str(), newName.length()), val);

    ++hiddenObjectCount;
    return newName;
}



v8::Handle<v8::Value> JSObjectScript::createWhen(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback, JSContextStruct* associatedContext)
{
    JSWhenStruct* internalwhen = new JSWhenStruct(predArray,callback,this, associatedContext);

    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner =mManager->mWhenTemplate->NewInstance();
    returner->SetInternalField(WHEN_TEMPLATE_FIELD, External::New(internalwhen));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String(WHEN_TYPEID_STRING)));

    return returner;
}

v8::Handle<v8::Value> JSObjectScript::createQuoted(const String& toQuote)
{
    JSQuotedStruct* internal_jsquote  = new JSQuotedStruct(toQuote);
    v8::HandleScope handle_scope;


    v8::Handle<v8::Object> returner =mManager->mQuotedTemplate->NewInstance();
    returner->SetInternalField(QUOTED_QUOTESTRUCT_FIELD, External::New(internal_jsquote));
    returner->SetInternalField(TYPEID_FIELD,External::New(new String(QUOTED_TYPEID_STRING)));

    return returner;
}


} // namespace JS
} // namespace Sirikata
