#ifndef __SIRIKATA_JS_PRESENCE_STRUCT_HPP__
#define __SIRIKATA_JS_PRESENCE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>

#include "JSContextStruct.hpp"

namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;

struct JSPresenceStruct
{
    
    JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef);
    ~JSPresenceStruct();

    void registerOnProxAddedEventHandler(v8::Persistent<v8::Function>& cb);
    void registerOnProxRemovedEventHandler(v8::Persistent<v8::Function>& cb);


    static JSPresenceStruct* decodePresenceStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);


    v8::Handle<v8::Value> struct_getPosition();
    v8::Handle<v8::Value> struct_setVelocity(const Vector3f& newVel);
    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries);

    v8::Handle<v8::Value> struct_broadcastVisible(v8::Handle<v8::Object> toBroadcast);
    
    void addAssociatedContext(JSContextStruct*);

    
    //data
    JSObjectScript* jsObjScript;
    SpaceObjectReference* sporef; //sporef associated with this presence.
    v8::Persistent<v8::Function> mOnProxRemovedEventHandler;
    v8::Persistent<v8::Function> mOnProxAddedEventHandler;

    
    typedef std::vector<JSContextStruct*> ContextVector;
    ContextVector associatedContexts;
    
};


}//end namespace js
}//end namespace sirikata

#endif
