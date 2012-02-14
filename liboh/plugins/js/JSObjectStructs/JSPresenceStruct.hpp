#ifndef __SIRIKATA_JS_PRESENCE_STRUCT_HPP__
#define __SIRIKATA_JS_PRESENCE_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>

#include "JSContextStruct.hpp"
#include "JSSuspendable.hpp"
#include "JSPositionListener.hpp"
#include <sirikata/core/util/Nullable.hpp>
#include "../JSObjects/JSObjectsUtils.hpp"
#include "JSCapabilitiesConsts.hpp"
#include "../JSCtx.hpp"

namespace Sirikata {
namespace JS {


struct PresStructRestoreParams
{
    PresStructRestoreParams(const SpaceObjectReference& _sporef,
        const Nullable<Time>& _positionTime,
        const Vector3f& _position,
        const Vector3f& _velocity,
        const Nullable<Time>& _orientTime,
        const Quaternion& _orient,
        const Quaternion& _orientVelocity,
        const String& _mesh,
        const String& _physics,
        const double& _scale,
        const bool& _isCleared,
        const Nullable<uint32>& _contID,
        const bool& _isConnected,
        const Nullable<v8::Handle<v8::Function> >& _connCallback,
        const bool& _isSuspended,
        const Nullable<Vector3f>& _suspendedVelocity,
        const Nullable<Quaternion>& _suspendedOrientationVelocity,
        const String& _query)
     :     sporef(_sporef),
           positionTime(_positionTime),
           position(_position),
           velocity(_velocity),
           orientTime(_orientTime),
           orient(_orient),
           orientVelocity(_orientVelocity),
           mesh(_mesh),
           physics(_physics),
           scale(_scale),
           isCleared(_isCleared),
           contID(_contID),
           isConnected(_isConnected),
           connCallback(_connCallback),
           isSuspended(_isSuspended),
           suspendedVelocity(_suspendedVelocity),
           suspendedOrientationVelocity(_suspendedOrientationVelocity),
           query(_query)
    {
    }

    SpaceObjectReference sporef;
    Nullable<Time> positionTime;
    Vector3f position;
    Vector3f velocity;
    Nullable<Time> orientTime;
    Quaternion orient;
    Quaternion orientVelocity;
    String mesh;
    String physics;
    double scale;
    bool isCleared;
    Nullable<uint32> contID;
    bool isConnected;
    Nullable<v8::Handle<v8::Function> > connCallback;
    bool isSuspended;
    Nullable<Vector3f> suspendedVelocity;
    Nullable<Quaternion> suspendedOrientationVelocity;
    String query;
};


//need to forward-declare this so that can reference this inside
class EmersonScript;


//note: only position and isConnected will actually set the flag of the watchable
struct JSPresenceStruct : public JSPositionListener,
                          public JSSuspendable
{
    //isConnected is false using this: have no sporef.
    JSPresenceStruct(EmersonScript* parent,v8::Handle<v8::Function> onConnected,JSContextStruct* ctx, HostedObject::PresenceToken presenceToken,JSCtx*);

    //Already have a sporef (ie, turn an entity in the world that wasn't built
    //for scripting into one that is)
    JSPresenceStruct(EmersonScript* parent, const SpaceObjectReference& _sporef, JSContextStruct* ctx,HostedObject::PresenceToken presenceToken,JSCtx*);

    //restoration constructor
    JSPresenceStruct(EmersonScript* parent,PresStructRestoreParams& psrp,Vector3f center, HostedObject::PresenceToken presToken,JSContextStruct* jscont, const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq,JSCtx*);


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
    void connect(const SpaceObjectReference& _sporef);

    void markDisconnected();
    void handleDisconnectedCallback();


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


    void addAssociatedContext(JSContextStruct*);

    v8::Persistent<v8::Function> mOnConnectedCallback;

    HostedObject::PresenceToken getPresenceToken();

    String getQuery();
    v8::Handle<v8::Value> struct_getQuery();
    v8::Handle<v8::Value> setQueryFunction(const String& new_query);


    v8::Handle<v8::Value>  setOrientationVelFunction(Quaternion newOrientationVel);
    v8::Handle<v8::Value>  struct_setVelocity(const Vector3f& newVel);
    v8::Handle<v8::Value>  struct_setPosition(Vector3f newPos);
    v8::Handle<v8::Value>  setVisualScaleFunction(float new_scale);
    v8::Handle<v8::Value>  setVisualFunction(String urilocation);
    v8::Handle<v8::Value>  setOrientationFunction(Quaternion newOrientation);



    v8::Handle<v8::Value>  setPhysicsFunction(const String& loc);

    //returns this presence as a visible object.
    v8::Local<v8::Object>  toVisible();


    v8::Handle<v8::Value>  runSimulation(String simname);

    v8::Handle<v8::Value>  toString()
    {
        v8::HandleScope handle_scope;
        String sporefReturner = getSporef().toString();
        return v8::String::New(sporefReturner.c_str(), sporefReturner.length());
    }

private:
    EmersonScript* mParent;
    uint32 mContID;

    //this function checks if we have a callback associated with this presence.
    //Then it asks jsobjectscript to call the callback
    void callConnectedCallback();

    //data
    bool isConnected;
    bool hasConnectedCallback;
    HostedObject::PresenceToken mPresenceToken;

    //These two pieces of state hold the values for a presence's
    //velocity and orientation velocity when suspend was called.
    //on resume, use these velocities to resume from.
    Vector3f   mSuspendedVelocity;
    Quaternion mSuspendedOrientationVelocity;


    JSContextStruct* mContext;
    String mQuery;

    ContextVector associatedContexts;
    void clearPreviousConnectedCB();
};

#define checkCleared(funcName)  \
    String fname (funcName);    \
    if (getIsCleared())         \
    {                           \
        String errorMessage = "Error when calling " + fname + " on presence.  The presence has already been cleared."; \
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str()))); \
    }

//returns a v8 exception if the current context we're executing from does not
//have the capability (whatCap) to preform operation on this JSPresenceStruct.
#define INLINE_CHECK_CAPABILITY_ERROR(whatCap,where)                    \
{                                                                       \
    if (! mContext->jsObjScript->checkCurCtxtHasCapability(this, whatCap))        \
        V8_EXCEPTION_CSTR("Error in " #where " you do not have the capability for " #whatCap " on this presence."); \
}

#define INLINE_CHECK_IS_CONNECTED_ERROR(where)                          \
    if (! isConnected)                                                  \
        V8_EXCEPTION_CSTR("Error in " #where " presence was disconnected when called.");

#define INLINE_PRESENCE_CONV_ERROR(toConvert,whereError,whichArg,whereWriteTo) \
    JSPresenceStruct* whereWriteTo;                                     \
    {                                                                   \
        String _errMsg = "In " #whereError "cannot convert " #whichArg " to presence struct"; \
        whereWriteTo = JSSystemStruct::decodeSystemStruct(toConvert,_errMsg); \
        if (whereWriteTo == NULL)                                       \
            return v8::ThrowException(v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length()))); \
    }



typedef std::vector<JSPresenceStruct*> JSPresVec;
typedef JSPresVec::iterator JSPresVecIter;

}//end namespace js
}//end namespace sirikata

#endif
