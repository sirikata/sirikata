#ifndef _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#define _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#include "ProxyObject.hpp"
namespace Sirikata {

  typedef double AbsTime;

/**
 * This class represents a generic object on a remote server
 * Every object has a SpaceObjectReference that allows one to communicate
 * with it. Subclasses implement several Providers for concerned listeners
 * This class should be casted to the various subclasses (ProxyLightObject,etc)
 * and appropriate listeners be registered.
 */
class SIRIKATA_OH_EXPORT ProxyPositionObject : public ProxyObject {
    Vector3d mPosition;
    Quaternion mOrientation;
    Vector3f mPositionPerSecond;
    Quaternion mOrientationPerSecond;
    AbsTime mLastUpdated;
public:
    ProxyPositionObject()
        : mPosition(-1,-1,-1),
          mOrientation(-1,-1,-1,-1),
          mPositionPerSecond(0,0,0),
          mOrientationPerSecond(0,0,0,0),
          mLastUpdated(0.) {
    }
    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    const Vector3d&getPosition() const{
        return mPosition;
    }
    const Quaternion&getOrientation() const{
        return mOrientation;
    }
    AbsTime getLastUpdated() const {
        return mLastUpdated;
    }
    bool isStatic() {
      return mPositionPerSecond.lengthSquared()==0 &&
        mOrientationPerSecond.lengthSquared()==0;
    }
    void setPosition(AbsTime timeStamp,
            const Vector3d &newPos,
            const Quaternion &newOri) {
        mPosition = newPos;
        mOrientation = newOri;
        mLastUpdated = timeStamp;
        mPositionPerSecond = Vector3f(0,0,0);
        mOrientationPerSecond = Quaternion(0,0,0,0);
    }
    void setPositionVelocity(AbsTime timeStamp,
            const Vector3d &newPos,
            const Quaternion &newOri,
            const Vector3f &newVel,
            const Quaternion &newAngVel) {
        mPosition = newPos;
        mOrientation = newOri;
        mLastUpdated = timeStamp;
        mPositionPerSecond = newVel;
        mOrientationPerSecond = newAngVel;
    }
    Vector3d extrapolatePosition(AbsTime current) const {
      return mPosition + (mPositionPerSecond * (current - mLastUpdated)).convert<Vector3d>();
    }
    Quaternion extrapolateOrientation(AbsTime current) const {
        return mOrientation + (mOrientationPerSecond * (current - mLastUpdated));
    }
};
}
#endif
