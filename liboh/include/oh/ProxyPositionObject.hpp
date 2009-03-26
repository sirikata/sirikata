/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  ProxyPositionObject.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#define _SIRIKATA_PROXY_POSITION_OBJECT_HPP_
#include <util/Extrapolation.hpp>
#include "ProxyObject.hpp"
#include "PositionListener.hpp"

namespace Sirikata {

typedef double AbsTime;

typedef Provider<PositionListener*> PositionProvider;

/**
 * This class represents a generic object on a remote server
 * Every object has a SpaceObjectReference that allows one to communicate
 * with it. Subclasses implement several Providers for concerned listeners
 * This class should be casted to the various subclasses (ProxyLightObject,etc)
 * and appropriate listeners be registered.
 */
class SIRIKATA_OH_EXPORT ProxyPositionObject
  : public PositionProvider,
    public ProxyObject   
{

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
    SpaceObjectReference mParentId;
public:
    ProxyPositionObject(ProxyManager *man, const SpaceObjectReference&id)
      : ProxyObject(man, id),
        mLocation(Duration::seconds(.1),
                  TemporalValue<Location>::Time::null(),
                  Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                           Vector3f(0,0,0),Vector3f(0,1,0),0),
                  UpdateNeeded()),
        mParentId(SpaceObjectReference::null())
    {
    }

    ///Returns the unique identification for this object and the space to which it is connected that gives it said name
    const Vector3d& getPosition() const{
        return mLocation.lastValue().getPosition();
    }
    const Quaternion& getOrientation() const{
        return mLocation.lastValue().getOrientation();
    }
    const SpaceObjectReference& getParent() const{
        return mParentId;
    }
    ProxyPositionObjectPtr getParentProxy() const;
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
                     const Quaternion &newOri);
    void setPositionVelocity(TemporalValue<Location>::Time timeStamp,
                             const Location&location);
    void resetPositionVelocity(TemporalValue<Location>::Time timeStamp,
                               const Location&location);
    void setParent(const ProxyPositionObjectPtr &parent,
               TemporalValue<Location>::Time timeStamp,
               const Location &absLocation,
               const Location &relLocation);
    void unsetParent(TemporalValue<Location>::Time timeStamp,
               const Location &absLocation);

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
