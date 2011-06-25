
#include "JSContextStruct.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../JSSystemNames.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"
#include "JSSuspendable.hpp"
#include "../JSLogging.hpp"
#include "JSUtilStruct.hpp"
#include "JSEventHandlerStruct.hpp"
#include "../JSPattern.hpp"
#include "../JSEntityCreateInfo.hpp"
#include "../JSUtil.hpp"
#include <sirikata/core/util/Vector3.hpp>
#include "../JSObjects/JSVec3.hpp"
#include "JS_JSMessage.pbj.hpp"
#include "../JSSerializer.hpp"
#include "../EmersonScript.hpp"
#include "JSSystemStruct.hpp"

namespace Sirikata {
namespace JS {

JSContextStruct::JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference home, bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport,bool canCreatePres,bool canCreateEnt,bool canEval, v8::Handle<v8::ObjectTemplate> contGlobTempl,uint32 contID)
 : JSSuspendable(),
   jsObjScript(parent),
   mContext(v8::Context::New(NULL, contGlobTempl)),
   mContextID(contID),
   hasOnConnectedCallback(false),
   hasOnDisconnectedCallback(false),
   associatedPresence(whichPresence),
   mHomeObject(home),
   mSystem(new JSSystemStruct(this,sendEveryone, recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval)),
   mContGlobTempl(contGlobTempl),
   mUtil(NULL),
   inClear(false),
   mInSuspendableLoop(false)
{
    createContextObjects();

    //no need to register this context with it's parent context because that's
    //taken care of in the createContext function of this class.
}

v8::Handle<v8::Value> JSContextStruct::storageBeginTransaction()
{
    return jsObjScript->storageBeginTransaction(this);
}
v8::Handle<v8::Value> JSContextStruct::storageCommit(v8::Handle<v8::Function> cb)
{
    return jsObjScript->storageCommit(this, cb);
}
v8::Handle<v8::Value> JSContextStruct::storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb)
{
    return jsObjScript->storageWrite(key,toWrite,cb,this);
}

v8::Handle<v8::Value> JSContextStruct::storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb)
{
    return jsObjScript->storageErase(key,cb,this);
}

v8::Handle<v8::Value> JSContextStruct::storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb)
{
    return jsObjScript->storageRead(key,cb,this);
}


v8::Handle<v8::Value> JSContextStruct::setRestoreScript(const String& key, v8::Handle<v8::Function> cb) {
    return jsObjScript->setRestoreScript(this, key, cb);
}


v8::Handle<v8::Value>JSContextStruct::debug_fileWrite(const String& strToWrite,const String& filename)
{
    return jsObjScript->debug_fileWrite(strToWrite,filename);
}

v8::Handle<v8::Value> JSContextStruct::debug_fileRead(const String& filename)
{
    return jsObjScript->debug_fileRead(filename);
}

uint32 JSContextStruct::getContextID()
{
    return mContextID;
}

v8::Handle<v8::Value> JSContextStruct::restorePresence(PresStructRestoreParams& psrp)
{
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,restorePresence,jsObjScript);
    return emerScript->restorePresence(psrp,this);
}



v8::Handle<v8::Value> JSContextStruct::proxAddedHandlerCallallback(v8::Handle<v8::Function>cb)
{
    if (!proxAddedFunc.IsEmpty())
        proxAddedFunc.Dispose();

    proxAddedFunc = v8::Persistent<v8::Function>::New(cb);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSContextStruct::proxRemovedHandlerCallallback(v8::Handle<v8::Function>cb)
{
    if (!proxRemovedFunc.IsEmpty())
        proxRemovedFunc.Dispose();

    proxRemovedFunc = v8::Persistent<v8::Function>::New(cb);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSContextStruct::killEntity()
{
    //checks to make sure executing killEntity from a non-headless script
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,killEntity,jsObjScript);
    return emerScript->killEntity(this);
}

/**
   Function runs through suspendablesToDelete and deregisters them.
   Also runs through suspendablesToAdd and adds them.  (Makes it so that we
   don't mess up our iterators by adding to and removing from what they're
   iterating over.)
 */
void JSContextStruct::flushQueuedSuspendablesToChange()
{
    if (mInSuspendableLoop)
    {
        JSLOG(error, "Error: should not be trying to flush queued suspendables until not in suspendable loop.  Aborting clearing suspendable objects");
        return;
    }


    for (SuspendableVecIter iter = suspendablesToDelete.begin(); iter != suspendablesToDelete.end(); ++iter)
        struct_deregisterSuspendable(*iter);
    for (SuspendableVecIter iter = suspendablesToAdd.begin(); iter != suspendablesToAdd.end(); ++iter)
        struct_registerSuspendable(*iter);

    suspendablesToDelete.clear();
    suspendablesToAdd.clear();
}

//performs the initialization and population of util object, system object,
//and system object's presences array.
void JSContextStruct::createContextObjects()
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    v8::Local<v8::Object> global_obj = mContext->Global();


    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    v8::Handle<v8::Object> global_proto = v8::Handle<v8::Object>::Cast(global_obj->GetPrototype());

    global_proto->SetInternalField(CONTEXT_GLOBAL_JSOBJECT_SCRIPT_FIELD, v8::External::New(jsObjScript));
    global_proto->SetInternalField(TYPEID_FIELD, v8::External::New(new String(CONTEXT_GLOBAL_TYPEID_STRING)));



    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    v8::Local<v8::Object> system_obj = v8::Local<v8::Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));
    system_obj->SetInternalField(SYSTEM_TEMPLATE_SYSTEM_FIELD, v8::External::New(mSystem));
    system_obj->SetInternalField(TYPEID_FIELD, v8::External::New(new String(SYSTEM_TYPEID_STRING)));


    //populates internal jscontextstruct field
    systemObj = v8::Persistent<v8::Object>::New(system_obj);

    JSUtilStruct* mUtil = new JSUtilStruct(this);
    Local<Object> util_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME)));
    util_obj->SetInternalField(UTIL_TEMPLATE_UTILSTRUCT_FIELD,External::New(mUtil));
    util_obj->SetInternalField(TYPEID_FIELD,External::New(new String(UTIL_TYPEID_STRING)));

    //Always load the shim layer.
    //import shim
    jsObjScript->import("std/shim.em",this);

}

v8::Handle<v8::Value>  JSContextStruct::checkHeadless()
{
    EmersonScript* emerScript = dynamic_cast<EmersonScript*> (jsObjScript);
    if (emerScript == NULL)
        return v8::Boolean::New(false);

    return v8::Boolean::New(true);
}


v8::Handle<v8::Value> JSContextStruct::struct_create_vis(const SpaceObjectReference& sporefWatching,JSProxyData* addParams)
{
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,create_vis,jsObjScript);
    return emerScript->createVisiblePersistent(sporefWatching, addParams, mContext);
}



//creates a vec3 emerson object out of the vec3d cpp object passed in.
v8::Handle<v8::Value> JSContextStruct::struct_createVec3(Vector3d& toCreate)
{
    return CreateJSResult_Vec3Impl(mContext, toCreate);
}

v8::Handle<v8::Value> JSContextStruct::sendMessageNoErrorHandler(JSPresenceStruct* jspres,const String& serialized_message,JSPositionListener* jspl)
{
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,sendMessage,jsObjScript);
    emerScript->sendMessageToEntity( jspl->getSporef(), jspres->getSporef(), serialized_message);

    return v8::Undefined();
}



//string argument is the filename that we're trying to open and execute
//contents of.
v8::Handle<v8::Value>  JSContextStruct::struct_import(const String& toImportFrom)
{
    return jsObjScript->import(toImportFrom,this);
}

//string argument is the filename that we're trying to open and execute
//contents of.
v8::Handle<v8::Value>  JSContextStruct::struct_require(const String& toRequireFrom)
{
    return jsObjScript->require(toRequireFrom,this);
}

//if receiver is one of my presences, or it is the system presence that I
//was created from return true.  Otherwise, return false.
bool JSContextStruct::canReceiveMessagesFor(const SpaceObjectReference& receiver)
{
    if ((associatedPresence != NULL) && (associatedPresence->getSporef() != SpaceObjectReference::null()))
    {
        //ie, we're not in the root sandbox
        if (associatedPresence->getSporef() == receiver)
            return true;
    }

    //check all of my presences
    if (hasPresence(receiver))
        return true;


    //cannot receive messages for this.
    return false;
}

v8::Handle<v8::Value> JSContextStruct::deserializeObject(const String& toDeserialize)
{
    Sirikata::JS::Protocol::JSMessage js_msg;
    bool parsed = js_msg.ParseFromString(toDeserialize);

    if (!parsed)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error deserializing string.")));

    v8::Local<v8::Object> obj = v8::Object::New();

    CHECK_EMERSON_SCRIPT_ERROR(emerScript,deserialize,jsObjScript);
    bool deserializedSuccess = JSSerializer::deserializeObject(emerScript, js_msg,obj);
    if (!deserializedSuccess)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error could not deserialize object")));

    return obj;
}


//runs through suspendable map to check if have a presence in this sandbox
//matching sporef
bool JSContextStruct::hasPresence(const SpaceObjectReference& sporef)
{
    mInSuspendableLoop = true;
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
    {
        JSPresenceStruct* jspres = dynamic_cast<JSPresenceStruct*> (iter->first);
        if ((jspres != NULL) && (jspres->getSporef() != SpaceObjectReference::null()))
        {
            if (jspres->getSporef() == sporef)
            {
                mInSuspendableLoop = false;
                flushQueuedSuspendablesToChange();
                return true;
            }
        }
    }
    mInSuspendableLoop = false;
    flushQueuedSuspendablesToChange();
    return false;
}


//Tries to eval the emerson code in native_contents that came from origin
//sOrigin inside of this context.
v8::Handle<v8::Value> JSContextStruct::struct_eval(const String& native_contents, ScriptOrigin* sOrigin)
{
    return jsObjScript->eval(native_contents,sOrigin,this);
}


/*
  This function should be called from system object.  It initiates the reset
  process for all objects and contexts.

 */
v8::Handle<v8::Value> JSContextStruct::struct_setReset()
{
    //jsobjscript will chcek if this is the root context.  If it is, returns
    //undefined, and schedules reset (eventually calling rootReset).  If it is
    //not, throws an error.
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,reset,jsObjScript);
    return emerScript->requestReset(this);
}

v8::Handle<v8::Value> JSContextStruct::struct_getScript()
{
    return v8::String::New(getScript().c_str(), getScript().size());
}

String JSContextStruct::getScript()
{
    return mScript;
}

v8::Handle<v8::Value> JSContextStruct::struct_setScript(const String& script)
{
    if (jsObjScript->isRootContext(this))
    {
        mScript = script;
        return v8::Undefined();
    }

    return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot set script for non-root context.")) );
}


/**
   This function is called only by JSObjectScript directly.
   It disposes of mContext and system objects.  It then runs through all
   suspendables associated with this context, calling clear on each (minus the
   presences).

 */
v8::Handle<v8::Value> JSContextStruct::struct_rootReset()
{
    inClear = true;
    JSPresVec jspresVec;
    mInSuspendableLoop = true;
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
    {

        JSPresenceStruct* jspres  = dynamic_cast<JSPresenceStruct*> (iter->first);
        if (jspres == NULL)
        {
            iter->first->clear();
            delete iter->first;
        }
        else
            jspresVec.push_back(jspres);
    }
    associatedSuspendables.clear();
    mInSuspendableLoop = false;
    suspendablesToDelete.clear();
    suspendablesToAdd.clear();

    //get rid of previous stuff in root
    systemObj.Dispose();
    mContext.Dispose();

    inClear = false;
    //recreate system and mcontext objects
    mContext = v8::Context::New(NULL, mContGlobTempl);
    createContextObjects();

    //re-exec mScript
    v8::ScriptOrigin origin(v8::String::New("(reset_script)"));
    jsObjScript->internalEval(mContext,mScript,&origin , true);


    //re-load presences
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,reset,jsObjScript);
    for (JSPresVecIter iter = jspresVec.begin(); iter != jspresVec.end(); ++iter)
    {
        emerScript->resetPresence(*iter);
        struct_registerSuspendable   (*iter);
        checkContextConnectCallback(*iter);
    }

    return v8::Undefined();
}

//should have called clear before got to destructor
JSContextStruct::~JSContextStruct()
{
    clear();

    delete mSystem;
    delete mUtil;

}




void JSContextStruct::checkContextConnectCallback(JSPresenceStruct* jspres)
{
    EmersonScript* emerScript = dynamic_cast<EmersonScript*> (jsObjScript);
    if (emerScript == NULL)
        return;

    //check whether should evaluate any further callbacks.
    if (getIsSuspended() || getIsCleared())
        return;

    if (hasOnConnectedCallback)
        emerScript->handlePresCallback(cbOnConnected,this,jspres);
}


void JSContextStruct::checkContextDisconnectCallback(JSPresenceStruct* jspres)
{
    EmersonScript* emerScript = dynamic_cast<EmersonScript*> (jsObjScript);
    if (emerScript == NULL)
        return;

    if (getIsSuspended() || getIsCleared())
        return;

    if (hasOnDisconnectedCallback)
        emerScript->handlePresCallback(cbOnDisconnected,this,jspres);
}

v8::Handle<v8::Value> JSContextStruct::struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    if (hasOnConnectedCallback)
        cbOnConnected.Dispose();

    cbOnConnected = cb_persist;
    hasOnConnectedCallback = true;

    return cbOnConnected;
}

v8::Handle<v8::Value> JSContextStruct::struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    if (hasOnDisconnectedCallback)
        cbOnDisconnected.Dispose();

    cbOnDisconnected = cb_persist;
    hasOnDisconnectedCallback = true;

    return cbOnDisconnected;
}


//Destroys all objects that were created in this context + all of this context's
//subcontexts.
v8::Handle<v8::Value> JSContextStruct::clear()
{
    if (getIsCleared())
        return v8::Boolean::New(true);

    v8::HandleScope handle_scope;
    v8::Handle<v8::Value> returner = JSSuspendable::clear();
    
    
    inClear = true;
    JSLOG(insane,"Clearing context.");


    mInSuspendableLoop = true;
    //when a suspendable gets cleared, it calls deregister_suspendable of
    //JSContextStruct directly.  All deletion is done in that method or when
    //flushing queued methods.
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->clear();
    mInSuspendableLoop = false;
    flushQueuedSuspendablesToChange();


    systemObj.Dispose();
    if (hasOnConnectedCallback)
        cbOnConnected.Dispose();
    if (hasOnDisconnectedCallback)
        cbOnDisconnected.Dispose();

    mContext.Dispose();
    inClear = false;
    return handle_scope.Close(returner);
}



void JSContextStruct::struct_registerSuspendable   (JSSuspendable* toRegister)
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when registering suspendable.  This context object was already cleared.");
        return;
    }

    if (mInSuspendableLoop)
    {
        suspendablesToAdd.push_back(toRegister);
        return;
    }


    SuspendableIter iter = associatedSuspendables.find(toRegister);
    if (iter != associatedSuspendables.end())
    {
        JSLOG(insane,"Strangeness in registerSuspendable of JSContextStruct.  Trying to re-register a suspendable with the context that was already registered.  Unlikely to be an error, but thought I should mention it.");
        return;
    }
    associatedSuspendables[toRegister] = 1;
}


void JSContextStruct::struct_deregisterSuspendable (JSSuspendable* toDeregister)
{
    if (mInSuspendableLoop)
    {
        suspendablesToDelete.push_back(toDeregister);
        return;
    }

    //remove the suspendable from our array of suspendable.s
    SuspendableIter iter = associatedSuspendables.find(toDeregister);
    if (iter == associatedSuspendables.end())
    {
        JSLOG(error,"Error when deregistering suspendable in JSContextStruct.cpp.  Trying to deregister a suspendable that the context struct had not already registered.  Likely an error.");
        return;
    }
    associatedSuspendables.erase(iter);

    EmersonScript* emerScript = dynamic_cast<EmersonScript*> (jsObjScript);
    
    //if it's an event handler struct, we also need to ensure that it is removed
    //from EmersonScript.  Calling emerScript->deleteHandler will kill the
    //event handler and free its memory.  Otherwise, we can free it directly here.
    JSEventHandlerStruct* jsev = dynamic_cast<JSEventHandlerStruct*>(toDeregister);
    if (jsev != NULL)
    {
        if (emerScript == NULL)
        {
            JSLOG(error, "should not be deregistering an event listener from headless script.");
            return;
        }
        
        emerScript->deleteHandler(jsev);
        return;
    }
    
    //handle presence clear.
    JSPresenceStruct* jspres = dynamic_cast<JSPresenceStruct*> (toDeregister);
    if (jspres != NULL)
    {
        if (emerScript == NULL)
        {
            JSLOG(error, "should not be deregistering an presence from a headless script.");
            return;
        }

        emerScript->deletePres(jspres);
        return;
    }

    

    //if it's just a timer or a context, can delete without requesting
    //Emerscript to do anything special.
    delete toDeregister;

}



v8::Handle<v8::Value> JSContextStruct::suspend()
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when suspending.  This context object was already cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot suspend a context that has already been cleared.")) );
    }


    JSLOG(insane,"Suspending all suspendable objects associated with context");
    mInSuspendableLoop = true;
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->suspend();

    mInSuspendableLoop = false;
    flushQueuedSuspendablesToChange();

    return JSSuspendable::suspend();
}

v8::Handle<v8::Value> JSContextStruct::resume()
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when resuming.  This context object was already cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot resume a context that has already been cleared.")) );
    }


    JSLOG(insane,"Resuming all suspendable objects associated with context");

    mInSuspendableLoop = true;
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->resume();

    mInSuspendableLoop = false;
    flushQueuedSuspendablesToChange();

    return JSSuspendable::resume();
}



//this function asks the jsObjScript to send a message from the presence associated
//with associatedPresence to the object with spaceobjectreference mHomeObject.
//The message contains the object toSend.
v8::Handle<v8::Value> JSContextStruct::struct_sendHome(const String& toSend)
{

    NullPresenceCheck("Context: sendHome");

    if (getIsCleared())
    {
        JSLOG(error,"Error when sending home.  This context object was already cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot call sendHome from a context that has already been cleared.")) );
    }

    CHECK_EMERSON_SCRIPT_ERROR(emerScript,sendHome,jsObjScript);
    emerScript->sendMessageToEntity(mHomeObject,associatedPresence->getSporef(),toSend);
    return v8::Undefined();
}


JSContextStruct* JSContextStruct::decodeContextStruct(v8::Handle<v8::Value> toDecode, String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of JSContextStruct.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != CONTEXT_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of JSContextStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSContextStructObj;
    wrapJSContextStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(CONTEXT_FIELD_CONTEXT_STRUCT));
    void* ptr = wrapJSContextStructObj->Value();

    JSContextStruct* returner;
    returner = static_cast<JSContextStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSContextStruct.  Internal field of object given cannot be casted to a JSContextStruct.";

    return returner;
}


//first argument of args is a function (funcToCall), which we skip
v8::Handle<v8::Value> JSContextStruct::struct_executeScript(v8::Handle<v8::Function> funcToCall,const v8::Arguments& args)
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when executing.  This context object was already cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot call execute on a context that has already been cleared.")) );
    }


    //copying arguments over to arg array for execution.
    int argc = args.Length() -1;
    Handle<Value>* argv = new Handle<Value>[argc];

    for (int s=1; s < args.Length(); ++s)
        argv[s-1] = args[s];

    v8::Handle<v8::Value> returner =  jsObjScript->executeInSandbox(mContext,funcToCall, argc,argv);

    delete argv; //free additional memory.
    return returner;
}



//create a timer that will fire in dur seconds from now, that will bind the
//this parameter to target and that will fire the callback cb.
v8::Handle<v8::Value> JSContextStruct::struct_createTimeout(double period,  v8::Persistent<v8::Function>& cb)
{
    //the timer that's created automatically registers as a suspendable with
    //this context.
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,createTimeout,jsObjScript);
    return emerScript->create_timeout(period, cb, this);
}

v8::Handle<v8::Value> JSContextStruct::struct_createTimeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared)
{
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,createTimeout,jsObjScript);
    return emerScript->create_timeout(period,cb, contID, timeRemaining, isSuspended,isCleared,this);
}




/**
presStruct: who the messages that this context's system sends will
be from

canMessage: who you can always send messages to.

sendEveryone creates system that can send messages to everyone besides just
who created you.

recvEveryone means that you can receive messages from everyone besides just
who created you.

proxQueries means that you can issue proximity queries yourself, and latch on
callbacks for them.

canImport means that you can import files/libraries into your code.

canCreatePres is whether have capability to create presences

canCreateEnt is whether have capability to create entities

if presStruct is null, just use the presence that is associated with this
context (which may be null as well).
*/
v8::Handle<v8::Value> JSContextStruct::struct_createContext(SpaceObjectReference canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport, bool canCreatePres, bool canCreateEnt, bool canEval,JSPresenceStruct* presStruct)
{
    if (presStruct == NULL)
        presStruct = associatedPresence;
    JSContextStruct* new_jscs      = NULL;

    v8::Local<v8::Object> returner = jsObjScript->createContext(presStruct,canMessage,sendEveryone,recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval,new_jscs);

    //register the new context as a child of the previous one
    struct_registerSuspendable(new_jscs);

    return returner;
}



v8::Handle<v8::Value> JSContextStruct::struct_createPresence(const String& newMesh, v8::Handle<v8::Function> initFunc,const Vector3d& poser, const SpaceID& spaceToCreateIn)
{
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,createPresence,jsObjScript);
    return emerScript->create_presence(newMesh,initFunc,this, poser, spaceToCreateIn);
}

v8::Handle<v8::Value> JSContextStruct::struct_createEntity(EntityCreateInfo& eci)
{
    CHECK_EMERSON_SCRIPT_ERROR(emerScript,createEntity,jsObjScript);
    emerScript->create_entity(eci);
    return v8::Undefined();
}




//creates a new jseventhandlerstruct and wraps it in a js object
//registers the jseventhandlerstruct both with this context and
//jsobjectscript
v8::Handle<v8::Value>  JSContextStruct::struct_makeEventHandlerObject(const PatternList& native_patterns, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist,bool issusp)
{
    //constructor of new_handler should take care of registering with context as
    //a suspendable.
    JSEventHandlerStruct* new_handler= new JSEventHandlerStruct(native_patterns, cb_persist,sender_persist,this,issusp);

    CHECK_EMERSON_SCRIPT_ERROR(emerScript,makeEventHandler,jsObjScript);
    emerScript->registerHandler(new_handler);
    return emerScript->makeEventHandlerObject(new_handler,this);
}




v8::Handle<Object> JSContextStruct::struct_getSystem()
{
  HandleScope handle_scope;
  Local<Object> global_obj = mContext->Global();
  // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
  // The template actually generates the root objects prototype, not the root
  // itself.
  //Otherwise, find it in the prototype.
  Handle<Object> global_proto = Handle<Object>::Cast(global_obj->GetPrototype());
  // And we add an internal field to the system object as well to make it
  // easier to find the pointer in different calls. Note that in this case we
  // don't use the prototype -- non-global objects work as we would expect.
  v8::Handle<v8::Value> syskey = v8::String::New(JSSystemNames::SYSTEM_OBJECT_PUBLIC_NAME);
  Local<Object> froot_obj = Local<Object>::Cast(global_proto->Get(syskey));

  Persistent<Object> ret_obj = Persistent<Object>::New(froot_obj);

  return ret_obj;
}


v8::Handle<v8::Value> JSContextStruct::struct_getAssociatedPresPosition()
{
    NullPresenceCheck("Context: getAssociatedPresPosition");

    v8::Context::Scope context_scope(mContext);
    v8::Handle<v8::Value> returner = associatedPresence->struct_getPosition();

    return returner;
}

void JSContextStruct::jsscript_print(const String& msg)
{
    jsObjScript->print(msg);
}

void JSContextStruct::presenceDied()
{
    JSLOG(error,"[JS] Incorrectly handling presence destructions in context struct.  Need additional code.");
}



}//js namespace
}//sirikata namespace
