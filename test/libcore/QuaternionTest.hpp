/*  Sirikata Tests -- Sirikata Test Suite
 *  QuaternionTest.hpp
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
class QuaternionTest : public CxxTest::TestSuite
{
    typedef Sirikata::Vector3f Vector3f;
    typedef Sirikata::Quaternion Quaternion;
public:
    void testAdd( void )
    {
        Quaternion a(1,2,3,4,Quaternion::XYZW());
		Quaternion b(5,6,8,7,Quaternion::XYZW());
        Quaternion c=a+b;
        TS_ASSERT_EQUALS(c.x,a.x+b.x);
        TS_ASSERT_EQUALS(c.y,a.y+b.y);
        TS_ASSERT_EQUALS(c.z,a.z+b.z);
        TS_ASSERT_EQUALS(c.w,a.w+b.w);
    }
    void testToString( void )
    {
        Quaternion a(1,2,3.25,4.125,Quaternion::XYZW());
        std::string akey("<1,2,3.25,4.125>");
        Quaternion b(-4,-.25,-.0625,1.25,Quaternion::XYZW());
        std::string bkey("<-4,-0.25,-0.0625,1.25>");
        TS_ASSERT_EQUALS(a.toString(),akey);
        TS_ASSERT_EQUALS(b.toString(),bkey);
        std::ostringstream oss;
        oss<<a<<b;
        TS_ASSERT_EQUALS(oss.str(),akey+bkey);
    }

    void assert_near(const Vector3f &a, const Vector3f &b) {
        TS_ASSERT_DELTA(a.x, b.x, 1e-6);
        TS_ASSERT_DELTA(a.y, b.y, 1e-6);
        TS_ASSERT_DELTA(a.z, b.z, 1e-6);
    }
    void assert_near(const Quaternion &a,const Quaternion &b) {
        TS_ASSERT_DELTA(a.w, b.w, 1e-6);
        assert_near(Vector3f(a.x,a.y,a.z),Vector3f(b.x,b.y,b.z));
    }

    void testIdentityOps( void) {
        Quaternion id = Quaternion::identity();
        Quaternion test(Vector3f(1,0,0),0.5);
        TS_ASSERT_EQUALS(test*id, test);
        TS_ASSERT_EQUALS(id*test, test);
        assert_near(id.normal(), id);

        Quaternion inverseid = id.inverse();
        TS_ASSERT_EQUALS(inverseid, id);

    }

    void testInverseCompose( void) {
        Quaternion id = Quaternion::identity();
        Quaternion test(Vector3f(1,0,0),0.5);
        assert_near(test * test.inverse(), id);
        assert_near(test.inverse() * test, id);
    }
    void testAngleAxis(void) {
        float origAngle = 1.234f;
        Vector3f origAxis = Vector3f(.5f,.5f,1.0f).normal();
        Quaternion test2(origAxis,origAngle);
        float angle;
        Vector3f axis;
        test2.toAngleAxis(angle,axis);
        TS_ASSERT_DELTA(origAngle, angle, 1.0e-6f);
        assert_near(origAxis,axis);
    }
};
