


#include <v8.h>
#include "JSFakerootStruct.hpp"
#include "JSContextStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSObjectScript.hpp"
#include "JSEventHandlerStruct.hpp"
#include "../JSPattern.hpp"


namespace Sirikata{
namespace JS{


JSFakerootStruct::JSFakerootStruct ( JSContextStruct* jscont, bool send, bool receive, bool prox,bool import)
 : associatedContext(jscont),
   canSend(send),
   canRecv(receive),
   canProx(prox),
   canImport(import)
{
}


JSFakerootStruct::~JSFakerootStruct()
{
}

v8::Handle<v8::Value> JSFakerootStruct::struct_import(const String& toImportFrom)
{
    return associatedContext->struct_import(toImportFrom);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_canImport()
{
    return v8::Boolean::New(canImport);
}


v8::Handle<v8::Value> JSFakerootStruct::struct_makeEventHandlerObject(const PatternList& native_patterns,v8::Persistent<v8::Object> target_persist, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist)
{
    if (!canRecv)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to receive messages.")));

    
    return associatedContext->struct_makeEventHandlerObject(native_patterns, target_persist, cb_persist, sender_persist);
}

JSContextStruct* JSFakerootStruct::getContext()
{
    return associatedContext;
}

v8::Handle<v8::Value> JSFakerootStruct::struct_canSendMessage()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(canSend);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_canRecvMessage()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(canRecv);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_canProx()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(canProx);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_getPosition()
{
    return associatedContext->struct_getAssociatedPresPosition();
}

v8::Handle<v8::Value> JSFakerootStruct::struct_print(const String& msg)
{
    associatedContext->jsscript_print(msg);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSFakerootStruct::struct_sendHome(const String& toSend)
{
    return associatedContext->struct_sendHome(toSend);
}

//decodes fakeroot object
JSFakerootStruct* JSFakerootStruct::decodeRootStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    
    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of JSFakerootStruct.  Should have received an object to decode.";
        return NULL;
    }
        
    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();
        
    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != FAKEROOT_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of JSFakerootStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }
        
    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSRootStructObj;
    wrapJSRootStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(FAKEROOT_TEMAPLATE_FIELD));
    void* ptr = wrapJSRootStructObj->Value();
    
    JSFakerootStruct* returner;
    returner = static_cast<JSFakerootStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSFakerootStruct.  Internal field of object given cannot be casted to a JSFakerootStruct.";

    return returner;
}



} //end namespace JS
} //end namespace Sirikata
