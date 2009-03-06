/*  Sirikata Utilities -- Math Library
 *  Transform.hpp
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
#ifndef _TRANSFORM_HPP_
#define _TRANSFORM_HPP_

#include "Quaternion.hpp"

namespace Sirikata {

class Transform: protected Vector3<float64> {
    Quaternion mOrientation;
public:
    Transform(){}
    Transform(const Vector3<float64>&position, 
             const Quaternion&orientation):mOrientation(orientation) {
        x=position.x;
        y=position.y;
        z=position.z;
    }
    bool operator ==(const Transform&other)const {
        bool eq=x==other.x&&y==other.y&&z==other.z;
        bool qeq=mOrientation==other.mOrientation;
        return eq && qeq;
    }
    class Error {
        float64 mDistanceError;
        float32 mAngularError;
    public:
        Error(float64 distanceError, float32 angularError){
            mDistanceError=distanceError;
            mAngularError=angularError;
        }
        Error(const Transform&correct, const Transform&incorrect) {
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
        bool operator() (const Transform &correct, const Transform &incorrect) const{
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
    const Quaternion&getOrientation() const{
        return mOrientation;
    }
    void setOrientation(const Quaternion&orientation) {
        mOrientation=orientation;
    }
    Transform blend(const Transform&newTransform,float32 percentNew) const{
        float32 percentOld=(1.0f-percentNew);
        return Transform (newTransform.getPosition()*percentNew+getPosition()*percentOld,
                         (newTransform.getOrientation()*percentNew+getOrientation()*percentOld).normal());
    }
};



}
#endif
