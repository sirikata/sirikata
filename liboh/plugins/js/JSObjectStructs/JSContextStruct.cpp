
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


namespace Sirikata {
namespace JS {

JSContextStruct::JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries, bool canImport,bool canCreatePres,bool canCreateEnt,bool canEval, v8::Handle<v8::ObjectTemplate> contGlobTempl)
 : JSSuspendable(),
   jsObjScript(parent),
   mContext(v8::Context::New(NULL, contGlobTempl)),
   hasOnConnectedCallback(false),
   hasOnDisconnectedCallback(false),
   associatedPresence(whichPresence),
   mHomeObject(new SpaceObjectReference(*home)),
   mSystem(new JSSystemStruct(this,sendEveryone, recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval)),
   mUtil(NULL),
   inClear(false),
   mContGlobTempl(contGlobTempl)
{
    createContextObjects();

    //no need to register this context with it's parent context because that's
    //taken care of in the createContext function of this class.
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

    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    v8::Local<v8::Object> system_obj = v8::Local<v8::Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));
    system_obj->SetInternalField(SYSTEM_TEMPLATE_SYSTEM_FIELD, v8::External::New(mSystem));
    system_obj->SetInternalField(TYPEID_FIELD, v8::External::New(new String(SYSTEM_TYPEID_STRING)));




    v8::Local<v8::Array> arrayObj = v8::Array::New();
    system_obj->Set(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME), arrayObj);

    //populates internal jscontextstruct field
    systemObj = v8::Persistent<v8::Object>::New(system_obj);

    JSUtilStruct* mUtil = new JSUtilStruct(this,jsObjScript);
    Local<Object> util_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME)));
    util_obj->SetInternalField(UTIL_TEMPLATE_UTILSTRUCT_FIELD,External::New(mUtil));
    util_obj->SetInternalField(TYPEID_FIELD,External::New(new String(UTIL_TYPEID_STRING)));
}


//creates a vec3 emerson object out of the vec3d cpp object passed in.
v8::Handle<v8::Value> JSContextStruct::struct_createVec3(Vector3d& toCreate)
{
    return CreateJSResult_Vec3Impl(mContext, toCreate);
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
    if (associatedPresence != NULL)
    {
        //ie, we're not in the root sandbox
        if (*(associatedPresence->getSporef()) == receiver)
            return true;
    }

    //check all of my presences
    if (hasPresence(receiver))
        return true;
    

    //cannot receive messages for this.
    return false;
}


//runs through suspendable map to check if have a presence in this sandbox
//matching sporef
bool JSContextStruct::hasPresence(const SpaceObjectReference& sporef)
{
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
    {
        JSPresenceStruct* jspres = dynamic_cast<JSPresenceStruct*> (iter->first);
        if (jspres != NULL)
        {
            if (*(jspres->getSporef()) == sporef)
                return true;
        }
    }
    return false;
}


//Tries to eval the emerson code in native_contents that came from origin
//sOrigin inside of this context.
v8::Handle<v8::Value> JSContextStruct::struct_eval(const String& native_contents, ScriptOrigin* sOrigin)
{
    return jsObjScript->eval(native_contents, sOrigin,this);
}

/*
  This function should be called from system object.  It initiates the reset
  process for all objects and contexts.
  
 */
v8::Handle<v8::Value> JSContextStruct::struct_setReset()
{
    //jsobjscript will chcek if this is teh root context.  If it is, it will
    //eventually call struct_rootReset
    return jsObjScript->resetScript(this);
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
    for (JSPresVecIter iter = jspresVec.begin(); iter != jspresVec.end(); ++iter)
    {
        jsObjScript->resetPresence(*iter);
        checkContextConnectCallback(*iter);
    }

    return v8::Undefined();
}





JSContextStruct::~JSContextStruct()
{
    delete mSystem;
    delete mHomeObject;

    if (hasOnConnectedCallback)
        cbOnConnected.Dispose();

    if (hasOnDisconnectedCallback)
        cbOnDisconnected.Dispose();


    if (! getIsCleared())
        mContext.Dispose();
}

//lkjs/FIXME: check if it already exists first;
v8::Persistent<v8::Object> JSContextStruct::addToPresencesArray(JSPresenceStruct* jspres)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(mContext);

    // Get the presences array
    v8::Local<v8::Array> presences_array =
        v8::Local<v8::Array>::Cast(systemObj->Get(v8::String::New(JSSystemNames::PRESENCES_ARRAY_NAME)));
    uint32 new_pos = presences_array->Length();

    // Create the object for the new presence
    v8::Local<v8::Object> js_pres =jsObjScript->wrapPresence(jspres,&(mContext));

    // Insert into the presences array
    presences_array->Set(v8::Number::New(new_pos), js_pres);

    return v8::Persistent<v8::Object>::New(js_pres);
}



void JSContextStruct::checkContextConnectCallback(JSPresenceStruct* jspres)
{
    addToPresencesArray(jspres);

    //check whether should evaluate any further callbacks.
    if (getIsSuspended() || getIsCleared())
        return;

    if (hasOnConnectedCallback)
        jsObjScript->handlePresCallback(cbOnConnected,this,jspres);
}


void JSContextStruct::checkContextDisconnectCallback(JSPresenceStruct* jspres)
{
    if (getIsSuspended() || getIsCleared())
        return;

    if (hasOnDisconnectedCallback)
        jsObjScript->handlePresCallback(cbOnDisconnected,this,jspres);
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
    inClear = true;
    JSLOG(insane,"Clearing a context.  Hopefully it works!");

    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->clear();


    systemObj.Dispose();
    if (hasOnConnectedCallback)
        cbOnConnected.Dispose();
    if (hasOnDisconnectedCallback)
        cbOnDisconnected.Dispose();

    mContext.Dispose();
    inClear = false;
    return JSSuspendable::clear();
}



void JSContextStruct::struct_registerSuspendable   (JSSuspendable* toRegister)
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when registering suspendable.  This context object was already cleared.");
        return;
    }

    
    SuspendableIter iter = associatedSuspendables.find(toRegister);
    if (iter != associatedSuspendables.end())
    {
        JSLOG(info,"Strangeness in registerSuspendable of JSContextStruct.  Trying to re-register a suspendable with the context that was already registered.  Unlikely to be an error, but thought I should mention it.");
        return;
    }
    associatedSuspendables[toRegister] = 1;
}


void JSContextStruct::struct_deregisterSuspendable (JSSuspendable* toDeregister)
{
    //don't want to de-register suspendables while I'm calling clear.
    //clear is already running through each and removing them from suspendables.
    if (inClear)
        return;
    
    if (getIsCleared())
    {
        JSLOG(error,"Error when deregistering suspendable.  This context object was already cleared.");
        return;
    }

    SuspendableIter iter = associatedSuspendables.find(toDeregister);
    if (iter == associatedSuspendables.end())
    {
        JSLOG(error,"Error when deregistering suspendable in JSContextStruct.cpp.  Trying to deregister a suspendable that the context struct had not already registered.  Likely an error.");
        return;
    }

    associatedSuspendables.erase(iter);
}



v8::Handle<v8::Value> JSContextStruct::suspend()
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when suspending.  This context object was already cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Cannot suspend a context that has already been cleared.")) );
    }

    JSLOG(insane,"Suspending all suspendable objects associated with context");
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->suspend();

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

    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->resume();

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


    jsObjScript->sendMessageToEntity(mHomeObject,associatedPresence->getSporef(),toSend);
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
v8::Handle<v8::Value> JSContextStruct::struct_createTimeout(const Duration& dur,  v8::Persistent<v8::Function>& cb)
{
    //the timer that's created automatically registers as a suspendable with
    //this context.
    return jsObjScript->create_timeout(dur, cb, this);
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
v8::Handle<v8::Value> JSContextStruct::struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport, bool canCreatePres, bool canCreateEnt, bool canEval,JSPresenceStruct* presStruct)
{
    if (presStruct == NULL)
        presStruct = associatedPresence;
    JSContextStruct* new_jscs      = NULL;

    v8::Local<v8::Object> returner = jsObjScript->createContext(presStruct,canMessage,sendEveryone,recvEveryone,proxQueries,canImport,canCreatePres,canCreateEnt,canEval,new_jscs);

    //register the new context as a child of the previous one
    struct_registerSuspendable(new_jscs);

    return returner;
}



v8::Handle<v8::Value> JSContextStruct::struct_createPresence(const String& newMesh, v8::Handle<v8::Function> initFunc)
{
    return jsObjScript->create_presence(newMesh,initFunc,this);
}

v8::Handle<v8::Value> JSContextStruct::struct_createEntity(EntityCreateInfo& eci)
{
    jsObjScript->create_entity(eci);
    return v8::Undefined();
}




//creates a new jseventhandlerstruct and wraps it in a js object
//registers the jseventhandlerstruct both with this context and
//jsobjectscript
v8::Handle<v8::Value>  JSContextStruct::struct_makeEventHandlerObject(const PatternList& native_patterns,v8::Persistent<v8::Object> target_persist, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist)
{
    //constructor of new_handler should take care of registering with context as
    //a suspendable.
    JSEventHandlerStruct* new_handler= new JSEventHandlerStruct(native_patterns, target_persist, cb_persist,sender_persist,this);

    jsObjScript->registerHandler(new_handler);
    return jsObjScript->makeEventHandlerObject(new_handler,this);
}




v8::Handle<Object> JSContextStruct::struct_getSystem()
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
  Local<Object> froot_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::SYSTEM_OBJECT_NAME)));

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
