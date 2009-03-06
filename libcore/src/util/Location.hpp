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
    bool operator ==(const Location&other)const {
        bool eq=x==other.x&&y==other.y&&z==other.z;
        bool veq=other.mVelocity==mVelocity;
        bool qeq=mOrientation==other.mOrientation;
        bool aeq=mAxisOfRotation==other.mAxisOfRotation;
        bool seq=mAngularSpeed==other.mAngularSpeed;
        return eq&&veq&&qeq&&aeq&&seq;
    }
    class Error {
        float64 mDistanceError;
        float32 mAngularError;
    public:
        Error(float64 distanceError, float32 angularError){
            mDistanceError=distanceError;
            mAngularError=angularError;
        }
        Error(const Location&correct, const Location&incorrect) {
            mDistanceError=(incorrect.getPosition()-correct.getPosition()).length();
            mAngularError=2.0*std::acos(correct.getOrientation().normal()
                                        .dot(incorrect.getOrientation().normal()));
        }
        bool operator<= (const Error&other)const {
            return mDistanceError<=other.mDistanceError&&mAngularError<=other.mAngularError;
        }
    };
    class ErrorPredicate {
        Error mBound;
    public:
        ErrorPredicate(const Error&bound):mBound(bound){}
        ///Returns true if the error is too high... i.e. the error is not less than the bound
        bool operator() (const Location &correct, const Location &incorrect) const{
            return !(Error(correct,incorrect)<=mBound);
        }
    }; 
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
    template<class TimeDuration> Location extrapolate(const TimeDuration&dt)const {
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
