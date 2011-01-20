
#include "JSContextStruct.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>
#include "JSPresenceStruct.hpp"

namespace Sirikata {
namespace JS {




JSContextStruct::JSContextStruct(JSObjectScript* parent, JSPresenceStruct* whichPresence, SpaceObjectReference* home, bool sendEveryone, bool recvEveryone, bool proxQueries)
 : jsObjScript(parent),
   associatedPresence(whichPresence),
   mHomeObject(new SpaceObjectReference(*home)),
   mFakeroot(new JSFakerootStruct(this,sendEveryone, recvEveryone,proxQueries)),
   mContext(v8::Context::New())
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
    v8::Local<v8::Object> system_obj = v8::Local<v8::Object>::Cast(global_proto->Get(v8::String::New(JSSystemNames::FAKEROOT_OBJECT_NAME)));
    //system_obj->SetInternalField(SYSTEM_TEMPLATE_JSOBJSCRIPT_FIELD,External::New(this));
    
        
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
v8::Handle<v8::Value> JSContextStruct::sendHome(String& toSend)
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
    if (toDecodeObject->InternalFieldCount() != CONTEXT_FIELD_CONTEXT_STRUCT)
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


v8::Handle<v8::Value> JSContextStruct::executeScript(v8::Handle<v8::Function> funcToCall,int argc, v8::Handle<v8::Value>* argv)
{
    return jsObjScript->executeInContext(mContext,funcToCall, argc,argv);
}

v8::Handle<v8::Value> JSContextStruct::getAssociatedPresPosition()
{
    return associatedPresence->script_getPosition();
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
