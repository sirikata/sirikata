/*     Iridium Utilities -- Math Library
 *  Matrix3x3.hpp
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
#ifndef _MATRIX3X3_HPP_
#define _MATRIX3X3_HPP_
namespace Iridium {
class COLUMNS{};
class ROWS{};

template <typename scalar> class Matrix3x3 {
public:
    typedef Vector3<scalar> Vector3x;
private:
    Vector3x mCol[3];
public:
    typedef scalar real;
    Matrix3x3(){}
    static const Matrix3x3& nil() {
        static Matrix3x3 nil(Vector3x::nil(),
                             Vector3x::nil(),
                             Vector3x::nil(),
                             COLUMNS());
        return nil;
    }
    static const Matrix3x3& identity() {
        static Matrix3x3 identity (Vector3x::unitX(),
                                   Vector3x::unitY(),
                                   Vector3x::unitZ(),
                                   COLUMNS());
        return identity;
    }
    Matrix3x3(const Vector3x&col1, const Vector3x&col2, const Vector3x&col3, COLUMNS c){
        setCol(0,col1);
        setCol(1,col2);
        setCol(2,col3);
    }
    Matrix3x3(const Vector3x&row1, const Vector3x&row2, const Vector3x&row3, ROWS r){
        setRow(0,row1);
        setRow(1,row2);
        setRow(2,row3);
    }
    const Vector3x& getCol(unsigned int which) const {
        assert(which<3);
        return mCol[which];
    }
    void setCol(unsigned int which,const Vector3x &col) {
        assert(which<3);
        mCol[which]=col;
    }
    Vector3x getRow(unsigned int which) const {
        assert(which<3);
        return Vector3x(mCol[0][which],
            mCol[1][which],
            mCol[2][which]);
    }
    void setRow(unsigned int which, const Vector3x &row) {
        assert(which<3);
        mCol[0][which]=row[0];
        mCol[1][which]=row[1];
        mCol[2][which]=row[2];
    }
    scalar&operator() (unsigned int row, unsigned int column) {
        return mCol[column][row];
    }
    scalar operator() (unsigned int row, unsigned int column) const{
        return mCol[column][row];
    }
    template <typename T> Vector3<T> operator *(const Vector3<T>&other)const {
        return mCol[0]*other.x+mCol[1]*other.y+mCol[2]*other.z;
    }
    Matrix3x3 operator *(scalar other)const {
        return Matrix3x3(getCol(0)*other,getCol(1)*other,getCol(2)*other,COLUMNS());
    }
    Matrix3x3 operator /(scalar other)const {
        return Matrix3x3(getCol(0)/other,getCol(1)/other,getCol(2)/other,COLUMNS());
    }
    bool operator == (const Matrix3x3&other)const{
        return getCol(0)==other.getCol(0)&&getCol(1)==other.getCol(1)&&getCol(2)==other.getCol(2);
    }
    bool operator != (const Matrix3x3&other)const{
        return getCol(0)!=other.getCol(0)&&getCol(1)!=other.getCol(1)&&getCol(2)!=other.getCol(2);
    }
    Matrix3x3 operator+ (const Matrix3x3&other)const {
        return Matrix3x3(getCol(0)+other.getCol(0),
                       getCol(1)+other.getCol(1),
                       getCol(2)+other.getCol(2),
                       COLUMNS());
    }
    Matrix3x3 operator- (const Matrix3x3&other)const {
        return Matrix3x3(getCol(0)-other.getCol(0),
                       getCol(1)-other.getCol(1),
                       getCol(2)-other.getCol(2),
                       COLUMNS());
    }
    Matrix3x3 operator- ()const {
        return Matrix3x3(-getCol(0),
                       -getCol(1),
                       -getCol(2),
                       COLUMNS());
    }
    Matrix3x3& operator+= (const Matrix3x3&other) {
        mCol[0]+=other.getCol(0);
        mCol[1]+=other.getCol(1);
        mCol[2]+=other.getCol(2);
        return *this;
    }
    Matrix3x3& operator-= (const Matrix3x3&other) {
        mCol[0]-=other.getCol(0);
        mCol[1]-=other.getCol(1);
        mCol[2]-=other.getCol(2);
        return *this;
    }
    Matrix3x3 operator* (const Matrix3x3&other)const {
        Vector3<scalar> ocol0=other.getCol(0);
        Vector3<scalar> ocol1=other.getCol(1);
        Vector3<scalar> ocol2=other.getCol(2);
        return Matrix3x3(mCol[0]*ocol0.x+mCol[1]*ocol0.y+mCol[2]*ocol0.z,
                         mCol[0]*ocol1.x+mCol[1]*ocol1.y+mCol[2]*ocol1.z,
                         mCol[0]*ocol2.x+mCol[1]*ocol2.y+mCol[2]*ocol2.z,
                         COLUMNS());
    }
    Matrix3x3& operator*= (const Matrix3x3&other) {
        Vector3<scalar> ocol0=other.getCol(0);
        Vector3<scalar> ocol1=other.getCol(1);
        Vector3<scalar> ocol2=other.getCol(2);
        //cannot do this *= inplace
        ocol0=mCol[0]*ocol0.x+mCol[1]*ocol0.y+mCol[2]*ocol0.z;
        ocol1=mCol[0]*ocol1.x+mCol[1]*ocol1.y+mCol[2]*ocol1.z;
        ocol2=mCol[0]*ocol2.x+mCol[1]*ocol2.y+mCol[2]*ocol2.z;
        //have to copy back
        mCol[0]=ocol0;
        mCol[1]=ocol1;
        mCol[2]=ocol2;
        return *this;
    }
    Matrix3x3& operator*= (scalar other) {
        mCol[0]*=other;
        mCol[1]*=other;
        mCol[2]*=other;
        return *this;
    }
    Matrix3x3& operator/= (scalar other) {
        mCol[0]/=other;
        mCol[1]/=other;
        mCol[2]/=other;
        return *this;
    }
    Matrix3x3 transpose() const {
        return Matrix3x3(getCol(0),
                      getCol(1),
                      getCol(2),
                      ROWS());
    }
    ///@FIXME: should this be a double or a scalar
    double determinant() const {
        return mCol[0].x*(double)mCol[1].y*mCol[2].z-
               mCol[0].x*(double)mCol[2].y*mCol[1].z +
               mCol[1].x*(double)mCol[2].y*mCol[0].z -
               mCol[1].x*(double)mCol[0].y*mCol[2].z +
               mCol[2].x*(double)mCol[0].y*mCol[1].z -
               mCol[2].x*(double)mCol[1].y*mCol[0].z;
    }
    std::string toString() const {
        std::ostringstream os;
        os<<"{ col1:"<<mCol[0]<<" col2:"<<mCol[1]<<" col3:"<<mCol[2]<<'}';
        return os.str();
    }    
};
template<typename scalar> inline std::ostream& operator <<(std::ostream& os, const Matrix3x3<scalar> &rhs) {
    os<<"{ col1:"<<rhs.getCol(0)<<" col2:"<<rhs.getCol(1)<<" col3:"<<rhs.getCol(2)<<'}';
    return os;
}

template <typename T,typename S> Vector3<T> operator *(const Vector3<T>&vec, const Matrix3x3<S>&mat) {
    return Vector3<T>(mat.getCol(0).dot(vec),
                      mat.getCol(1).dot(vec),
                      mat.getCol(2).dot(vec));        
}
template <typename T> Matrix3x3<T> operator *(T other, const Matrix3x3<T>&mat) {
    return Matrix3x3<T>(mat.getCol(0)*other,mat.getCol(1)*other,mat.getCol(2)*other,COLUMNS());
}
template <typename T> Matrix3x3<T> operator /(T other, const Matrix3x3<T>&mat) {
    return Matrix3x3<T>(other/mat.getCol(0),other/mat.getCol(1),other/mat.getCol(2),COLUMNS());
}
}
#endif
