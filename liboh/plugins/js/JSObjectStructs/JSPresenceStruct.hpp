#ifndef __SIRIKATA_JS_PRESENCE_STRUCT_HPP__
#define __SIRIKATA_JS_PRESENCE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>

#include "JSContextStruct.hpp"
#include "JSSuspendable.hpp"
#include "JSPositionListener.hpp"

namespace Sirikata {
namespace JS {


struct PresStructRestoreParams
{
    PresStructRestoreParams(SpaceObjectReference* sporef,TimedMotionVector3f* tmv3f,TimedMotionQuaternion* tmq,String* mesh,double* scale,bool *isCleared,uint32* contID,bool* isConnected,v8::Handle<v8::Function>* connCallback,bool* isSuspended,Vector3f* suspendedVelocity,Quaternion* suspendedOrientationVelocity, SolidAngle* sa)
        : mSporef(sporef),
          mTmv3f(tmv3f),
          mTmq(tmq),
          mMesh(mesh),
          mScale(scale),
          mIsCleared(isCleared),
          mContID(contID),
          mIsConnected(isConnected),
          mConnCallback(connCallback),
          mIsSuspended(isSuspended),
          mSuspendedVelocity(suspendedVelocity),
          mSuspendedOrientationVelocity(suspendedOrientationVelocity),
          mQuery(sa)
    {
    }

    SpaceObjectReference* mSporef;
    TimedMotionVector3f* mTmv3f;
    TimedMotionQuaternion* mTmq;
    String* mMesh;
    double* mScale;
    bool *mIsCleared;
    uint32* mContID;
    bool* mIsConnected;
    v8::Handle<v8::Function>* mConnCallback;
    bool* mIsSuspended;
    Vector3f* mSuspendedVelocity;
    Quaternion* mSuspendedOrientationVelocity;
    SolidAngle* mQuery;
};




//need to forward-declare this so that can reference this inside
class EmersonScript;


//note: only position and isConnected will actually set the flag of the watchable
struct JSPresenceStruct : public JSPositionListener,
                          public JSSuspendable
{
    //isConnected is false using this: have no sporef.
    JSPresenceStruct(EmersonScript* parent,v8::Handle<v8::Function> onConnected,JSContextStruct* ctx, HostedObject::PresenceToken presenceToken);

    //Already have a sporef (ie, turn an entity in the world that wasn't built
    //for scripting into one that is)
    JSPresenceStruct(EmersonScript* parent, const SpaceObjectReference& _sporef, JSContextStruct* ctx,HostedObject::PresenceToken presenceToken);

    //restoration constructor
    JSPresenceStruct(EmersonScript* parent,PresStructRestoreParams& psrp,Vector3f center, HostedObject::PresenceToken presToken,JSContextStruct* jscont);

    virtual void fixupSuspendable()
    {}
    
    ~JSPresenceStruct();


    /**
       @param {SpaceObjectReference} _sporef The new sporef for this object now
       that it is connected.
       @param {JSProxyPtr} newJPP Before this presenceStruct was connected, we
       didn't have a sporef, and therefore, had to use an empty proxy ptr in the
       positionlistener for this presence.  Now that we know sporef, we also
       know the proxy ptr should set in position listener.  
     */
    void connect(const SpaceObjectReference& _sporef, std::tr1::shared_ptr<JSProxyData> newJPP);
    void disconnectCalledFromObjScript();


    virtual v8::Handle<v8::Value> suspend();
    virtual v8::Handle<v8::Value> resume();
    //currently, cannot be called by scripter.
    virtual v8::Handle<v8::Value> clear();

    v8::Handle<v8::Value> disconnect();

    static JSPresenceStruct* decodePresenceStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);

    v8::Handle<v8::Value> getAllData();

    v8::Handle<v8::Value> doneRestoring();
    
    bool getIsConnected();
    v8::Handle<v8::Value> getIsConnectedV8();
    v8::Handle<v8::Value> setConnectedCB(v8::Handle<v8::Function> newCB);


    v8::Handle<v8::Value> struct_createContext(SpaceObjectReference canMessage, bool sendEveryone,bool recvEveryone,bool proxQueries,bool canImport,bool canCreatePres, bool canCreateEnt,bool canEval);


    void addAssociatedContext(JSContextStruct*);

    v8::Persistent<v8::Function> mOnConnectedCallback;

    HostedObject::PresenceToken getPresenceToken();

    SolidAngle getQueryAngle();
    v8::Handle<v8::Value>  struct_getQueryAngle();
    v8::Handle<v8::Value>  setQueryAngleFunction(SolidAngle new_qa);
    v8::Handle<v8::Value>  setOrientationVelFunction(Quaternion newOrientationVel);
    v8::Handle<v8::Value>  struct_setVelocity(const Vector3f& newVel);
    v8::Handle<v8::Value>  struct_setPosition(Vector3f newPos);
    v8::Handle<v8::Value>  setVisualScaleFunction(float new_scale);
    v8::Handle<v8::Value>  setVisualFunction(String urilocation);
    v8::Handle<v8::Value>  setOrientationFunction(Quaternion newOrientation);



    v8::Handle<v8::Value>  getPhysicsFunction();
    v8::Handle<v8::Value>  setPhysicsFunction(const String& loc);

    //returns this presence as a visible object.
    v8::Persistent<v8::Object>  toVisible();


    v8::Handle<v8::Value>  runSimulation(String simname);

    v8::Handle<v8::Value>  toString()
    {
        v8::HandleScope handle_scope;
        String sporefReturner = getSporef().toString();
        return v8::String::New(sporefReturner.c_str(), sporefReturner.length());
    }

private:
    uint32 mContID;
    
    //this function checks if we have a callback associated with this presence.
    //Then it asks jsobjectscript to call the callback
    void callConnectedCallback();

    //data
    bool isConnected;
    bool hasConnectedCallback;
    HostedObject::PresenceToken mPresenceToken;

    TimedMotionVector3f mLocation;
    TimedMotionQuaternion mOrientation;

    //These two pieces of state hold the values for a presence's
    //velocity and orientation velocity when suspend was called.
    //on resume, use these velocities to resume from.
    Vector3f   mSuspendedVelocity;
    Quaternion mSuspendedOrientationVelocity;

    
    JSContextStruct* mContext;
    SolidAngle mQuery;
    
    ContextVector associatedContexts;
    void clearPreviousConnectedCB();

#define checkCleared(funcName)  \
    String fname (funcName);    \
    if (getIsCleared())         \
    {                           \
        String errorMessage = "Error when calling " + fname + " on presence.  The presence has already been cleared."; \
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str()))); \
    }
};

typedef std::vector<JSPresenceStruct*> JSPresVec;
typedef JSPresVec::iterator JSPresVecIter;

}//end namespace js
}//end namespace sirikata

#endif
