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
#include "util/Location.hpp"
class ExtrapolationTest : public CxxTest::TestSuite
{
    typedef Sirikata::Location Location;
    typedef Sirikata::Transform Transform;
    typedef Sirikata::Vector3d Vector3d;
    typedef Sirikata::Quaternion Quaternion;
    typedef Sirikata::Vector3f Vector3f;
    typedef Sirikata::Extrapolator<Location> ExtrapolatorBase;
    typedef Location::ErrorPredicate ErrorPredicate;
    typedef Sirikata::TimedWeightedExtrapolator<Location,ErrorPredicate> Extrapolator;
public:
    void testPredict( void )
    {
        using namespace Sirikata;
        ErrorPredicate ep(Location::Error(3,3));
        Time now=Time::now();
		Duration inc=Duration::seconds(.25);
		Duration hinc=Duration::seconds(.125);
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
        assert_near(base->extrapolate(now+inc+hinc),
                         Location(Vector3d(256,16,1)*.5+
                                  Vector3d(256,16,1)+
                                  Vector3d(.125,0,0)*.5,
                                  Quaternion(Vector3f(0,1,0),0),
                                  Vector3f(.5,0,0),
                                  Vector3f(0,0,0),
                                  0));
        delete base;
    }

    template <class T>
    bool check_near(T a, T b) {
        float delta = 1e-5;
        return ((b > a-delta) && (b < a+delta));
    }

    template <class T>
    bool check_near(const Sirikata::Vector3<T> &a, const Sirikata::Vector3<T> &b) {
        return check_near(a.x, b.x) &&
            check_near(a.y, b.y) &&
            check_near(a.z, b.z);
    }
    bool check_near(const Sirikata::Quaternion &a,const Sirikata::Quaternion &b) {
        return check_near(a.w, b.w) &&
            check_near(Vector3f(a.x,a.y,a.z),Vector3f(b.x,b.y,b.z));
    }

    void assert_near(const Transform &a,const Transform &b) {
        assert_near(
            Location(a.getPosition(),
                     a.getOrientation(),
                     Vector3f::nil(),Vector3f::nil(),0),
            Location(b.getPosition(),
                     b.getOrientation(),
                     Vector3f::nil(),Vector3f::nil(),0));
    }
    void assert_near(const Location &a,const Location &b) {
        bool fail=false;
        std::ostringstream str;
        if (!check_near(a.getPosition(), b.getPosition())) {
            str << "Positions "<<a.getPosition()<<" and "<<
                b.getPosition()<<" differ."<<std::endl;
            fail=true;
        }
        if (!check_near(a.getOrientation(), b.getOrientation())) {
            str << "Orientations "<<a.getOrientation()<<" and "<<
                b.getOrientation()<<" differ."<<std::endl;
            fail=true;
        }
        if (!check_near(a.getVelocity(), b.getVelocity())) {
            str << "Velocities "<<a.getVelocity()<<" and "<<
                b.getVelocity()<<" differ."<<std::endl;
            fail=true;
        }
        if (!check_near(a.getAxisOfRotation(), b.getAxisOfRotation())) {
            str << "Axes "<<a.getAxisOfRotation()<<" and "<<
                b.getAxisOfRotation()<<" differ."<<std::endl;
            fail=true;
        }
        if (!check_near(a.getAngularSpeed(), b.getAngularSpeed())) {
            str << "Rotation speeds "<<a.getAngularSpeed()<<" and "<<
                b.getAngularSpeed()<<" differ."<<std::endl;
            fail=true;
        }
        if (fail) {
            TS_FAIL(str.str());
        }
    }


    void testTransformIdentity(void) {
        Transform frameOfReference(Vector3d(1,0,0),
                                  Quaternion(0,0,0,1, Quaternion::XYZW()));
        Transform testObject(Vector3d(1,0,0),
                                  Quaternion(0,0,0,1, Quaternion::XYZW()));
        Transform worldCoords(testObject.toWorld(frameOfReference).toLocal(frameOfReference));
        assert_near(worldCoords,testObject);
    }

    void testWorldCoordinates_position() {
        Location frameOfReference(Vector3d(1,0,0),
                                  Quaternion(0,0,0,1, Quaternion::XYZW()),
                                  Vector3f(0,100,0),
                                  Vector3f(0,0,0),0);
        Location testObject(Vector3d(1,0,0),
                                  Quaternion(0,0,0,1, Quaternion::XYZW()),
                                  Vector3f(0,200,100),
                                  Vector3f(0,0,0),0);
        Transform worldCoords(testObject);
        worldCoords = worldCoords.toWorld(frameOfReference);
        worldCoords = worldCoords.toLocal(frameOfReference);
        assert_near(worldCoords, testObject);
    }
    void testWorldCoordinates_orientation() {
        Location frameOfReference(Vector3d(0,0,0),
                                  Quaternion(Vector3f(0,1,0),0.77),
                                  Vector3f(0,0,0),
                                  Vector3f(0,0,0),0);
        Location testObject(Vector3d(1,0,1),
                                  Quaternion(Vector3f(1,0,1),0),
                                  Vector3f(0,100,50),
                                  Vector3f(0,0,1),1);
        Location worldCoords(testObject);
        worldCoords = worldCoords.toWorld(frameOfReference);
        worldCoords = worldCoords.toLocal(frameOfReference);
        assert_near(worldCoords, testObject);
    }
    void testWorldCoordinates_positionorientation() {
        Location frameOfReference(Vector3d(10,20,0),
                                  Quaternion(Vector3f(0,1,0),0.77),
                                  Vector3f(1000,50,50),
                                  Vector3f(0,0,0),0);
        Location testObject(Vector3d(1,0,1),
                                  Quaternion(Vector3f(1,0,1),0),
                                  Vector3f(0,100,50),
                                  Vector3f(0,0,1),1);
        Location worldCoords(testObject);
        worldCoords = worldCoords.toWorld(frameOfReference);
        worldCoords = worldCoords.toLocal(frameOfReference);
        assert_near(worldCoords, testObject);
    }
    void testWorldCoordinates_angvel() {
        Location frameOfReference(Vector3d(10,20,0),
                                  Quaternion(Vector3f(0,1,0),0.77),
                                  Vector3f(1000,50,50),
                                  Vector3f(0,0.71,0.71),-2);
        Location testObject(Vector3d(1,0,1),
                                  Quaternion(Vector3f(1,0,1),0),
                                  Vector3f(0,100,50),
                                  Vector3f(0,0,0),0);
        Location worldCoords(testObject);
        worldCoords = worldCoords.toWorld(frameOfReference);
        worldCoords = worldCoords.toLocal(frameOfReference);
        assert_near(worldCoords, testObject);
    }
    void testWorldCoordinates_all() {
        Location frameOfReference(Vector3d(10,20,0),
                                  Quaternion(Vector3f(0,1,0),0.77),
                                  Vector3f(1000,50,50),
                                  Vector3f(0,0.71,0.71),-2);
        Location testObject(Vector3d(1,0,1),
                                  Quaternion(Vector3f(1,0,1),0),
                                  Vector3f(0,100,50),
                                  Vector3f(0,0,1),10);
        Location worldCoords(testObject);
        worldCoords = worldCoords.toWorld(frameOfReference);
        worldCoords = worldCoords.toLocal(frameOfReference);
        assert_near(worldCoords, testObject);
    }

};
