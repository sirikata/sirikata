/*  cbr
 *  MotionVector.hpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_MOTION_VECTOR_HPP_
#define _CBR_MOTION_VECTOR_HPP_

#include "Utility.hpp"

namespace CBR {

template<typename CoordType>
class MotionVector {
public:
    typedef CoordType PositionType;
    typedef CoordType VelocityType;

    MotionVector()
     : mStart(0,0,0), mDirection(0,0,0)
    {
    }

    MotionVector(const CoordType& pos, const CoordType& vel)
     : mStart(pos), mDirection(vel)
    {
    }

    const CoordType& position() const {
        return mStart;
    }

    const CoordType& velocity() const {
        return mDirection;
    }

    MotionVector extrapolate(const Duration& dt) const {
        return MotionVector(mStart + mDirection * dt.toSeconds(), mDirection);
    }

private:
    CoordType mStart;
    CoordType mDirection;
}; // class MotionVector

typedef MotionVector<Vector3f> MotionVector3f;
typedef MotionVector<Vector3d> MotionVector3d;

template<typename MotionVectorType>
class TimedMotionVector : public TemporalValue<MotionVectorType> {
public:
    typedef TemporalValue<MotionVectorType> Base;
    typedef typename MotionVectorType::PositionType PositionType;
    typedef typename MotionVectorType::VelocityType VelocityType;

    TimedMotionVector()
     : TemporalValue<MotionVectorType>()
    {}
    TimedMotionVector(const Time& when, const MotionVectorType& l)
     : TemporalValue<MotionVectorType>(when, l)
    {}

    Time updateTime() const {
        return Base::time();
    }

    const PositionType& position() const {
        return Base::value().position();
    }

    PositionType position(const Duration& dt) const {
        return Base::value().position() + Base::value().velocity() * dt.toSeconds();
    }

    PositionType position(const Time& t) const {
        return position(t - Base::time());
    }

    const VelocityType& velocity() const {
        return Base::value().velocity();
    }
    TimedMotionVector& operator+=(const PositionType &offset) {
        Base::value().position()+=offset;
        return *this;
    }
    void update(const Time& t, const PositionType& pos, const VelocityType& vel) {
        assert(t > Base::time());
        Base::updateValue(t, MotionVectorType(pos, vel));
    }
};

typedef TimedMotionVector<MotionVector3f> TimedMotionVector3f;
typedef TimedMotionVector<MotionVector3d> TimedMotionVector3d;

} // namespace CBR

#endif //_CBR_MOTION_VECTOR_HPP_
