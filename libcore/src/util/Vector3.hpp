/*  Sirikata Utilities -- Math Library
 *  Vector3.hpp
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
#ifndef _SIRIKATA_VECTOR3_HPP_
#define _SIRIKATA_VECTOR3_HPP_
#include <assert.h>
namespace Sirikata {
template <typename scalar> class Vector3 {
public:
    typedef scalar real;
    union {
        struct {
            scalar x;
            scalar y;
            scalar z;
        };
        scalar v[3];
    };
    Vector3(scalar x, scalar y, scalar z) {
        this->x=x;
        this->y=y;
        this->z=z;
    }
    template <class V> explicit Vector3(const V&other) {
        x=other[0];
        y=other[1];
        z=other[2];
    }
    Vector3(){}
    template <class V> static Vector3 fromSSE(const V&other){
        return Vector3(other.x(),other.y(),other.z());
    }
    scalar&operator[](const unsigned int i) {
        assert(i<3);
        return v[i];
    }
#if 0
    template<class T> operator T() const{
        return T(x,y,z);
    }
#else
    template<class T> T convert(const T*ptr=NULL) const{
        return T(x,y,z);
    }
#endif
    scalar operator[](const unsigned int i) const{
        assert(i<3);
        return v[i];
    }
    Vector3&operator=(scalar other) {
        x=other;
        y=other;
        z=other;
        return *this;
    }
    static Vector3 nil() {
        return Vector3(0,0,0);
    }
    static Vector3 unitX() {
        return Vector3(1,0,0);
    }
    static Vector3 unitY() {
        return Vector3(0,1,0);
    }
    static Vector3 unitZ() {
        return Vector3(0,0,1);
    }
    static Vector3 unitNegX() {
        return Vector3(-1,0,0);
    }
    static Vector3 unitNegY() {
        return Vector3(0,-1,0);
    }
    static Vector3 unitNegZ() {
        return Vector3(0,0,-1);
    }
    Vector3 operator*(scalar scale) const {
        return Vector3(x*scale,y*scale,z*scale);
    }
    Vector3 operator/(scalar scale) const {
        return Vector3(x/scale,y/scale,z/scale);
    }
    Vector3 operator+(const Vector3&other) const {
        return Vector3(x+other.x,y+other.y,z+other.z);
    }
    Vector3 componentMultiply(const Vector3&other) const {
        return Vector3(x*other.x,y*other.y,z*other.z);
    }
    Vector3 operator-(const Vector3&other) const {
        return Vector3(x-other.x,y-other.y,z-other.z);
    }
    Vector3 operator-() const {
        return Vector3(-x,-y,-z);
    }
    Vector3& operator*=(scalar scale) {
        x*=scale;
        y*=scale;
        z*=scale;
        return *this;
    }
    Vector3& operator/=(scalar scale) {
        x/=scale;
        y/=scale;
        z/=scale;
        return *this;
    }
    Vector3& operator+=(const Vector3& other) {
        x+=other.x;
        y+=other.y;
        z+=other.z;
        return *this;
    }
    Vector3& operator-=(const Vector3& other) {
        x-=other.x;
        y-=other.y;
        z-=other.z;
        return *this;
    }
    Vector3 cross(const Vector3&other) const {
        return Vector3(y*other.z-z*other.y,
                       z*other.x-x*other.z,
                       x*other.y-y*other.x);
    }
    scalar dot(const Vector3 &other) const{
        return x*other.x+y*other.y+z*other.z;
    }
    Vector3 min(const Vector3&other)const {
        return Vector3(other.x<x?other.x:x,
                       other.y<y?other.y:y,
                       other.z<z?other.z:z);
    }
    Vector3 max(const Vector3&other)const {
        return Vector3(other.x>x?other.x:x,
                       other.y>y?other.y:y,
                       other.z>z?other.z:z);
    }
    scalar lengthSquared()const {
        return dot(*this);
    }
    scalar length()const {
        return (scalar)sqrt(dot(*this));
    }
    bool operator==(const Vector3&other)const {
        return x==other.x&&y==other.y&&z==other.z;
    }
    bool operator!=(const Vector3&other)const {
        return x!=other.x||y!=other.y||z!=other.z;
    }
    Vector3 normal()const {
        scalar len=length();
        if (len>1e-08)
            return *this/len;
        return *this;
    }
    scalar normalizeThis() {
        scalar len=length();
        if (len>1e-08)
            this/=len;
        return len;
    }
    std::string toString()const {
        std::ostringstream os;
        os<<'<'<<x<<", "<<y<<", "<<z<<'>';
        return os.str();
    }
    Vector3 reflect(const Vector3& normal)const {
        return Vector3(*this-normal*((scalar)2.0*this->dot(normal)));
    }
};

template<typename scalar> inline Vector3<scalar> operator *(scalar lhs, const Vector3<scalar> &rhs) {
    return Vector3<scalar>(lhs*rhs.x,lhs*rhs.y,lhs*rhs.z);
}
template<typename scalar> inline Vector3<scalar> operator /(scalar lhs, const Vector3<scalar> &rhs) {
    return Vector3<scalar>(lhs/rhs.x,lhs/rhs.y,lhs/rhs.z);
}
template<typename scalar> inline std::ostream& operator <<(std::ostream& os, const Vector3<scalar> &rhs) {
    os<<'<'<<rhs.x<<", "<<rhs.y<<", "<<rhs.z<<'>';
    return os;
}

}
#endif
