


#include <v8.h>
#include "JSFakerootStruct.hpp"
#include "JSContextStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSObjectScript.hpp"
#include "JSEventHandlerStruct.hpp"
#include "../JSPattern.hpp"
#include "../JSEntityCreateInfo.hpp"

namespace Sirikata{
namespace JS{


JSFakerootStruct::JSFakerootStruct ( JSContextStruct* jscont, bool send, bool receive, bool prox,bool import, bool createPres, bool createEnt, bool evalable)
 : associatedContext(jscont),
   canSend(send),
   canRecv(receive),
   canProx(prox),
   canImport(import),
   canCreatePres(createPres),
   canCreateEnt(createEnt),
   canEval(evalable)
{
}


JSFakerootStruct::~JSFakerootStruct()
{
}


v8::Handle<v8::Value> JSFakerootStruct::struct_require(const String& toRequireFrom)
{
    //require uses the same capability as import
    if (! canImport)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to require.")));
    
    return associatedContext->struct_require(toRequireFrom);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_import(const String& toImportFrom)
{
    if (! canImport)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to import.")));
    
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

//creates and returns a new context object.  arguments should be described in
//JSObjects/JSFakeroot.cpp
//new context will have at most as many permissions as parent context.
//note: if presStruct is null, just means use the one that is associated with
//this fakeroot's context
v8::Handle<v8::Value> JSFakerootStruct::struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool import, bool createPres,bool createEnt, bool evalable,JSPresenceStruct* presStruct)
{
    sendEveryone &= canSend;
    recvEveryone &= canRecv;
    proxQueries  &= canProx;
    import       &= canImport;
    createPres   &= canCreatePres;
    createEnt    &= canCreateEnt;
    evalable     &= canEval;
    
    return associatedContext->struct_createContext(canMessage, sendEveryone,recvEveryone,proxQueries,import,createPres,createEnt,evalable,presStruct);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    return associatedContext->struct_registerOnPresenceConnectedHandler(cb_persist);
}

v8::Handle<v8::Value> JSFakerootStruct::struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    return associatedContext->struct_registerOnPresenceDisconnectedHandler(cb_persist);
}


v8::Handle<v8::Value> JSFakerootStruct::struct_eval(const String& native_contents, ScriptOrigin* sOrigin)
{
    if (!canEval)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to call eval directly.")));

    
    return associatedContext->struct_eval(native_contents,sOrigin);
}

//create a timer that will fire in dur seconds from now, that will bind the
//this parameter to target and that will fire the callback cb.
v8::Handle<v8::Value> JSFakerootStruct::struct_createTimeout(const Duration& dur, v8::Persistent<v8::Function>& cb)
{
    return associatedContext->struct_createTimeout(dur,cb);
}



//if have the capability to create presences, create a new presence with
//mesh newMesh and executes initFunc, which gets executed onConnected.
//if do not have the capability, throws an error.
v8::Handle<v8::Value> JSFakerootStruct::struct_createPresence(const String& newMesh, v8::Handle<v8::Function> initFunc)
{
    if (! canCreatePres)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to create presences.")));

    return associatedContext->struct_createPresence(newMesh, initFunc);
}

//if have the capability to create presences, create a new presence with
//mesh newMesh and executes initFunc, which gets executed onConnected.
//if do not have the capability, throws an error.
v8::Handle<v8::Value> JSFakerootStruct::struct_createEntity(EntityCreateInfo& eci)
{
    if (! canCreateEnt)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to create entities.")));

    return associatedContext->struct_createEntity(eci);
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
