#ifndef _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#define _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#include <util/TemporalValue.hpp>
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
    TemporalValue<Location> mLocation;
public:
    ProxyPositionObject():mLocation(TemporalValue<Location>::Time::null(),Location(Vector3d(0,0,0),Quaternion(0,0,0,1),Vector3f(0,0,0),Vector3f(0,1,0),0)) {
    }
    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    const Vector3d&getPosition() const{
        return mLocation.value().getPosition();
    }
    const Quaternion&getOrientation() const{
        return mLocation.value().getOrientation();
    }
    TemporalValue<Location>::Time getLastUpdated() const {
        return mLocation.time();
    }
    bool isStatic() const {
        return mLocation.value().getVelocity()==Vector3f(0,0,0) &&
            mLocation.value().getAngularSpeed()==0;
    }
    void setPosition(TemporalValue<Location>::Time timeStamp,
                     const Vector3d &newPos,
                     const Quaternion &newOri) {
        mLocation.updateValue(timeStamp,
                              Location(newPos,
                                       newOri,
                                       mLocation.value().getVelocity(),
                                       mLocation.value().getAxisOfRotation(),
                                       mLocation.value().getAngularSpeed()));
    }
    void setPositionVelocity(TemporalValue<Location>::Time timeStamp,
                             const Location&location) {
        mLocation.updateValue(timeStamp,
                              location);
    }
    Vector3d extrapolatePosition(TemporalValue<Location>::Time current) const {
        return mLocation.extrapolate(current).getPosition();
    }
    Quaternion extrapolateOrientation(TemporalValue<Location>::Time current) const {
        return mLocation.extrapolate(current).getOrientation();
    }
    Location extrapolateLocation(TemporalValue<Location>::Time current) const {
        return mLocation.extrapolate(current);
    }
};
}
#endif
