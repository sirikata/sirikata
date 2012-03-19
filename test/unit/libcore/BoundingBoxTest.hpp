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
class BoundingBoxTest : public CxxTest::TestSuite
{
    typedef Sirikata::Vector3<float> Vector3f;
    typedef Sirikata::Vector3<double> Vector3d;
public:
    void testConstruct( void )
    {
        Vector3d a(1,2,3);
        Vector3d b(4,6,8);
        BoundingBox3d3f bb(a,b);
        TS_ASSERT_EQUALS(bb.min(),a);
        TS_ASSERT_EQUALS(bb.max(),b);
        TS_ASSERT_EQUALS(bb.across(),Vector3f(3,4,5));
        BoundingBox3f3f bba(bb);
        TS_ASSERT_EQUALS(Vector3f(bb.min()),bba.min());
        TS_ASSERT_EQUALS(Vector3f(bb.max()),bba.max());
        
    }
    void testMerge ( void )
    {
        Vector3d a(1,2,3);
        Vector3d b(4,6,8);
        BoundingBox3d3f bba(a,b);
        Vector3d aB(0,1,2);
        Vector3d bB(3,4,5);
        BoundingBox3d3f bbb(aB,bB);
        BoundingBox3d3f bb=bba;
        bb.mergeIn(bbb);
        BoundingBox3d3f bbc=bbb;
        bbc.mergeIn(bba);
        TS_ASSERT_EQUALS(bb.min(),bbb.min());
        TS_ASSERT_EQUALS(bb.max(),bba.max());
        TS_ASSERT_EQUALS(bbc.min(),bbb.min());
        TS_ASSERT_EQUALS(bbc.max(),bba.max());
        
    }
};
