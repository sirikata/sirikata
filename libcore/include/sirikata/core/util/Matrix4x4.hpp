/*  Sirikata Utilities -- Math Library
 *  Matrix4x4.hpp
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
#ifndef _SIRIKATA_MATRIX4X4_HPP_
#define _SIRIKATA_MATRIX4X4_HPP_

#include "Vector4.hpp"

namespace Sirikata {

template <typename scalar> class Matrix4x4 {
public:
    typedef Vector4<scalar> Vector4x;

    class COLUMNS{};
    class ROWS{};

    class COLUMN_MAJOR{};
    class ROW_MAJOR{};

    enum Dimension {
        X = 0,
        Y = 1,
        Z = 2,
        W = 3
    };
private:
    Vector4x mCol[4];
public:
    typedef scalar real;
    Matrix4x4(){}
    static const Matrix4x4& nil() {
        static Matrix4x4 nil(Vector4x::nil(),
                             Vector4x::nil(),
                             Vector4x::nil(),
                             Vector4x::nil(),
                             COLUMNS());
        return nil;
    }
    static const Matrix4x4& identity() {
        static Matrix4x4 identity (Vector4x::unitX(),
                                   Vector4x::unitY(),
                                   Vector4x::unitZ(),
            Vector4x(0, 0, 0, 1),
                                   COLUMNS());
        return identity;
    }
    // Generate a matrix for swapping two dimensions.
    static Matrix4x4 swapDimensions(Dimension old_up, Dimension new_up) {
        assert(old_up != W && new_up != W);
        if (old_up == new_up) return identity();
        Vector4x dims[3] = { Vector4x::unitX(), Vector4x::unitY(), Vector4x::unitZ() };
        std::swap(dims[old_up], dims[new_up]);
        return Matrix4x4(
            dims[0], dims[1], dims[2],
            Vector4x(0, 0, 0, 1),
            COLUMNS()
        );
    }
    static Matrix4x4 reflection(Dimension across) {
        return Matrix4x4(
            (across == X ? -1.f : 1.f) * Vector4x::unitX(),
            (across == Y ? -1.f : 1.f) * Vector4x::unitY(),
            (across == Z ? -1.f : 1.f) * Vector4x::unitZ(),
            Vector4x(0, 0, 0, 1),
            COLUMNS());
    }
    static Matrix4x4 scale(Dimension dim, scalar s) {
        return Matrix4x4(
            (dim == X ? s : 1.f) * Vector4x::unitX(),
            (dim == Y ? s : 1.f) * Vector4x::unitY(),
            (dim == Z ? s : 1.f) * Vector4x::unitZ(),
            Vector4x(0, 0, 0, 1),
            COLUMNS()
        );
    }
    static Matrix4x4 scale(scalar s) {
        return Matrix4x4(
            Vector4x::unitX() * s,
            Vector4x::unitY() * s,
            Vector4x::unitZ() * s,
            Vector4x(0, 0, 0, 1),
            COLUMNS()
        );
    }
    Matrix4x4(const Vector4x&col1, const Vector4x&col2, const Vector4x&col3, const Vector4x&col4, COLUMNS c){
        setCol(0,col1);
        setCol(1,col2);
        setCol(2,col3);
        setCol(3,col4);
    }
    Matrix4x4(const Vector4x&row1, const Vector4x&row2, const Vector4x&row3, const Vector4x&row4, ROWS r){
        setRow(0,row1);
        setRow(1,row2);
        setRow(2,row3);
        setRow(3,row4);
    }

    template<typename T>
    Matrix4x4(const T&other, COLUMN_MAJOR c) {
        for(int i = 0; i < 4; i++)
            setCol(i, Vector4x(other[i][0], other[i][1], other[i][2], other[i][3]));
    }
    template<typename T>
    Matrix4x4(const T&other, ROW_MAJOR r) {
        for(int i = 0; i < 4; i++)
            setRow(i, Vector4x(other[i][0], other[i][1], other[i][2], other[i][3]));
    }

    const Vector4x& getCol(unsigned int which) const {
        assert(which<4);
        return mCol[which];
    }
    void setCol(unsigned int which,const Vector4x &col) {
        assert(which<4);
        mCol[which]=col;
    }
    Vector4x getRow(unsigned int which) const {
        assert(which<4);
        return Vector4x(mCol[0][which],
            mCol[1][which],
            mCol[2][which],
            mCol[3][which]);
    }
    void setRow(unsigned int which, const Vector4x &row) {
        assert(which<4);
        mCol[0][which]=row[0];
        mCol[1][which]=row[1];
        mCol[2][which]=row[2];
        mCol[3][which]=row[3];
    }
    scalar&operator() (unsigned int row, unsigned int column) {
        return mCol[column][row];
    }
    scalar operator() (unsigned int row, unsigned int column) const{
        return mCol[column][row];
    }
    template <typename T> Vector4<T> operator *(const Vector4<T>&other)const {
        return mCol[0]*other.x+mCol[1]*other.y+mCol[2]*other.z+mCol[3]*other.w;
    }
    Matrix4x4 operator *(scalar other)const {
        return Matrix4x4(getCol(0)*other,getCol(1)*other,getCol(2)*other,getCol(3)*other,COLUMNS());
    }
    Matrix4x4 operator /(scalar other)const {
        return Matrix4x4(getCol(0)/other,getCol(1)/other,getCol(2)/other,getCol(3)/other,COLUMNS());
    }
    bool operator == (const Matrix4x4&other)const{
        return getCol(0)==other.getCol(0)&&getCol(1)==other.getCol(1)&&getCol(2)==other.getCol(2)&&getCol(3)==other.getCol(3);
    }
    bool operator != (const Matrix4x4&other)const{
        return getCol(0)!=other.getCol(0)&&getCol(1)!=other.getCol(1)&&getCol(2)!=other.getCol(2)&&getCol(3)!=other.getCol(3);
    }
    Matrix4x4 operator+ (const Matrix4x4&other)const {
        return Matrix4x4(getCol(0)+other.getCol(0),
                       getCol(1)+other.getCol(1),
                       getCol(2)+other.getCol(2),
                       getCol(3)+other.getCol(3),
                       COLUMNS());
    }
    Matrix4x4 operator- (const Matrix4x4&other)const {
        return Matrix4x4(getCol(0)-other.getCol(0),
                       getCol(1)-other.getCol(1),
                       getCol(2)-other.getCol(2),
                       getCol(3)-other.getCol(3),
                       COLUMNS());
    }
    Matrix4x4 operator- ()const {
        return Matrix4x4(-getCol(0),
                       -getCol(1),
                       -getCol(2),
                       -getCol(3),
                       COLUMNS());
    }
    Matrix4x4& operator+= (const Matrix4x4&other) {
        mCol[0]+=other.getCol(0);
        mCol[1]+=other.getCol(1);
        mCol[2]+=other.getCol(2);
        mCol[3]+=other.getCol(3);
        return *this;
    }
    Matrix4x4& operator-= (const Matrix4x4&other) {
        mCol[0]-=other.getCol(0);
        mCol[1]-=other.getCol(1);
        mCol[2]-=other.getCol(2);
        mCol[3]-=other.getCol(3);
        return *this;
    }
    Matrix4x4 operator* (const Matrix4x4&other)const {
        Vector4<scalar> ocol0=other.getCol(0);
        Vector4<scalar> ocol1=other.getCol(1);
        Vector4<scalar> ocol2=other.getCol(2);
        Vector4<scalar> ocol3=other.getCol(3);
        return Matrix4x4(mCol[0]*ocol0.x+mCol[1]*ocol0.y+mCol[2]*ocol0.z+mCol[3]*ocol0.w,
                         mCol[0]*ocol1.x+mCol[1]*ocol1.y+mCol[2]*ocol1.z+mCol[3]*ocol1.w,
                         mCol[0]*ocol2.x+mCol[1]*ocol2.y+mCol[2]*ocol2.z+mCol[3]*ocol2.w,
                         mCol[0]*ocol3.x+mCol[1]*ocol3.y+mCol[2]*ocol3.z+mCol[3]*ocol3.w,
                         COLUMNS());
    }
    Matrix4x4& operator*= (const Matrix4x4&other) {
        Vector4<scalar> ocol0=other.getCol(0);
        Vector4<scalar> ocol1=other.getCol(1);
        Vector4<scalar> ocol2=other.getCol(2);
        Vector4<scalar> ocol3=other.getCol(3);
        //cannot do this *= inplace
        ocol0=mCol[0]*ocol0.x+mCol[1]*ocol0.y+mCol[2]*ocol0.z+mCol[3]*ocol0.w;
        ocol1=mCol[0]*ocol1.x+mCol[1]*ocol1.y+mCol[2]*ocol1.z+mCol[3]*ocol1.w;
        ocol2=mCol[0]*ocol2.x+mCol[1]*ocol2.y+mCol[2]*ocol2.z+mCol[3]*ocol2.w;
        ocol3=mCol[0]*ocol3.x+mCol[1]*ocol3.y+mCol[2]*ocol3.z+mCol[3]*ocol3.w;
        //have to copy back
        mCol[0]=ocol0;
        mCol[1]=ocol1;
        mCol[2]=ocol2;
        mCol[3]=ocol3;
        return *this;
    }
    Matrix4x4& operator*= (scalar other) {
        mCol[0]*=other;
        mCol[1]*=other;
        mCol[2]*=other;
        mCol[3]*=other;
        return *this;
    }
    Matrix4x4& operator/= (scalar other) {
        mCol[0]/=other;
        mCol[1]/=other;
        mCol[2]/=other;
        mCol[3]/=other;
        return *this;
    }
    Matrix4x4 transpose() const {
        return Matrix4x4(getCol(0),
                      getCol(1),
                      getCol(2),
                      getCol(3),
                      ROWS());
    }
    Matrix3x3<scalar> extract3x3() const {
        Vector4x c1 = getCol(0);
        Vector4x c2 = getCol(1);
        Vector4x c3 = getCol(2);
        return Matrix3x3<scalar>(
            Vector3<scalar>(c1[0], c1[1], c1[2]),
            Vector3<scalar>(c2[0], c2[1], c2[2]),
            Vector3<scalar>(c3[0], c3[1], c3[2]),
            Sirikata::COLUMNS()
        );
    }
    std::string toString() const {
        std::ostringstream os;
        os<<"{ col1:"<<mCol[0]<<" col2:"<<mCol[1]<<" col3:"<<mCol[2]<<" col4:"<<mCol[3]<<'}';
        return os.str();
    }
};
template<typename scalar> inline std::ostream& operator <<(std::ostream& os, const Matrix4x4<scalar> &rhs) {
    os<<"{ col1:"<<rhs.getCol(0)<<" col2:"<<rhs.getCol(1)<<" col3:"<<rhs.getCol(2)<<" col4:"<<rhs.getCol(3)<<'}';
    return os;
}

template <typename T,typename S> Vector4<T> operator *(const Vector4<T>&vec, const Matrix4x4<S>&mat) {
    return Vector4<T>(
        mat.getCol(0).dot(vec),
        mat.getCol(1).dot(vec),
        mat.getCol(2).dot(vec),
        mat.getCol(3).dot(vec)
    );
}
template <typename T> Matrix4x4<T> operator *(T other, const Matrix4x4<T>&mat) {
    return Matrix4x4<T>(mat.getCol(0)*other,mat.getCol(1)*other,mat.getCol(2)*other,mat.getCol(3)*other,COLUMNS());
}
template <typename T> Matrix4x4<T> operator /(T other, const Matrix4x4<T>&mat) {
    return Matrix4x4<T>(other/mat.getCol(0),other/mat.getCol(1),other/mat.getCol(2),other/mat.getCol(3),COLUMNS());
}

typedef Matrix4x4<float32> Matrix4x4f;
typedef Matrix4x4<float64> Matrix4x4d;

}
#endif // _SIRIKATA_MATRIX4x4_HPP_
