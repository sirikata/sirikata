/*     Iridium Utilities -- Math Library
 *  Vector4.hpp
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
namespace Iridium {
template <typename scalar> class Vector3;

template <typename scalar> class Vector4 {
public:
    typedef scalar real;
    union {
        struct {
            scalar x;
            scalar y;
            scalar z;
            scalar w;
        };
        scalar v[4];
    };
    Vector4(scalar x, scalar y, scalar z, scalar w) {
        this->x=x;
        this->y=y;
        this->z=z;
        this->w=w;
    }
    Vector4(){}
    template<class T>T convert(const T*ptr=NULL) const{
        return T(x,y,z);
    }
    template <class V> static Vector4 fromArray(const V&other){
        return Vector4(other[0],other[1],other[2],other[3]);
    }

    template <class V> static Vector4 fromSSE(const V&other){
        return Vector4(other.x(),other.y(),other.z(),other.w());
    }
    scalar&operator[](const unsigned int i) {
        assert(i<4);
        return v[i];
    }
    scalar operator[](const unsigned int i) const{
        assert(i<4);
        return v[i];
    }
    Vector4&operator=(scalar other) {
        x=other;
        y=other;
        z=other;
        w=other;
        return *this;
    }
    static Vector4 nil() {
        return Vector4(0,0,0,0);
    }
    static Vector4 unitX() {
        return Vector4(1,0,0,0);
    }
    static Vector4 unitY() {
        return Vector4(0,1,0,0);
    }
    static Vector4 unitZ() {
        return Vector4(0,0,1,0);
    }
    static Vector4 unitNegX() {
        return Vector4(-1,0,0,0);
    }
    static Vector4 unitNegY() {
        return Vector4(0,-1,0,0);
    }
    static Vector4 unitNegZ() {
        return Vector4(0,0,-1,0);
    }
    Vector4 operator*(scalar scale) const {
        return Vector4(x*scale,y*scale,z*scale,w*scale);
    }
    Vector4 operator/(scalar scale) const {
        return Vector4(x/scale,y/scale,z/scale,w/scale);
    }
    Vector4 operator+(const Vector4&other) const {
        return Vector4(x+other.x,y+other.y,z+other.z,w+other.w);
    }
    Vector4 operator-(const Vector4&other) const {
        return Vector4(x-other.x,y-other.y,z-other.z,w-other.w);
    }
    Vector4 operator-() const {
        return Vector4(-x,-y,-z,-w);
    }
    Vector4& operator*=(scalar scale) {
        x*=scale;
        y*=scale;
        z*=scale;
        w*=scale;
        return *this;
    }
    Vector4& operator/=(scalar scale) {
        x/=scale;
        y/=scale;
        z/=scale;
        w/=scale;
        return *this;
    }
    Vector4& operator+=(const Vector4& other) {
        x+=other.x;
        y+=other.y;
        z+=other.z;
        w+=other.w;
        return *this;
    }
    Vector4& operator-=(const Vector4& other) {
        x-=other.x;
        y-=other.y;
        z-=other.z;
        w-=other.w;
        return *this;
    }
    scalar dot(const Vector4 &other) const{
        return x*other.x+y*other.y+z*other.z+w*other.w;
    }
    Vector4 min(const Vector4&other)const {
        return Vector4(other.x<x?other.x:x,
                       other.y<y?other.y:y,
                       other.z<z?other.z:z,
                       other.w<w?other.w:w);
    }
    Vector4 max(const Vector4&other)const {
        return Vector4(other.x>x?other.x:x,
                       other.y>y?other.y:y,
                       other.z>z?other.z:z,
                       other.w>w?other.w:w);
    }
    scalar lengthSquared()const {
        return dot(*this);
    }
    scalar length()const {
        return (scalar)sqrt(dot(*this));
    }
    bool operator==(const Vector4&other)const {
        return x==other.x&&y==other.y&&z==other.z&&w==other.w;
    }
    bool operator!=(const Vector4&other)const {
        return x!=other.x||y!=other.y||z!=other.z||w!=other.w;
    }
    Vector4 normal()const {
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
        os<<'<'<<x<<", "<<y<<", "<<z<<", "<<w<<'>';
        return os.str();
    }
};

template<typename scalar> inline Vector4<scalar> operator *(scalar lhs, const Vector4<scalar> &rhs) {
    return Vector4<scalar>(lhs*rhs.x,lhs*rhs.y,lhs*rhs.z,lhs*rhs.w);
}
template<typename scalar> inline Vector4<scalar> operator /(scalar lhs, const Vector4<scalar> &rhs) {
    return Vector4<scalar>(lhs/rhs.x,lhs/rhs.y,lhs/rhs.z,lhs/rhs.w);
}
template<typename scalar> inline std::ostream& operator <<(std::ostream& os, const Vector4<scalar> &rhs) {
    os<<'<'<<rhs.x<<", "<<rhs.y<<", "<<rhs.z<<", "<<rhs.w<<'>';
    return os;
}

}
