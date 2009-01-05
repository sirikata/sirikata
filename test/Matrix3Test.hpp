/*     Iridium Tests -- Iridium Test Suite
 *  Vector3Test.hpp
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
#include <cxxtest/TestSuite.h>
#include <string>
#include <sstream>
#include "util/Matrix3x3.hpp"
class Matrix3x3Test : public CxxTest::TestSuite 
{
    typedef Iridium::Vector3<float> Vector3f;
    typedef Iridium::Vector3<double> Vector3d;
    typedef Iridium::Matrix3x3<float> Matrix3x3f;
    typedef Iridium::Matrix3x3<double> Matrix3x3d;
    typedef Iridium::COLUMNS COLUMNS;
public:
    void testAdd( void )
    {
        Matrix3x3f a(Vector3f(5,6,7),
                     Vector3f(8,9,10),
                     Vector3f(11,12,13),COLUMNS());
        Matrix3x3f b(Vector3f(8,1,9),
                     Vector3f(-1,-4,-5),
                     Vector3f(5,2,1),COLUMNS());
        Matrix3x3f c=a+b;
        TS_ASSERT_EQUALS(c(0,0),a(0,0)+b(0,0));
        TS_ASSERT_EQUALS(c(0,1),a(0,1)+b(0,1));
        TS_ASSERT_EQUALS(c(0,2),a(0,2)+b(0,2));
        TS_ASSERT_EQUALS(c(1,0),a(1,0)+b(1,0));
        TS_ASSERT_EQUALS(c(1,1),a(1,1)+b(1,1));
        TS_ASSERT_EQUALS(c(1,2),a(1,2)+b(1,2));
        TS_ASSERT_EQUALS(c(2,0),a(2,0)+b(2,0));
        TS_ASSERT_EQUALS(c(2,1),a(2,1)+b(2,1));
        TS_ASSERT_EQUALS(c(2,2),a(2,2)+b(2,2));
    }
    void testAddDouble( void )
    {
        Matrix3x3d a(Vector3d(5,6,7),
                     Vector3d(8,9,10),
                     Vector3d(11,12,13),ROWS());
        Matrix3x3d b(Vector3d(8,1,9),
                     Vector3d(-1,-4,-5),
                     Vector3d(5,2,1),COLUMNS());
        Matrix3x3d c=a+b;
        TS_ASSERT_EQUALS(c.getCol(0).x,a(0,0)+b(0,0));
        TS_ASSERT_EQUALS(c.getCol(1).x,a(0,1)+b(0,1));
        TS_ASSERT_EQUALS(c.getCol(2).x,a(0,2)+b(0,2));
        TS_ASSERT_EQUALS(c.getCol(0).y,a(1,0)+b(1,0));
        TS_ASSERT_EQUALS(c.getCol(1).y,a(1,1)+b(1,1));
        TS_ASSERT_EQUALS(c.getCol(2).y,a(1,2)+b(1,2));
        TS_ASSERT_EQUALS(c.getCol(0).z,a(2,0)+b(2,0));
        TS_ASSERT_EQUALS(c.getCol(1).z,a(2,1)+b(2,1));
        TS_ASSERT_EQUALS(c.getCol(2).z,a(2,2)+b(2,2));
    }
    void testMatrixMultiply ( void )
    {
        Matrix3x3f a(Vector3f(55,6,7),
                     Vector3f(8,9.25,10),
                     Vector3f(11,12,13),COLUMNS());
        Matrix3x3f b(Vector3f(8,1,9),
                     Vector3f(-1.25,-4,-.5),
                     Vector3f(5,2,1),COLUMNS());
        Matrix3x3f c = a*b;
        TS_ASSERT_EQUALS(55*8+8*1+11*9,c(0,0));
        TS_ASSERT_EQUALS(6*8+9.25*1+12*9,c(1,0));
        TS_ASSERT_EQUALS(7*8+10*1+13*9,c(2,0));

        TS_ASSERT_EQUALS(55*-1.25+8*-4+11*-.5,c(0,1));
        TS_ASSERT_EQUALS(6*-1.25+9.25*-4+12*-.5,c(1,1));
        TS_ASSERT_EQUALS(7*-1.25+10*-4+13*-.5,c(2,1));

        TS_ASSERT_EQUALS(55*5+8*2+11*1,c(0,2));
        TS_ASSERT_EQUALS(6*5+9.25*2+12*1,c(1,2));
        TS_ASSERT_EQUALS(7*5+10*2+13*1,c(2,2));


        TS_ASSERT_EQUALS(a.getRow(0).dot(b.getCol(0)),c(0,0));
        TS_ASSERT_EQUALS(a.getRow(1).dot(b.getCol(0)),c(1,0));
        TS_ASSERT_EQUALS(a.getRow(2).dot(b.getCol(0)),c(2,0));
        TS_ASSERT_EQUALS(a.getRow(0).dot(b.getCol(1)),c(0,1));
        TS_ASSERT_EQUALS(a.getRow(1).dot(b.getCol(1)),c(1,1));
        TS_ASSERT_EQUALS(a.getRow(2).dot(b.getCol(1)),c(2,1));
        TS_ASSERT_EQUALS(a.getRow(0).dot(b.getCol(2)),c(0,2));
        TS_ASSERT_EQUALS(a.getRow(1).dot(b.getCol(2)),c(1,2));
        TS_ASSERT_EQUALS(a.getRow(2).dot(b.getCol(2)),c(2,2));

        TS_ASSERT_EQUALS(a.getRow(0).dot(b.getCol(0)),c(0,0));
        TS_ASSERT_EQUALS(a.getRow(1).dot(b.getCol(0)),c(1,0));
        TS_ASSERT_EQUALS(a.getRow(2).dot(b.getCol(0)),c(2,0));
        TS_ASSERT_EQUALS(a.getRow(0).dot(b.getCol(1)),c(0,1));
        TS_ASSERT_EQUALS(a.getRow(1).dot(b.getCol(1)),c(1,1));
        TS_ASSERT_EQUALS(a.getRow(2).dot(b.getCol(1)),c(2,1));
        TS_ASSERT_EQUALS(a.getRow(0).dot(b.getCol(2)),c(0,2));
        TS_ASSERT_EQUALS(a.getRow(1).dot(b.getCol(2)),c(1,2));
        TS_ASSERT_EQUALS(a.getRow(2).dot(b.getCol(2)),c(2,2));
    }
    void testToString( void )
    {
        Matrix3x3f a(Vector3f(5,6,7),
                     Vector3f(8,9.25,10),
                     Vector3f(11,12,13),COLUMNS());
        Matrix3x3d b(Vector3d(8,1,9),
                     Vector3d(-1.25,-4,-.5),
                     Vector3d(5,2,1),COLUMNS());

        std::string akey("{ col1:<5, 6, 7> col2:<8, 9.25, 10> col3:<11, 12, 13>}");
        std::string bkey("{ col1:<8, 1, 9> col2:<-1.25, -4, -0.5> col3:<5, 2, 1>}");
        TS_ASSERT_EQUALS(a.toString(),akey);
        TS_ASSERT_EQUALS(b.toString(),bkey);
        std::ostringstream oss;
        oss<<a<<b;
        TS_ASSERT_EQUALS(oss.str(),akey+bkey);
    }
};
