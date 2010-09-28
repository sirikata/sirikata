/*  Sirikata Utilities -- Math Library
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
#ifndef _QUATERNION_HPP_
#define _QUATERNION_HPP_

#include "Vector3.hpp"
#include "Vector4.hpp"

namespace Sirikata {
namespace QuaternionImpl {

}
class Quaternion:public Vector4<float> {
public:
    typedef float scalar;
private:
    Quaternion(const Vector4<scalar>&value):Vector4<scalar>(value){

    }
    friend Quaternion operator *(scalar, const Quaternion&);
    friend Quaternion operator /(scalar, const Quaternion&);
    static void ToRotationMatrix(const Quaternion &q, Matrix3x3<Quaternion::scalar> kRot) {
        Quaternion::scalar fTx  = 2.0f*q.x;
        Quaternion::scalar fTy  = 2.0f*q.y;
        Quaternion::scalar fTz  = 2.0f*q.z;
        Quaternion::scalar fTwx = fTx*q.w;
        Quaternion::scalar fTwy = fTy*q.w;
        Quaternion::scalar fTwz = fTz*q.w;
        Quaternion::scalar fTxx = fTx*q.x;
        Quaternion::scalar fTxy = fTy*q.x;
        Quaternion::scalar fTxz = fTz*q.x;
        Quaternion::scalar fTyy = fTy*q.y;
        Quaternion::scalar fTyz = fTz*q.y;
        Quaternion::scalar fTzz = fTz*q.z;

        kRot(0,0) = 1.0f-(fTyy+fTzz);
        kRot(0,1) = fTxy-fTwz;
        kRot(0,2) = fTxz+fTwy;
        kRot(1,0) = fTxy+fTwz;
        kRot(1,1) = 1.0f-(fTxx+fTzz);
        kRot(1,2) = fTyz-fTwx;
        kRot(2,0) = fTxz-fTwy;
        kRot(2,1) = fTyz+fTwx;
        kRot(2,2) = 1.0f-(fTxx+fTyy);
    }
    static void FromRotationMatrix (Quaternion &q, const Matrix3x3<Quaternion::scalar>& kRot) {
        /* Shoemake SIGGRAPH 1987 algorithm */
        Quaternion::scalar fTrace = kRot(0,0)+kRot(1,1)+kRot(2,2);
        Quaternion::scalar fRoot;
        if ( fTrace > 0.0 )
        {
            // |w| > 1/2, may as well choose w > 1/2
            fRoot = sqrt(fTrace + 1.0f);  // 2w
            q.w = 0.5f*fRoot;
            fRoot = 0.5f/fRoot;  // 1/(4w)
            q.x = (kRot(2,1)-kRot(1,2))*fRoot;
            q.y = (kRot(0,2)-kRot(2,0))*fRoot;
            q.z = (kRot(1,0)-kRot(0,1))*fRoot;
        }
        else
        {
            // |w| <= 1/2
            static unsigned int s_iNext[3] = { 1, 2, 0 };
            unsigned int i = 0;
            if ( kRot(1,1) > kRot(0,0) )
                i = 1;
            if ( kRot(2,2) > kRot(i,i) )
                i = 2;
            unsigned int j = s_iNext[i];
            unsigned int k = s_iNext[j];
            fRoot = sqrt(kRot(i,i)-kRot(j,j)-kRot(k,k) + 1.0f);
            Quaternion::scalar* apkQuat[3] = { &q.x, &q.y, &q.z };
            *apkQuat[i] = 0.5f*fRoot;
            fRoot = 0.5f/fRoot;
            q.w = (kRot(k,j)-kRot(j,k))*fRoot;
            *apkQuat[j] = (kRot(j,i)+kRot(i,j))*fRoot;
            *apkQuat[k] = (kRot(k,i)+kRot(i,k))*fRoot;
        }
    }
public:
    Quaternion(){}
    class XYZW{};
    class WXYZ{};
    Quaternion(scalar x, scalar y, scalar z, scalar w):Vector4<scalar>(x,y,z,w) {

    }
    Quaternion(scalar x,scalar y, scalar z, scalar w, XYZW convention):Vector4<scalar>(x,y,z,w) {

    }
    Quaternion(scalar w, scalar x,scalar y, scalar z, WXYZ convention):Vector4<scalar>(x,y,z,w) {

    }
    Quaternion(const Vector3<scalar>&axis, scalar angle){
        Quaternion::scalar sinHalfAngle=(Quaternion::scalar)sin(angle*.5);
        x=sinHalfAngle*axis.x;
        y=sinHalfAngle*axis.y;
        z=sinHalfAngle*axis.z;
        w=(Quaternion::scalar)cos(angle*.5);
    }
    Quaternion(const Vector3<scalar> &xAxis,
               const Vector3<scalar> &yAxis,
               const Vector3<scalar> &zAxis){
        FromRotationMatrix(*this,
                           Matrix3x3<Quaternion::scalar>(xAxis,
                                                         yAxis,
                                                         zAxis,
                                                         COLUMNS()));

    }
    Quaternion (const Matrix3x3<Quaternion::scalar>& kRot) {
        FromRotationMatrix(*this, kRot);
    }

    static Quaternion identity() {
        Quaternion retval;
        retval.w=1.0;
        retval.x=retval.y=retval.z=0.0;
        return retval;
    }
    static Quaternion zero() {
        return Quaternion(0,0,0,0,WXYZ());
    }
    void toAngleAxis (scalar& returnAngleRadians,
                      Vector3<scalar>& returnAxis) const{
        scalar fSqrLength = x*x+y*y+z*z;
        scalar tmpw=w;
        scalar eps=(scalar)1e-06;
        if (tmpw>1.0f&&tmpw<1.0f+eps)
            tmpw=1.0f;
        if (tmpw<-1.0&&tmpw>-1.0f-eps)
            tmpw=-1.0f;
        if ( fSqrLength > 1e-08 && tmpw<=1 && tmpw>=-1)
        {
            returnAngleRadians = 2.0f*acos(tmpw);
            scalar length = sqrt(fSqrLength);
            returnAxis.x = x/length;
            returnAxis.y = y/length;
            returnAxis.z = z/length;
        }
        else
        {
            returnAngleRadians = 0.0f;
            returnAxis.x = 1.0f;
            returnAxis.y = 0.0f;
            returnAxis.z = 0.0f;
        }
    }
    Quaternion normal()const {
        return Quaternion(Vector4<scalar>::normal());
    }
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
            w*other.z + z*other.w + x*other.y - y*other.x, WXYZ());
    }

    template<typename myscalar>
    Vector3<myscalar> operator *(const Vector3<myscalar>&other)const {
        Vector3<myscalar> quat_axis(x,y,z);
        Vector3<myscalar>uv = quat_axis.cross(other);
        Vector3<myscalar>uuv= quat_axis.cross(uv);
        uv *=(2.0f*w);
        uuv*=2.0f;
        return other + uv + uuv;
    }

    Quaternion inverse() const {
        scalar len=lengthSquared();
        if (len>1e-8) {
            return Quaternion(w/len,-x/len,-y/len,-z/len,WXYZ());
        }
        return zero();
    }
    Vector3<scalar> xAxis() const{
        //scalar fTx  = 2.0*x;
        scalar fTy  = 2.0f*y;
        scalar fTz  = 2.0f*z;
        scalar fTwy = fTy*w;
        scalar fTwz = fTz*w;
        scalar fTxy = fTy*x;
        scalar fTxz = fTz*x;
        scalar fTyy = fTy*y;
        scalar fTzz = fTz*z;

        return Vector3<scalar>(1.0f-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy);
    }

    Vector3<scalar> yAxis() const{
        scalar fTx  = 2.0f*x;
        scalar fTy  = 2.0f*y;
        scalar fTz  = 2.0f*z;
        scalar fTwx = fTx*w;
        scalar fTwz = fTz*w;
        scalar fTxx = fTx*x;
        scalar fTxy = fTy*x;
        scalar fTyz = fTz*y;
        scalar fTzz = fTz*z;

        return Vector3<scalar>(fTxy-fTwz, 1.0f-(fTxx+fTzz), fTyz+fTwx);

    }
    Vector3<scalar> zAxis() const{
        scalar fTx  = 2.0f*x;
        scalar fTy  = 2.0f*y;
        scalar fTz  = 2.0f*z;
        scalar fTwx = fTx*w;
        scalar fTwy = fTy*w;
        scalar fTxx = fTx*x;
        scalar fTxz = fTz*x;
        scalar fTyy = fTy*y;
        scalar fTyz = fTz*y;
        return Vector3<scalar>(fTxz+fTwy, fTyz-fTwx, 1.0f-(fTxx+fTyy));
    }
    void toAxes(Vector3<scalar> &xAxis,
                Vector3<scalar> &yAxis,
                Vector3<scalar> &zAxis)const{
        Matrix3x3<Quaternion::scalar> rot;
        ToRotationMatrix(*this,rot);
        xAxis=rot.getCol(0);
        yAxis=rot.getCol(1);
        zAxis=rot.getCol(2);
    }

    /** Raise the quaternion to a power (i.e. apply the rotation n times).
     *  \param n the exponent
     */
    Quaternion exp(float32 n) const {
        // FIXME there's probably a much more efficient way to do this
        float angle;
        Vector3<float32> axis;
        toAngleAxis(angle, axis);
        return Quaternion(axis, angle*n);
    }
};
inline Quaternion operator *(Quaternion::scalar s,const Quaternion&q) {
    return Quaternion(s*q.x,s*q.y,s*q.z,s*q.w,Quaternion::XYZW());
}
inline Quaternion operator /(Quaternion::scalar s,const Quaternion&q) {
    return Quaternion(s/q.x,s/q.y,s/q.z,s/q.w,Quaternion::XYZW());
}
}
#endif
