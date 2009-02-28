/*  Sirikata Tests -- Sirikata Test Suite
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

#include <cxxtest/TestSuite.h>
class Vector3Test : public CxxTest::TestSuite
{
    typedef Sirikata::Vector3<float> Vector3f;
    typedef Sirikata::Vector3<double> Vector3d;
public:
    void testAdd( void )
    {
        Vector3f a(Vector3d(5,6,7));
        Vector3f b(Vector3d(8,1,9));
        Vector3f c=a+b;
        TS_ASSERT_EQUALS(c.x,a.x+b.x);
        TS_ASSERT_EQUALS(c.y,a.y+b.y);
        TS_ASSERT_EQUALS(c.z,a.z+b.z);
    }
    void testAddDouble( void )
    {
        Vector3d a(Vector3f(5,6,7));
        Vector3d b(Vector3f(8,1,9));
        Vector3d c=a+b;
        TS_ASSERT_EQUALS(c.x,a.x+b.x);
        TS_ASSERT_EQUALS(c.y,a.y+b.y);
        TS_ASSERT_EQUALS(c.z,a.z+b.z);
    }
    void testDotCross ( void )
    {
        Vector3d a(Vector3f(5,6,7));
        Vector3d b(Vector3f(8,1,9));
        Vector3d c=a.cross(b);
        TS_ASSERT_DELTA(c.dot(a),0,1e-08);
        TS_ASSERT_DELTA(c.dot(b),0,1e-08);
    }
    void testLengthFloat ( void )
    {
        Vector3f a(Vector3d(5,6.125,7));
        Vector3f b(Vector3d(8,-1.25,9.0625));
        Vector3f c=a.cross(b);
        Vector3f nearlyzero;
        nearlyzero=Vector3f(c.dot(a),c.dot(b),c.lengthSquared()-c.dot(c));
        TS_ASSERT_DELTA(a.normal().length(),1,1e-08);
        TS_ASSERT_DELTA(b.normal().length(),1,1e-08);
        TS_ASSERT_DELTA(c.normal().length(),1,1e-08);
        TS_ASSERT_DELTA(nearlyzero.normal().length(),0,1e-08);
    }
    void testLengthDouble ( void )
    {
        Vector3d a(Vector3f(5,6,7));
        Vector3d b(Vector3f(8,1,9));
        Vector3d c=a.cross(b);
        Vector3d nearlyzero;
        nearlyzero=c.dot(b);
        TS_ASSERT_DELTA(a.normal().length(),1,1e-08);
        TS_ASSERT_DELTA(b.normal().length(),1,1e-08);
        TS_ASSERT_DELTA(c.normal().length(),1,1e-08);
        TS_ASSERT_DELTA(nearlyzero.normal().length(),0,1e-08);
    }
    void testToString( void )
    {
        Vector3d a(1,2,3.25);
        std::string akey("<1,2,3.25>");
        Vector3f b(-4,-.25,-.0625);
        std::string bkey("<-4,-0.25,-0.0625>");
        TS_ASSERT_EQUALS(a.toString(),akey);
        TS_ASSERT_EQUALS(b.toString(),bkey);
        std::ostringstream oss;
        oss<<a<<b;
        TS_ASSERT_EQUALS(oss.str(),akey+bkey);
    }
};
