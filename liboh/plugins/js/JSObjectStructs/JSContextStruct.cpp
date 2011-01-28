
#include "JSContextStruct.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../JSSystemNames.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"

namespace Sirikata {
namespace JS {




JSContextStruct::JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries, v8::Handle<v8::ObjectTemplate> contGlobTempl)
 : jsObjScript(parent),
   associatedPresence(whichPresence),
   mHomeObject(new SpaceObjectReference(*home)),
   mFakeroot(new JSFakerootStruct(this,sendEveryone, recvEveryone,proxQueries)),
   mContext(v8::Context::New(NULL, contGlobTempl))
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

    
}



JSContextStruct::~JSContextStruct()
{
    delete mFakeroot;
    delete mHomeObject;
    mContext.Dispose();
}


//this function asks the jsObjScript to send a message from the presence associated
//with associatedPresence to the object with spaceobjectreference mHomeObject.
//The message contains the object toSend.
v8::Handle<v8::Value> JSContextStruct::struct_sendHome(String& toSend)
{
    jsObjScript->sendMessageToEntity(mHomeObject,associatedPresence->sporef,toSend);
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


void JSContextStruct::struct_deregisterTimeout(JSTimerStruct* jsts)
{
    TimerMap::iterator iter = associatedTimers.find(jsts);

    if (iter != associatedTimers.end())
        associatedTimers.erase(iter);
}


void JSContextStruct::struct_registerTimeout(JSTimerStruct* jsts)
{
    associatedTimers[jsts] = true;
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
    return associatedPresence->struct_getPosition();
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
