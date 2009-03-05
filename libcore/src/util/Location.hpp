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

#include "Quaternion.hpp"

namespace Sirikata {

class Location: protected Vector3<float64> {
    Vector3<float32> mVelocity;
    Quaternion mOrientation;
    Vector3<float32> mAxisOfRotation;
    float32 mAngularSpeed;
public:
    Location(){}
    Location(const Vector3<float64>&position, 
             const Quaternion&orientation, 
             const Vector3<float32> &velocity, 
             const Vector3<float32> angularVelocityAxis, 
             float32 angularVelocityRadians):mVelocity(velocity),mOrientation(orientation),mAxisOfRotation(angularVelocityAxis), mAngularSpeed(angularVelocityRadians) {
        x=position.x;
        y=position.y;
        z=position.z;
    }
    const Vector3<float64>&getPosition()const {
        return *this;
    }
    void setPosition(const Vector3<float64>& position) {
        x=position.x;
        y=position.y;
        z=position.z;
    }
    const Vector3<float32>&getVelocity()const {
        return mVelocity;
    }
    void setVelocity(const Vector3<float32> velocity) {
        mVelocity=velocity;
    } 
    const Quaternion&getOrientation() const{
        return mOrientation;
    }
    void setOrientation(const Quaternion&orientation) {
        mOrientation=orientation;
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
        mAxisOfRotation/=mAngularSpeed;
    }
    template<class TimeDuration> Location predict(const TimeDuration&dt)const {
        return Location(getPosition()+Vector3<float64>(getVelocity())*(float64)dt,
                        getAngularSpeed() 
                         ? getOrientation()*Quaternion(getAxisOfRotation(),
                                                       getAngularSpeed()*(float64)dt)
                         : getOrientation(),
                        getVelocity(),
                        getAxisOfRotation(),
                        getAngularSpeed());
    }
};
}
#endif
