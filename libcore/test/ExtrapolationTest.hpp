/*  Sirikata Tests -- Sirikata Test Suite
 *  ExtrapolationTest.hpp
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
#include "util/Extrapolation.hpp"

class ExtrapolationTest : public CxxTest::TestSuite
{
    typedef Sirikata::Extrapolator<Sirikata::Location> ExtrapolatorBase;
    typedef Sirikata::Location::ErrorPredicate ErrorPredicate;
    typedef Sirikata::TimedWeightedExtrapolator<Sirikata::Location,ErrorPredicate> Extrapolator;
public:
    void testPredict( void )
    {
        using namespace Sirikata;
        ErrorPredicate ep(Location::Error(3,3));
        Time now=Time::now();
        Duration inc(.25);
        Duration hinc(.125);
        ExtrapolatorBase * base=new Extrapolator(inc,
                                                 now,
                                                 Location(Vector3d(256,16,1),
                                                          Quaternion(Vector3f(0,1,0),0),
                                                          Vector3f(0,0,0),
                                                          Vector3f(0,1,0),
                                                          0),ep);
        TS_ASSERT(base->needsUpdate(now+inc,
                                    Location(Vector3d(256,16,1)*2.0,
                                             Quaternion(Vector3f(0,1,0),0),
                                             Vector3f(0,0,0),
                                             Vector3f(0,1,0),
                                             0)));
        base->updateValue(now+inc,Location(Vector3d(256,16,1)*2.0,
                                           Quaternion(Vector3f(0,1,0),0),
                                           Vector3f(1,0,0),
                                           Vector3f(0,1,0),
                                           0));
        TS_ASSERT_EQUALS(base->extrapolate(now+inc+hinc),
                         Location(Vector3d(256,16,1)*.5+
                                  Vector3d(256,16,1)+
                                  Vector3d(.125,0,0)*.5,
                                  Quaternion(Vector3f(0,1,0),0),                         
                                  Vector3f(.5,0,0),                                  
                                  Vector3f(0,0,0),
                                  0));
        delete base;
    }
};
