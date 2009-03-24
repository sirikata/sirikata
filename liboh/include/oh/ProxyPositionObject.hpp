#ifndef _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#define _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#include <util/Extrapolation.hpp>
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
    class UpdateNeeded {
    public:
        bool operator()(const Location&updatedValue, const Location&predictedValue) const{
            Vector3f ux,uy,uz,px,py,pz;
            updatedValue.getOrientation().toAxes(ux,uy,uz);
            predictedValue.getOrientation().toAxes(px,py,pz);
            return (updatedValue.getPosition()-predictedValue.getPosition()).lengthSquared()>1.0||
                ux.dot(px)<.9||uy.dot(py)<.9||uz.dot(pz)<.9;
        }
    };
     TimedWeightedExtrapolator<Location,UpdateNeeded> mLocation;
public:
     ProxyPositionObject(const SpaceObjectReference&id):ProxyObject(id),mLocation(Duration::seconds(.1),TemporalValue<Location>::Time::null(),Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),Vector3f(0,0,0),Vector3f(0,1,0),0),UpdateNeeded()) {
    }
    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    const Vector3d& getPosition() const{
        return mLocation.lastValue().getPosition();
    }
    const Quaternion& getOrientation() const{
        return mLocation.lastValue().getOrientation();
    }
    TemporalValue<Location>::Time getLastUpdated() const {
        return mLocation.lastUpdateTime();
    }
    class IsLocationStatic{
    public:
        bool operator() (const Location&l) const {
            return l.getVelocity()==Vector3f(0.0,0.0,0.0)
                &&(l.getAxisOfRotation()==Vector3f(0.0,0.0,0.0)
                   || l.getAngularSpeed()==0.0);
        }
    };
    bool isStatic(const TemporalValue<Location>::Time& when) const {
        return mLocation.templatedPropertyHolds(when,IsLocationStatic());
    }
    void setPosition(TemporalValue<Location>::Time timeStamp,
                     const Vector3d &newPos,
                     const Quaternion &newOri) {
        Location soon=mLocation.extrapolate(timeStamp);
        mLocation.updateValue(timeStamp,
                              Location(newPos,
                                       newOri,
                                       soon.getVelocity(),
                                       soon.getAxisOfRotation(),
                                       soon.getAngularSpeed()));
    }
    void setPositionVelocity(TemporalValue<Location>::Time timeStamp,
                             const Location&location) {
        mLocation.updateValue(timeStamp,
                              location);
    }
    void resetPositionVelocity(TemporalValue<Location>::Time timeStamp,
                               const Location&location) {
        mLocation.resetValue(timeStamp,
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
