/*  Sirikata Utilities -- Math Library
 *  Vector2.hpp
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
#ifndef _SIRIKATA_VECTOR2_HPP_
#define _SIRIKATA_VECTOR2_HPP_

#include <string>
#include <cassert>
#include <sstream>

namespace Sirikata {
template <typename scalar> class Vector2 {
public:
    static const unsigned char size = 2;
    typedef scalar real;
    union {
        struct {
            scalar x;
            scalar y;
        };
        scalar v[2];
    };
    Vector2(scalar x, scalar y) {
        this->x=x;
        this->y=y;
    }
    template <class V> explicit Vector2(const V&other) {
        x=other[0];
        y=other[1];
    }
    Vector2(){}
    template <class V> static Vector2 fromSSE(const V&other){
        return Vector2(other.x(),other.y());
    }
    scalar&operator[](const unsigned int i) {
        assert(i<2);
        return v[i];
    }
#if 0
    template<class T> operator T() const{
        return T(x,y);
    }
#else
    template<class T> T convert() const{
        return T(x,y);
    }
#endif
    scalar operator[](const unsigned int i) const{
        assert(i<2);
        return v[i];
    }
    Vector2&operator=(scalar other) {
        x=other;
        y=other;
        return *this;
    }
    static Vector2 nil() {
        return Vector2(0,0);
    }
    static Vector2 unitX() {
        return Vector2(1,0);
    }
    static Vector2 unitY() {
        return Vector2(0,1);
    }
    static Vector2 unitNegX() {
        return Vector2(-1,0);
    }
    static Vector2 unitNegY() {
        return Vector2(0,-1);
    }
    Vector2 operator*(scalar scale) const {
        return Vector2(x*scale,y*scale);
    }
    Vector2 operator/(scalar scale) const {
        return Vector2(x/scale,y/scale);
    }
    Vector2 operator+(const Vector2&other) const {
        return Vector2(x+other.x,y+other.y);
    }
    Vector2 componentMultiply(const Vector2&other) const {
        return Vector2(x*other.x,y*other.y);
    }
    Vector2 operator-(const Vector2&other) const {
        return Vector2(x-other.x,y-other.y);
    }
    Vector2 operator-() const {
        return Vector2(-x,-y);
    }
    Vector2& operator*=(scalar scale) {
        x*=scale;
        y*=scale;
        return *this;
    }
    Vector2& operator/=(scalar scale) {
        x/=scale;
        y/=scale;
        return *this;
    }
    Vector2& operator+=(const Vector2& other) {
        x+=other.x;
        y+=other.y;
        return *this;
    }
    Vector2& operator-=(const Vector2& other) {
        x-=other.x;
        y-=other.y;
        return *this;
    }
    scalar dot(const Vector2 &other) const{
        return x*other.x+y*other.y;
    }
    Vector2 min(const Vector2&other)const {
        return Vector2(other.x<x?other.x:x,
                       other.y<y?other.y:y);
    }
    Vector2 max(const Vector2&other)const {
        return Vector2(other.x>x?other.x:x,
                       other.y>y?other.y:y);
    }
    scalar lengthSquared()const {
        return dot(*this);
    }
    scalar length()const {
        return (scalar)sqrt(dot(*this));
    }
    bool operator==(const Vector2&other)const {
        return x==other.x&&y==other.y;
    }
    bool operator!=(const Vector2&other)const {
        return x!=other.x||y!=other.y;
    }
    Vector2 normal()const {
        scalar len=length();
        if (len>1e-08)
            return *this/len;
        return *this;
    }
    scalar normalizeThis() {
        scalar len=length();
        if (len>1e-08)
            *this/=len;
        return len;
    }
    std::string toString()const {
        std::ostringstream os;
        os<<'<'<<x<<","<<y<<'>';
        return os.str();
    }
    Vector2 reflect(const Vector2& normal)const {
        return Vector2(*this-normal*((scalar)2.0*this->dot(normal)));
    }
};

template<typename scalar> inline Vector2<scalar> operator *(scalar lhs, const Vector2<scalar> &rhs) {
    return Vector2<scalar>(lhs*rhs.x,lhs*rhs.y,lhs*rhs.z);
}
template<typename scalar> inline Vector2<scalar> operator /(scalar lhs, const Vector2<scalar> &rhs) {
    return Vector2<scalar>(lhs/rhs.x,lhs/rhs.y);
}
template<typename scalar> inline std::ostream& operator <<(std::ostream& os, const Vector2<scalar> &rhs) {
    os<<'<'<<rhs.x<<","<<rhs.y<<","<<'>';
    return os;
}
template<typename scalar> inline std::istream& operator >>(std::istream& is, Vector2<scalar> &rhs) {
    // FIXME this should be more robust.  It currently relies on the exact format provided by operator <<
    char dummy;
    is >> dummy >> rhs.x >> dummy >> rhs.y >> dummy;
    return is;
}

}
#endif
