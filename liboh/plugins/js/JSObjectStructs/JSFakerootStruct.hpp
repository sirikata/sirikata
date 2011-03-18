#ifndef __SIRIKATA_JS_FAKEROOT_STRUCT_HPP__
#define __SIRIKATA_JS_FAKEROOT_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include "../JSPattern.hpp"


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSContextStruct;
class JSEventHandlerStruct;
class JSPresenceStruct;

struct JSFakerootStruct
{
    JSFakerootStruct(JSContextStruct* jscont, bool send, bool receive, bool prox,bool import);
    ~JSFakerootStruct();

    static JSFakerootStruct* decodeRootStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    //regular members
    v8::Handle<v8::Value> struct_canSendMessage();
    v8::Handle<v8::Value> struct_canRecvMessage();
    v8::Handle<v8::Value> struct_canProx();
    v8::Handle<v8::Value> struct_canImport();
    
    v8::Handle<v8::Value> struct_getPosition();

    //returns the presence that is associated with the jscontext
    v8::Handle<v8::Value> struct_getPresence();
    v8::Handle<v8::Value> struct_print(const String& msg);    
    v8::Handle<v8::Value> struct_sendHome(const String& toSend);
    v8::Handle<v8::Value> struct_import(const String& toImportFrom);


    JSPresenceStruct* struct_getPresenceCPP();
    
    v8::Handle<v8::Value> struct_makeEventHandlerObject(const PatternList& native_patterns,v8::Persistent<v8::Object> target_persist, v8::Persistent<v8::Function> cb_persist, v8::Persistent<v8::Object> sender_persist);

    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport,JSPresenceStruct* presStruct);
    
    JSContextStruct* getContext();
    
    //associated data 
    JSContextStruct* associatedContext;
    bool canSend, canRecv, canProx,canImport;
};


}//end namespace js
}//end namespace sirikata

#endif
