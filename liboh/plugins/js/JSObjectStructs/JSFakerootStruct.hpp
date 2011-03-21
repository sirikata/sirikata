#ifndef __SIRIKATA_JS_FAKEROOT_STRUCT_HPP__
#define __SIRIKATA_JS_FAKEROOT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include "../JSPattern.hpp"
#include "../JSEntityCreateInfo.hpp"

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSContextStruct;
class JSEventHandlerStruct;
class JSPresenceStruct;

struct JSFakerootStruct
{
    JSFakerootStruct(JSContextStruct* jscont, bool send, bool receive, bool prox,bool import,bool createPres, bool createEntity);
    ~JSFakerootStruct();

    static JSFakerootStruct* decodeRootStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    //regular members
    v8::Handle<v8::Value> struct_canSendMessage();
    v8::Handle<v8::Value> struct_canRecvMessage();
    v8::Handle<v8::Value> struct_canProx();
    v8::Handle<v8::Value> struct_canImport();
    
    v8::Handle<v8::Value> struct_getPosition();


    v8::Handle<v8::Value> struct_print(const String& msg);    
    v8::Handle<v8::Value> struct_sendHome(const String& toSend);
    v8::Handle<v8::Value> struct_import(const String& toImportFrom);

    //returns the presence that is associated with the jscontext
    // v8::Handle<v8::Value> struct_getPresence();
    // JSPresenceStruct* struct_getPresenceCPP();


    
    //if have the capability to create presences, create a new presence with
    //mesh newMesh and executes initFunc, which gets executed onConnected.
    //if do not have the capability, throws an error.
    v8::Handle<v8::Value> struct_createPresence(const String& newMesh, v8::Handle<v8::Function> initFunc);

    //if have the capability to create presences, create a new presence with
    //mesh newMesh and executes initFunc, which gets executed onConnected.
    //if do not have the capability, throws an error.
    v8::Handle<v8::Value> struct_createEntity(EntityCreateInfo& eci);
    
    v8::Handle<v8::Value> struct_makeEventHandlerObject(const PatternList& native_patterns,v8::Persistent<v8::Object> target_persist, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist);

    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool import,bool createPres,bool createEnt, JSPresenceStruct* presStruct);
    
    JSContextStruct* getContext();

    v8::Handle<v8::Value> struct_registerOnPresenceConnectedHandler(v8::Persistent<v8::Function> cb_persist);
    v8::Handle<v8::Value> struct_registerOnPresenceDisconnectedHandler(v8::Persistent<v8::Function> cb_persist);
    
    
private:
    //associated data 
    JSContextStruct* associatedContext;
    bool canSend, canRecv, canProx,canImport,canCreatePres,canCreateEnt;
};


}//end namespace js
}//end namespace sirikata

#endif
