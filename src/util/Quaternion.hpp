/*     Iridium Utilities -- Math Library
 *  Quaternion.hpp
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
 *  * Neither the name of Iridium nor the names of its contributors may
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
#ifndef _QUATERNION_HPP_
#define _QUATERNION_HPP_
#include "Vector3.hpp"
#include "Vector4.hpp"
namespace Iridium {
class Quaternion:public Vector4<float> {
    typedef float scalar;
    Quaternion(const Vector4<scalar>&value):Vector4<scalar>(value){
        
    }
    friend Quaternion operator *(scalar, const Quaternion&);
    friend Quaternion operator /(scalar, const Quaternion&);
public:
    Quaternion(){}
    Quaternion(scalar x,scalar y, scalar z, scalar w):Vector4<scalar>(x,y,z,w) {
        
    }
    Quaternion(const Vector3<scalar>&axis, scalar angle);
    void ToAngleAxis (scalar& returnAngleRadians,
                      Vector3<scalar>& returnAxis) const;

    Quaternion operator+(const Quaternion&other) const{
        return Quaternion(Vector4<scalar>::operator+(other));
    }

    Quaternion operator-(const Quaternion&other) const{
        return Quaternion(Vector4<scalar>::operator+(other));
    }

    Quaternion operator-()const {
        return Quaternion(Vector4<scalar>::operator -());
    }

    Quaternion operator*(scalar other) const{
        return Quaternion(Vector4<scalar>::operator*(other));
    }

    Quaternion operator/(scalar other) const{
        return Quaternion(Vector4<scalar>::operator/(other));
    }

    Quaternion operator*(const Quaternion&other)const {
        return Quaternion(
            w*other.w - x*other.x - y*other.y - z*other.z,
            w*other.x + x*other.w + y*other.z - z*other.y,
            w*other.y + y*other.w + z*other.x - x*other.z,
            w*other.z + z*other.w + x*other.y - y*other.x);
    }

    template<typename real>
    Vector3<real> operator *(const Vector3<real>&other)const {
        Vector3<real> quat_axis(x,y,z);
        Vector3<real>uv = quat_axis.cross(other);
        Vector3<real>uuv= quat_axis.cross(uv);
        uv *=(2.0*w);
        uuv*=2.0;
        return other + uv + uuv;
    }

    Quaternion Quaternion::inverse() const {
        scalar len=lengthSquared();
        if (len>1e-8) {
            return Quaternion(w/len,-x/len,-y/len,-z/len);
        }
        return Quaternion(0.,0.,0.,0.);
    }
    Vector3<scalar> xAxis() const;
    Vector3<scalar> yAxis() const;
    Vector3<scalar> zAxis() const;
};
inline Quaternion operator *(Quaternion::scalar s,const Quaternion&q) {
    return Quaternion(s*q.x,s*q.y,s*q.z,s*q.w);
}
inline Quaternion operator /(Quaternion::scalar s,const Quaternion&q) {
    return Quaternion(s/q.x,s/q.y,s/q.z,s/q.w);
}
}
#endif
