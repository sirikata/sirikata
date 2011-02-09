
#include "JSContextStruct.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../JSSystemNames.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"
#include "JSSuspendable.hpp"
#include "../JSLogging.hpp"


namespace Sirikata {
namespace JS {

JSContextStruct::JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries, v8::Handle<v8::ObjectTemplate> contGlobTempl)
 : JSSuspendable(),
   jsObjScript(parent),
   associatedPresence(whichPresence),
   mHomeObject(new SpaceObjectReference(*home)),
   mFakeroot(new JSFakerootStruct(this,sendEveryone, recvEveryone,proxQueries)),
   mContext(v8::Context::New(NULL, contGlobTempl)),
   isSuspended(false)
{
    v8::HandleScope handle_scope;
    
    v8::Local<v8::Object> global_obj = mContext->Global();
    // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
    // The template actually generates the root objects prototype, not the root
    // itself.
    v8::Handle<v8::Object> global_proto = v8::Handle<v8::Object>::Cast(global_obj->GetPrototype());
    
    // And we add an internal field to the system object as well to make it
    // easier to find the pointer in different calls. Note that in this case we
    // don't use the prototype -- non-global objects work as we would expect.
    v8::Local<v8::Object> fakeroot_obj = v8::Local<v8::Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::FAKEROOT_OBJECT_NAME)));
    fakeroot_obj->SetInternalField(FAKEROOT_TEMAPLATE_FIELD, v8::External::New(mFakeroot));
    fakeroot_obj->SetInternalField(TYPEID_FIELD, v8::External::New(new String(FAKEROOT_TYPEID_STRING)));


    Local<Object> util_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::UTIL_OBJECT_NAME)));
    util_obj->SetInternalField(UTIL_TEMPLATE_JSOBJSCRIPT_FIELD,External::New(this));
    util_obj->SetInternalField(TYPEID_FIELD,External::New(new String(UTIL_TYPEID_STRING)));
    
}


//looks in the current context for a fakeroot object.
//tries to decode that fakeroot object as a cpp fakeroot.
//if successful, returns the JSContextStruct associated with that fakeroot
//object.
//if unsuccessful, returns NULL.
JSContextStruct* JSContextStruct::getJSContextStruct()
{
    v8::HandleScope handle_scope;
    
    v8::Handle<v8::Context> currContext = v8::Context::GetCurrent();
    if (currContext->Global()->Has(v8::String::New(JSSystemNames::FAKEROOT_OBJECT_NAME)))
    {
        v8::Handle<v8::Value> fakerootVal = currContext->Global()->Get(v8::String::New(JSSystemNames::FAKEROOT_OBJECT_NAME));
        String errorMessage; //error message isn't important in this case.  Not
                             //an error to not be within a js context struct.
        JSFakerootStruct* jsfakeroot = JSFakerootStruct::decodeRootStruct(fakerootVal,errorMessage);

        if (jsfakeroot == NULL)
            return NULL;
        
        return jsfakeroot->associatedContext;
    }

    return NULL;
}


JSContextStruct::~JSContextStruct()
{
    delete mFakeroot;
    delete mHomeObject;
    if (! getIsCleared())
        mContext.Dispose();
}


v8::Handle<v8::Value> JSContextStruct::clear()
{
    JSLOG(error,"Error.  Have not finished writing context clear's cleanup methods.  For instance, may want to delete fakeroot and homeobject.");

    assert(false);
    
    for (SuspendableIter iter = associatedSuspendables.begin(); iter != associatedSuspendables.end(); ++iter)
        iter->first->clear();
    
    mContext.Dispose();
    
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
        JSLOG(error,"Strangeness in registerSuspendable of JSContextStruct.  Trying to re-register a suspendable with the context that was already registered.  Likely an error.");
        return;
    }

    associatedSuspendables[toRegister] = 1;
}


void JSContextStruct::struct_deregisterSuspendable (JSSuspendable* toDeregister)
{
    if (getIsCleared())
    {
        JSLOG(error,"Error when registering suspendable.  This context object was already cleared.");
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
v8::Handle<v8::Value> JSContextStruct::struct_sendHome(String& toSend)
{
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

    
    int argc = args.Length(); //args to function.  first argument is going to be
                              //a 
    Handle<Value>* argv = new Handle<Value>[argc];

    
    //putting fakeroot in argv0
    argv[0] = struct_getFakeroot();
    for (int s=1; s < args.Length(); ++s)
        argv[s] = args[s];

    
    v8::Handle<v8::Value> returner =  jsObjScript->executeInContext(mContext,funcToCall, argc,argv);
    
    delete argv; //free additional memory.
    return returner;
}




v8::Handle<Object> JSContextStruct::struct_getFakeroot()
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
  Local<Object> froot_obj = Local<Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::FAKEROOT_OBJECT_NAME)));

  Persistent<Object> ret_obj = Persistent<Object>::New(froot_obj);
  
  return ret_obj;
}


v8::Handle<v8::Value> JSContextStruct::struct_getAssociatedPresPosition()
{
    return jsObjScript->getContextPosition(mContext,associatedPresence->getSporef() );
    //return associatedPresence->struct_getPosition();
}

void JSContextStruct::jsscript_print(const String& msg)
{
    jsObjScript->print(msg);
}

void JSContextStruct::presenceDied()
{
    SILOG(js,error,"[JS] Incorrectly handling presence destructions in context struct.  Need additional code.");
}


}//js namespace
}//sirikata namespace
