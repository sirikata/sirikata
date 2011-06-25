


#include <v8.h>
#include "JSSystemStruct.hpp"
#include "JSContextStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "JSEventHandlerStruct.hpp"
#include "../JSPattern.hpp"
#include "../JSEntityCreateInfo.hpp"
#include "JSPositionListener.hpp"
#include "JSPresenceStruct.hpp"

namespace Sirikata{
namespace JS{


JSSystemStruct::JSSystemStruct ( JSContextStruct* jscont, bool send, bool receive, bool prox,bool import, bool createPres, bool createEnt, bool evalable)
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

v8::Handle<v8::Value> JSSystemStruct::storageBeginTransaction() {
    return associatedContext->storageBeginTransaction();
}

v8::Handle<v8::Value> JSSystemStruct::storageCommit(v8::Handle<v8::Function> cb)
{
    return associatedContext->storageCommit(cb);
}
v8::Handle<v8::Value> JSSystemStruct::storageWrite(const OH::Storage::Key& key, const String& toWrite, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageWrite(key, toWrite, cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageErase(key, cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageRead(key, cb);
}


v8::Handle<v8::Value> JSSystemStruct::setRestoreScript(const String& key, v8::Handle<v8::Function> cb) {
    return associatedContext->setRestoreScript(key, cb);
}


v8::Handle<v8::Value> JSSystemStruct::checkResources()
{
    return v8::Boolean::New(true);
}


v8::Handle<v8::Value> JSSystemStruct::debug_fileWrite(const String& strToWrite,const String& filename)
{
    return associatedContext->debug_fileWrite(strToWrite,filename);
}

v8::Handle<v8::Value> JSSystemStruct::debug_fileRead(const String& filename)
{
    return associatedContext->debug_fileRead(filename);
}


v8::Handle<v8::Value> JSSystemStruct::proxAddedHandlerCallallback(v8::Handle<v8::Function>cb)
{
    return associatedContext->proxAddedHandlerCallallback(cb);
}
v8::Handle<v8::Value> JSSystemStruct::proxRemovedHandlerCallallback(v8::Handle<v8::Function>cb)
{
    return associatedContext->proxRemovedHandlerCallallback(cb);
}


v8::Handle<v8::Value> JSSystemStruct::killEntity()
{
    return associatedContext->killEntity();
}


v8::Handle<v8::Value> JSSystemStruct::restorePresence(PresStructRestoreParams& psrp)
{
    return associatedContext->restorePresence(psrp);
}


JSSystemStruct::~JSSystemStruct()
{
}



v8::Handle<v8::Value> JSSystemStruct::struct_create_vis(const SpaceObjectReference& sporefWatching,JSProxyData* addParams)
{
    return associatedContext->struct_create_vis(sporefWatching,addParams);
}


v8::Handle<v8::Value> JSSystemStruct::deserializeObject(const String& toDeserialize)
{
    return associatedContext->deserializeObject(toDeserialize);
}

v8::Handle<v8::Value> JSSystemStruct::checkHeadless()
{
    return associatedContext->checkHeadless();
}


v8::Handle<v8::Value> JSSystemStruct::struct_require(const String& toRequireFrom)
{
    //require uses the same capability as import
    if (! canImport)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to require.")));

    return associatedContext->struct_require(toRequireFrom);
}

v8::Handle<v8::Value> JSSystemStruct::struct_import(const String& toImportFrom)
{
    if (! canImport)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to import.")));

    return associatedContext->struct_import(toImportFrom);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canImport()
{
    return v8::Boolean::New(canImport);
}


v8::Handle<v8::Value> JSSystemStruct::sendMessageNoErrorHandler(JSPresenceStruct* jspres, const String& serialized_message,JSPositionListener* jspl)
{

    //checking capability for this presence
    if(getContext()->getAssociatedPresenceStruct() != NULL)
    {
        if (getContext()->getAssociatedPresenceStruct()->getSporef() == jspres->getSporef())
        {
            if (!canSend)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to send messages.")));
        }
    }

    return associatedContext->sendMessageNoErrorHandler(jspres,serialized_message,jspl);
}


v8::Handle<v8::Value> JSSystemStruct::struct_makeEventHandlerObject(const PatternList& native_patterns,v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist,bool isSuspended)
{
    if (!canRecv)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to receive messages.")));


    return associatedContext->struct_makeEventHandlerObject(native_patterns, cb_persist, sender_persist,isSuspended);
}

JSContextStruct* JSSystemStruct::getContext()
{
    return associatedContext;
}


v8::Handle<v8::Value> JSSystemStruct::struct_canCreatePres()
{
    return v8::Boolean::New(canCreatePres);
}
v8::Handle<v8::Value> JSSystemStruct::struct_canCreateEnt()
{
    return v8::Boolean::New(canCreateEnt);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canEval()
{
    return v8::Boolean::New(canEval);
}



//creates and returns a new context object.  arguments should be described in
//JSObjects/JSSystem.cpp
//new context will have at most as many permissions as parent context.
//note: if presStruct is null, just means use the one that is associated with
//this system's context
v8::Handle<v8::Value> JSSystemStruct::struct_createContext(SpaceObjectReference canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool import, bool createPres,bool createEnt, bool evalable,JSPresenceStruct* presStruct)
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

v8::Handle<v8::Value> JSSystemStruct::struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    return associatedContext->struct_registerOnPresenceConnectedHandler(cb_persist);
}

v8::Handle<v8::Value> JSSystemStruct::struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    return associatedContext->struct_registerOnPresenceDisconnectedHandler(cb_persist);
}


v8::Handle<v8::Value> JSSystemStruct::struct_eval(const String& native_contents, ScriptOrigin* sOrigin)
{
    if (!canEval)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to call eval directly.")));


    return associatedContext->struct_eval(native_contents,sOrigin);
}

v8::Handle<v8::Value> JSSystemStruct::struct_setScript(const String& script)
{
    return associatedContext->struct_setScript(script);
}

v8::Handle<v8::Value> JSSystemStruct::struct_getScript()
{
    return associatedContext->struct_getScript();
}

v8::Handle<v8::Value> JSSystemStruct::struct_reset()
{
    return associatedContext->struct_setReset();
}




//create a timer that will fire in dur seconds from now, that will bind the
//this parameter to target and that will fire the callback cb.
v8::Handle<v8::Value> JSSystemStruct::struct_createTimeout(double period, v8::Persistent<v8::Function>& cb)
{
    return associatedContext->struct_createTimeout(period,cb);
}

v8::Handle<v8::Value> JSSystemStruct::struct_createTimeout(double period,v8::Persistent<v8::Function>& cb, uint32 contID,double timeRemaining, bool isSuspended, bool isCleared)
{
    return associatedContext->struct_createTimeout(period,cb, contID, timeRemaining, isSuspended,isCleared);
}



//if have the capability to create presences, create a new presence with
//mesh newMesh and executes initFunc, which gets executed onConnected.
//if do not have the capability, throws an error.

v8::Handle<v8::Value> JSSystemStruct::struct_createPresence(const String& newMesh, v8::Handle<v8::Function> initFunc,const Vector3d& poser, const SpaceID& spaceToCreateIn)
{
    if (! canCreatePres)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to create presences.")));

    return associatedContext->struct_createPresence(newMesh, initFunc,poser,spaceToCreateIn);
}

//if have the capability to create presences, create a new presence with
//mesh newMesh and executes initFunc, which gets executed onConnected.
//if do not have the capability, throws an error.
v8::Handle<v8::Value> JSSystemStruct::struct_createEntity(EntityCreateInfo& eci)
{
    if (! canCreateEnt)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to create entities.")));

    return associatedContext->struct_createEntity(eci);
}





v8::Handle<v8::Value> JSSystemStruct::struct_canSendMessage()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(canSend);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canRecvMessage()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(canRecv);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canProx()
{
    v8::HandleScope handle_scope;  //for garbage collection.
    return v8::Boolean::New(canProx);
}

v8::Handle<v8::Value> JSSystemStruct::struct_getPosition()
{
    return associatedContext->struct_getAssociatedPresPosition();
}

v8::Handle<v8::Value> JSSystemStruct::struct_print(const String& msg)
{
    associatedContext->jsscript_print(msg);
    return v8::Undefined();
}

v8::Handle<v8::Value> JSSystemStruct::struct_sendHome(const String& toSend)
{
    return associatedContext->struct_sendHome(toSend);
}

//decodes system object
JSSystemStruct* JSSystemStruct::decodeSystemStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of JSSystemStruct.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != SYSTEM_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of JSSystemStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSRootStructObj;
    wrapJSRootStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(SYSTEM_TEMPLATE_SYSTEM_FIELD));
    void* ptr = wrapJSRootStructObj->Value();

    JSSystemStruct* returner;
    returner = static_cast<JSSystemStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSSystemStruct.  Internal field of object given cannot be casted to a JSSystemStruct.";

    return returner;
}


} //end namespace JS
} //end namespace Sirikata
