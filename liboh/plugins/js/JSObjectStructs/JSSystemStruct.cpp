


#include <v8.h>
#include "JSSystemStruct.hpp"
#include "JSContextStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSEntityCreateInfo.hpp"
#include "JSPositionListener.hpp"
#include "JSPresenceStruct.hpp"
#include "../JSLogging.hpp"


namespace Sirikata{
namespace JS{


JSSystemStruct::JSSystemStruct ( JSContextStruct* jscont, uint32 capNum)
 : associatedContext(jscont),
   canSend(capNum & Capabilities::SEND_MESSAGE),
   canRecv(capNum & Capabilities::RECEIVE_MESSAGE),
   canImport(capNum & Capabilities::IMPORT),
   canCreatePres(capNum & Capabilities::CREATE_PRESENCE),
   canCreateEnt(capNum & Capabilities::CREATE_ENTITY),
   canEval(capNum & Capabilities::EVAL),
   canProxCallback(capNum & Capabilities::PROX_CALLBACKS),
   canProxChangeQuery(capNum & Capabilities::PROX_QUERIES),
   canCreateSandbox(capNum & Capabilities::CREATE_SANDBOX),
   canGui(capNum & Capabilities::GUI),
   canHttp(capNum & Capabilities::HTTP)
{
}

v8::Handle<v8::Value> JSSystemStruct::struct_evalInGlobal(const String& native_contents, ScriptOrigin* sOrigin)
{
    if (!canEval)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability tocall eval in global directly.")));

    return associatedContext->struct_evalInGlobal(native_contents,sOrigin);
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

v8::Handle<v8::Value> JSSystemStruct::httpRequest(Sirikata::Network::Address addr, Transfer::HttpManager::HTTP_METHOD method, String request, v8::Persistent<v8::Function> cb)
{
    return associatedContext->httpRequest(addr,method,request,cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageErase(const OH::Storage::Key& key, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageErase(key, cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageRead(const OH::Storage::Key& key, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageRead(key, cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageRangeRead(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageRangeRead(start, finish, cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageRangeErase(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageRangeErase(start, finish, cb);
}

v8::Handle<v8::Value> JSSystemStruct::storageCount(const OH::Storage::Key& start, const OH::Storage::Key& finish, v8::Handle<v8::Function> cb)
{
    return associatedContext->storageCount(start, finish, cb);
}

v8::Handle<v8::Value> JSSystemStruct::sendSandbox(const String& msgToSend, JSContextStruct* destination)
{
    return associatedContext->sendSandbox(msgToSend,destination);
}


v8::Handle<v8::Value> JSSystemStruct::setRestoreScript(const String& key, v8::Handle<v8::Function> cb)
{
    return associatedContext->setRestoreScript(key, cb);
}


v8::Handle<v8::Value> JSSystemStruct::emersonCompileString(const String& toCompile)
{
    return associatedContext->emersonCompileString(toCompile);
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
    if (! canCreatePres)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to create presences.")));

    return associatedContext->restorePresence(psrp);
}


JSSystemStruct::~JSSystemStruct()
{
}


v8::Handle<v8::Value> JSSystemStruct::deserialize(const String& toDeserialize)
{
    return associatedContext->deserialize(toDeserialize);
}

v8::Handle<v8::Value> JSSystemStruct::checkHeadless()
{
    return associatedContext->checkHeadless();
}


v8::Handle<v8::Value> JSSystemStruct::struct_require(const String& toRequireFrom,bool isJS)
{
    //require uses the same capability as import
    if (! canImport)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to require.")));

    return associatedContext->struct_require(toRequireFrom,isJS);
}

v8::Handle<v8::Value> JSSystemStruct::struct_import(const String& toImportFrom,bool isJS)
{
    if (! canImport)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to import.")));

    return associatedContext->struct_import(toImportFrom,isJS);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canImport()
{
    return v8::Boolean::New(canImport);
}


v8::Handle<v8::Value> JSSystemStruct::sendMessageNoErrorHandler(JSPresenceStruct* jspres, const String& serialized_message,JSPositionListener* jspl,bool reliable)
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

    return associatedContext->sendMessageNoErrorHandler(jspres,serialized_message,jspl,reliable);
}




v8::Handle<v8::Value> JSSystemStruct::setSandboxMessageCallback(v8::Persistent<v8::Function> callback)
{
    return associatedContext->setSandboxMessageCallback(callback);
}

v8::Handle<v8::Value> JSSystemStruct::setPresenceMessageCallback(v8::Persistent<v8::Function> callback)
{
    return associatedContext->setPresenceMessageCallback(callback);
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
v8::Handle<v8::Value> JSSystemStruct::struct_createContext(JSPresenceStruct* jspres,const SpaceObjectReference& canSendTo, uint32 permNum)
{
    //prevents scripter from escalating capabilities beyond those that he/she
    //already has
    INLINE_CAPABILITY_STRIP(permNum,SEND_MESSAGE,canSend);
    INLINE_CAPABILITY_STRIP(permNum,RECEIVE_MESSAGE,canRecv);
    INLINE_CAPABILITY_STRIP(permNum,IMPORT,canImport);
    INLINE_CAPABILITY_STRIP(permNum,CREATE_PRESENCE,canCreatePres);
    INLINE_CAPABILITY_STRIP(permNum,CREATE_ENTITY,canCreateEnt);
    INLINE_CAPABILITY_STRIP(permNum,EVAL,canEval);
    INLINE_CAPABILITY_STRIP(permNum,PROX_CALLBACKS,canProxCallback);
    INLINE_CAPABILITY_STRIP(permNum,PROX_QUERIES,canProxChangeQuery);
    INLINE_CAPABILITY_STRIP(permNum,CREATE_SANDBOX,canCreateSandbox);
    INLINE_CAPABILITY_STRIP(permNum,GUI,canGui);
    INLINE_CAPABILITY_STRIP(permNum,HTTP,canHttp);

    return associatedContext->struct_createContext(jspres,canSendTo,permNum);
}



v8::Handle<v8::Value> JSSystemStruct::struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    return associatedContext->struct_registerOnPresenceConnectedHandler(cb_persist);
}

v8::Handle<v8::Value> JSSystemStruct::struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist)
{
    return associatedContext->struct_registerOnPresenceDisconnectedHandler(cb_persist);
}


v8::Handle<v8::Value> JSSystemStruct::struct_setScript(const String& script)
{
    return associatedContext->struct_setScript(script);
}

v8::Handle<v8::Value> JSSystemStruct::struct_getScript()
{
    return associatedContext->struct_getScript();
}

v8::Handle<v8::Value> JSSystemStruct::struct_reset(const std::map<SpaceObjectReference, std::vector<SpaceObjectReference> > & proxResSet)
{
    return associatedContext->struct_setReset(proxResSet);
}


v8::Handle<v8::Value> JSSystemStruct::struct_event(v8::Persistent<v8::Function>& cb) {
    return associatedContext->struct_event(cb);
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
v8::Handle<v8::Value> JSSystemStruct::struct_createEntity(EntityCreateInfo& eci)
{
    if (! canCreateEnt)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  You do not have the capability to create entities.")));

    return associatedContext->struct_createEntity(eci);
}





v8::Handle<v8::Value> JSSystemStruct::struct_canSendMessage()
{
    return v8::Boolean::New(canSend);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canRecvMessage()
{
    return v8::Boolean::New(canRecv);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canProxCallback()
{
    return v8::Boolean::New(canProxCallback);
}

v8::Handle<v8::Value> JSSystemStruct::struct_canProxChangeQuery()
{
    return v8::Boolean::New(canProxChangeQuery);
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
