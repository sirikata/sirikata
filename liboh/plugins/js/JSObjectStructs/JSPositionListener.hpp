#ifndef __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__
#define __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>

#include "JSContextStruct.hpp"
namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;


//note: only position and isConnected will actually set the flag of the watchable
struct JSPositionListener : public PositionListener
{
    friend class JSSerializer;

    JSPositionListener(JSObjectScript* script);
    ~JSPositionListener();

    void setListenTo(const SpaceObjectReference* objToListenTo,const SpaceObjectReference* objToListenFrom);

    virtual Vector3f     getPosition();
    virtual Vector3f     getVelocity();
    virtual Quaternion   getOrientation();
    virtual Quaternion   getOrientationVelocity();
    virtual BoundingSphere3f   getBounds();

    virtual v8::Handle<v8::Value> struct_getPosition();
    virtual v8::Handle<v8::Value> struct_getVelocity();
    virtual v8::Handle<v8::Value> struct_getOrientation();
    virtual v8::Handle<v8::Value> struct_getOrientationVel();
    virtual v8::Handle<v8::Value> struct_getScale();

    virtual v8::Handle<v8::Value> struct_getDistance(const Vector3d& distTo);

    //simple accessors for sporef fields
    SpaceObjectReference* getToListenTo();
    SpaceObjectReference* getToListenFrom();

    v8::Handle<v8::Value> sendMessage (String& msgToSend);

    virtual void updateLocation (const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds);
    virtual void destroyed();

    //calls updateLocation on jspos, filling in mLocation, mOrientation, and mBounds
    //for the newLocation,newOrientation, and newBounds of updateLocation field.
    void updateOtherJSPosListener(JSPositionListener* jspos);

protected:
    //data
    JSObjectScript*                jsObjScript;
    SpaceObjectReference*     sporefToListenTo;
    SpaceObjectReference*   sporefToListenFrom;

    TimedMotionVector3f              mLocation;
    TimedMotionQuaternion         mOrientation;
    BoundingSphere3f                   mBounds;



    //registers/deregisters position listener with associated jsobjectscript
    bool registerAsPosListener();
    void deregisterAsPosListener();

private:
    //returns true if inContext and sporefToListenTo is not null
    //otherwise, returns false, and adds to error message reason it failed.  Arg
    //funcIn specifies which function is asking passErrorChecks, which gets
    //mixed inot the errorMsg string if passErrorChecks returns true.
    bool passErrorChecks(String& errorMsg, const String& funcIn );

    bool hasRegisteredListener;

};


}//end namespace js
}//end namespace sirikata

#endif
