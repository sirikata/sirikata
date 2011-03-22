#ifndef __SIRIKATA_JS_PRESENCE_STRUCT_HPP__
#define __SIRIKATA_JS_PRESENCE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>
#include "JSPositionListener.hpp"
#include "JSContextStruct.hpp"


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;


//note: only position and isConnected will actually set the flag of the watchable
struct JSPresenceStruct : public JSPositionListener
{
    //isConnected is false using this: have no sporef.
    JSPresenceStruct(JSObjectScript* parent,v8::Handle<v8::Function> onConnected,JSContextStruct* ctx, HostedObject::PresenceToken presenceToken); 
    JSPresenceStruct(JSObjectScript* parent, const SpaceObjectReference& _sporef, JSContextStruct* ctx,HostedObject::PresenceToken presenceToken);
    ~JSPresenceStruct();


    void connect(const SpaceObjectReference& _sporef);
    void disconnect();

    v8::Handle<v8::Value> registerOnProxRemovedEventHandler(v8::Handle<v8::Function>cb);
    v8::Handle<v8::Value> registerOnProxAddedEventHandler(v8::Handle<v8::Function> cb);

    static JSPresenceStruct* decodePresenceStruct(v8::Handle<v8::Value> toDecode,std::string& errorMessage);


    
    bool getIsConnected();
    v8::Handle<v8::Value> getIsConnectedV8();
    v8::Handle<v8::Value> setConnectedCB(v8::Handle<v8::Function> newCB);


    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference* canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport,bool canCreatePres, bool canCreateEnt,bool canEval);


    void addAssociatedContext(JSContextStruct*);

    v8::Persistent<v8::Function> mOnProxRemovedEventHandler;
    v8::Persistent<v8::Function> mOnProxAddedEventHandler;
    v8::Persistent<v8::Function> mOnConnectedCallback;

    HostedObject::PresenceToken getPresenceToken();

    v8::Handle<v8::Value>  setQueryAngleFunction(SolidAngle new_qa);
    v8::Handle<v8::Value>  setOrientationVelFunction(Quaternion newOrientationVel);
    v8::Handle<v8::Value>  struct_setVelocity(const Vector3f& newVel);
    v8::Handle<v8::Value>  struct_setPosition(Vector3f newPos);
    v8::Handle<v8::Value>  setVisualScaleFunction(float new_scale);
    v8::Handle<v8::Value>  setVisualFunction(String urilocation);
    v8::Handle<v8::Value>  setOrientationFunction(Quaternion newOrientation);

    v8::Handle<v8::Value>  getVisualFunction();


    v8::Handle<v8::Value>  runSimulation(String simname);

    v8::Handle<v8::Value>  toString()
    {
        v8::HandleScope handle_scope;
        String sporefReturner = "Presence unconnected";
        if (sporefToListenTo != NULL)
            sporefReturner = sporefToListenTo->toString();
        return v8::String::New(sporefReturner.c_str(), sporefReturner.length());
    }

    SpaceObjectReference* getSporef()
    {
        return getToListenTo();
    }


private:

    //this function checks if we have a callback associated with this presence.
    //Then it asks jsobjectscript to call the callback
    void callConnectedCallback();
    

    
    //data
    bool isConnected;
    bool hasConnectedCallback;
    HostedObject::PresenceToken mPresenceToken;

    TimedMotionVector3f mLocation;
    TimedMotionQuaternion mOrientation;

    JSContextStruct* mContext;

    ContextVector associatedContexts;
    void clearPreviousConnectedCB();
};


}//end namespace js
}//end namespace sirikata

#endif
