/*  Sirikata Utilities -- Math Library
 *  Location.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _LOCATION_HPP_
#define _LOCATION_HPP_

#include "Transform.hpp"

namespace Sirikata {

class Location: public Transform {
    Vector3<float32> mVelocity;
    Vector3<float32> mAxisOfRotation;
    float32 mAngularSpeed;

    void changeToWorld(const Location &reference) {
        setVelocity(reference.getVelocity() + reference.getOrientation() * getVelocity());
        addAngularRotation(reference.getAxisOfRotation(), reference.getAngularSpeed());
        setVelocity(getVelocity() +
                    reference.getAngularSpeed() * (
                        reference.getAxisOfRotation().cross(Vector3<float32>(getPosition()))));

        Transform temp = Transform::toWorld(reference);
        setPosition(temp.getPosition());
        setOrientation(temp.getOrientation());
    }
    void changeToLocal(const Location &reference) {
        Transform temp = Transform::toLocal(reference);
        setPosition(temp.getPosition());
        setOrientation(temp.getOrientation());

        setVelocity(getVelocity() -
                    reference.getAngularSpeed() * (
                        reference.getAxisOfRotation().cross(Vector3<float32>(getPosition()))));
        addAngularRotation(reference.getAxisOfRotation(), -reference.getAngularSpeed());
        Quaternion inverseOtherOrientation (reference.getOrientation().inverse());
        setVelocity(inverseOtherOrientation * (getVelocity() - reference.getVelocity()));
    }
public:
    Location(){}
    Location(const Vector3<float64>&position,
             const Quaternion&orientation,
             const Vector3<float32> &velocity,
             const Vector3<float32> angularVelocityAxis,
             float32 angularVelocityRadians):Transform(position,orientation),mVelocity(velocity),mAxisOfRotation(angularVelocityAxis), mAngularSpeed(angularVelocityRadians) {}
    bool operator ==(const Location&other)const {
        bool eq=getPosition()==other.getPosition();
        bool veq=other.mVelocity==mVelocity;
        bool qeq=getOrientation()==other.getOrientation();
        bool aeq=mAxisOfRotation==other.mAxisOfRotation;
        bool seq=mAngularSpeed==other.mAngularSpeed;
        return eq&&veq&&qeq&&aeq&&seq;
    }

    const Vector3<float32>&getVelocity()const {
        return mVelocity;
    }
    void setVelocity(const Vector3<float32> velocity) {
        mVelocity=velocity;
    }
    const Transform &getTransform() const {
        return *this;
    }
    const Vector3<float32>&getAxisOfRotation() const {
        return mAxisOfRotation;
    }
    void setAxisOfRotation(const Vector3<float32>&axis) {
        mAxisOfRotation=axis;
    }
    float32 getAngularSpeed() const{
        return mAngularSpeed;
    }
    void setAngularSpeed(float32 radianspersecond) {
        mAngularSpeed=radianspersecond;
    }
    void addAngularRotation(const Vector3<float32> &axis, float32 radianspersecond) {
        mAxisOfRotation=mAxisOfRotation*mAngularSpeed+axis*radianspersecond;
        mAngularSpeed=mAxisOfRotation.length();
        if (mAngularSpeed)
            mAxisOfRotation/=mAngularSpeed;
    }
    Location blend(const Location&newLocation,float32 percentNew) const{
        float32 percentOld=(1.0f-percentNew);
        Vector3<float32> angAxis=mAxisOfRotation*mAngularSpeed*percentOld;
        angAxis+=newLocation.getAxisOfRotation()*newLocation.getAngularSpeed()*percentNew;
        float angSpeed=angAxis.length();
        if (angSpeed) angAxis/=angSpeed;
        return Location (newLocation.getPosition()*percentNew+getPosition()*percentOld,
                         (newLocation.getOrientation()*percentNew+getOrientation()*percentOld).normal(),
                         newLocation.getVelocity()*percentNew+getVelocity()*percentOld,
                         angAxis,
                         angSpeed);
    }
    Location toWorld(const Location &reference) const {
        Location copy(*this);
        copy.changeToWorld(reference);
        return copy;
    }
    Location toLocal(const Location &reference) const {
        Location copy(*this);
        copy.changeToLocal(reference);
        return copy;
    }
    template<class TimeDuration> Location extrapolate(const TimeDuration&dt)const {
        return Location(getPosition()+Vector3<float64>(getVelocity())*dt.toSeconds(),
                        getAngularSpeed()
                         ? getOrientation()*Quaternion(getAxisOfRotation(),
                             getAngularSpeed()*dt.toSeconds())
                         : getOrientation(),
                        getVelocity(),
                        getAxisOfRotation(),
                        getAngularSpeed());
    }
};
inline std::ostream &operator<< (std::ostream &os, const Location &loc) {
    os << "[" << loc.getTransform() << "; vel=" <<
        loc.getVelocity() << "; angVel = " << loc.getAngularSpeed() <<
        " around " << loc.getAxisOfRotation() << "]";
    return os;
}

}
#endif
