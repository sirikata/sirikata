/*  Sirikata
 *  MotionQuaternion.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_MOTION_QUATERNION_HPP_
#define _SIRIKATA_MOTION_QUATERNION_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/util/TemporalValue.hpp>


namespace Sirikata {

class MotionQuaternion {
public:
    typedef Quaternion PositionType;
    typedef Quaternion VelocityType;

    MotionQuaternion()
     : mStart(Quaternion::identity()),
       mDirection(Quaternion::identity())
    {
    }

    MotionQuaternion(const Quaternion& pos, const Quaternion& vel)
     : mStart(pos), mDirection(vel)
    {
    }

    const Quaternion& position() const {
        return mStart;
    }

    const Quaternion& velocity() const {
        return mDirection.getDirection();
    }

    Quaternion extrapolatePosition(const Duration& dt) const {
        return (mStart * (mDirection.getDirection().normal().exp(mDirection.getMag() * dt.toSeconds())));
    }


    Quaternion extrapolateVelocity(const Duration& dt) const {
        return mDirection.getDirection();
    }

    MotionQuaternion extrapolate(const Duration& dt) const {
        return MotionQuaternion(extrapolatePosition(dt), extrapolateVelocity(dt));
    }

private:
    Quaternion mStart;

    struct DirectionQuat
    {
        DirectionQuat(Quaternion newDir)
         : mDir(newDir),
           mDirMag(newDir.length())
        {
        }
        void setDirectction(const Quaternion& newDir)
        {
            mDir = newDir;
            mDirMag = newDir.length();
        }

        const Quaternion& getDirection() const
        {
            return mDir;
        }
        const float& getMag() const
        {
            return mDirMag;
        }
        
    private:
        Quaternion mDir;
        float mDirMag;
    };

    DirectionQuat mDirection;
    
}; // class MotionQuaternion

class TimedMotionQuaternion : public TemporalValue<MotionQuaternion> {
public:
    typedef TemporalValue<MotionQuaternion> Base;
    typedef MotionQuaternion::PositionType PositionType;
    typedef MotionQuaternion::VelocityType VelocityType;

    TimedMotionQuaternion()
     : TemporalValue<MotionQuaternion>()
    {}
    TimedMotionQuaternion(const Time& when, const MotionQuaternion& l)
     : TemporalValue<MotionQuaternion>(  when , l)
       //: //TemporalValue<MotionQuaternion>( (when == Time::null()) ? (Time::local()) : when , l)
       //FIXME: making it so that TimedMotionQuaternion does not accept null time.
       //Only doing this because we haven't solved synchronization yet, and it's
       //making it so that can't put default motion values into initialization
       //file.  
    {}

    Time updateTime() const {
        return Base::time();
    }

    const PositionType& position() const {
        return Base::value().position();
    }

    PositionType position(const Duration& dt) const {
        return Base::value().extrapolatePosition(dt);
    }

    PositionType position(const Time& t) const {
        return position(t - Base::time());
    }

    const VelocityType& velocity() const {
        return Base::value().velocity();
    }

    TimedMotionQuaternion& operator+=(const PositionType &offset) {
        update(Base::time(), Base::value().position() * offset, Base::value().velocity());
        return *this;
    }

    void update(const Time& t, const PositionType& pos, const VelocityType& vel) {
        assert(t > Base::time());
        Base::updateValue(t, MotionQuaternion(pos, vel));
    }
};

} // namespace Sirikata

#endif //_SIRIKATA_MOTION_QUATERNION_HPP_
