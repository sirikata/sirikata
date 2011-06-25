#ifndef __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__
#define __SIRIKATA_JS_POSITION_LISTENER_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>

#include "JSContextStruct.hpp"


namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class EmersonScript;

struct VisAddParams
{
    VisAddParams(const SpaceObjectReference* sporefWatchFrom, const TimedMotionVector3f* loc, const TimedMotionQuaternion* orient, const BoundingSphere3f* bounds, const String* mesh, const String* physics, const bool* isVisible)
     : mSporefWatchingFrom (sporefWatchFrom),
       mLocation(loc),
       mOrientation(orient),
       mBounds(bounds),
       mMesh(mesh),
       mPhysics(physics),
       mIsVisible(isVisible)
    {}
    VisAddParams(const bool* isVisible)
     : mSporefWatchingFrom (NULL),
       mLocation(NULL),
       mOrientation(NULL),
       mBounds(NULL),
       mMesh(NULL),
       mPhysics(NULL),
       mIsVisible(isVisible)
    {}


    const SpaceObjectReference*          mSporefWatchingFrom;
    const TimedMotionVector3f*                     mLocation;
    const TimedMotionQuaternion*                mOrientation;
    const BoundingSphere3f*                          mBounds;
    const String*                                      mMesh;
    const String*                                   mPhysics;
    const bool*                                   mIsVisible;
};



//note: only position and isConnected will actually set the flag of the watchable
struct JSPositionListener : public PositionListener,
                            public MeshListener
{
    friend class JSSerializer;

    JSPositionListener(EmersonScript* script, VisAddParams* addParams);
    ~JSPositionListener();

    //objToListenTo for presence objects contains the sporef following
    //objToListenFrom for presence object contains null
    void setListenTo(const SpaceObjectReference* objToListenTo,const SpaceObjectReference* objToListenFrom);

    virtual Vector3f     getPosition();
    virtual Vector3f     getVelocity();
    virtual Quaternion   getOrientation();
    virtual Quaternion   getOrientationVelocity();
    virtual BoundingSphere3f   getBounds();
    virtual String getMesh();
    virtual String getPhysics();

    virtual v8::Handle<v8::Value> struct_getPosition();
    virtual v8::Handle<v8::Value> struct_getVelocity();
    virtual v8::Handle<v8::Value> struct_getOrientation();
    virtual v8::Handle<v8::Value> struct_getOrientationVel();
    virtual v8::Handle<v8::Value> struct_getScale();
    virtual v8::Handle<v8::Value> struct_getMesh();
    virtual v8::Handle<v8::Value> struct_getPhysics();
    virtual v8::Handle<v8::Value> struct_getTransTime();
    virtual v8::Handle<v8::Value> struct_getOrientTime();
    virtual v8::Handle<v8::Value> struct_getSporefListeningTo();
    virtual v8::Handle<v8::Value> struct_getSporefListeningFrom();

    virtual v8::Handle<v8::Object> struct_getAllData();

    virtual v8::Handle<v8::Value> struct_getDistance(const Vector3d& distTo);

    //simple accessors for sporef fields
    SpaceObjectReference* getToListenTo();
    SpaceObjectReference* getToListenFrom();


    virtual void updateLocation (const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef);

    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);
    virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef);
    virtual void onSetPhysics (ProxyObjectPtr proxy, const String& newphy,const SpaceObjectReference& sporef);


    virtual void destroyed();

    //calls updateLocation on jspos, filling in mLocation, mOrientation, and mBounds
    //for the newLocation,newOrientation, and newBounds of updateLocation field.
    void updateOtherJSPosListener(JSPositionListener* jspos);

protected:
    //data
    EmersonScript*                  emerScript;
    SpaceObjectReference*     sporefToListenTo;
    SpaceObjectReference*   sporefToListenFrom;

    TimedMotionVector3f              mLocation;
    TimedMotionQuaternion         mOrientation;
    BoundingSphere3f                   mBounds;
    String                               mMesh;
    String                            mPhysics;


    //registers/deregisters position listener with associated jsobjectscript
    bool registerAsPosAndMeshListener();
    void deregisterAsPosAndMeshListener();

    v8::Handle<v8::Value> wrapSporef(SpaceObjectReference* sporef);

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
