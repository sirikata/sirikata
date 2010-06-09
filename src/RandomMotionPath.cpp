/*  cbr
 *  RandomMotionPath.cpp
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

#include "RandomMotionPath.hpp"
#include <algorithm>
#include <cmath>
#include "Random.hpp"

#ifndef PI
#define PI 3.14159f
#endif

namespace Sirikata {

static Vector3f UniformSampleSphere(float u1, float u2) {
    float32 z = 1.f - 2.f * u1;
    float32 r = sqrtf(std::max(0.f, 1.f - z*z));
    float32 phi = 2.f * PI * u2;
    float32 x = r * cosf(phi);
    float32 y = r * sinf(phi);
    return Vector3f(x, y, z);
}

RandomMotionPath::RandomMotionPath(const Time& start, const Time& end, const Vector3f& startpos, float32 speed, const Duration& update_period, const BoundingBox3f& region, float zfactor) {
    TimedMotionVector3f last_motion(start, MotionVector3f(startpos, Vector3f(0,0,0)));
    mUpdates.push_back(last_motion);
    Time offset_start = start + update_period * randFloat();
    for(Time curtime = offset_start; curtime < end; curtime += update_period) {
        Vector3f curpos = last_motion.extrapolate(curtime).position();
        Vector3f dir = UniformSampleSphere( randFloat(), randFloat() ) * speed;
        dir.z = dir.z * zfactor;
        dir = dir.normal() *  speed;
        last_motion = TimedMotionVector3f(curtime, MotionVector3f(curpos, dir));
        // last_motion is what we would like, no we have to make sure we'll stay  in range
        Vector3f nextpos = last_motion.extrapolate(curtime + update_period).position();
        Vector3f nextpos_clamped = region.clamp(nextpos);
        Vector3f to_dest = nextpos_clamped - curpos;
        dir = to_dest * (1.f / update_period.toSeconds());

        last_motion = TimedMotionVector3f(curtime, MotionVector3f(curpos, dir));

        mUpdates.push_back(last_motion);
    }
}

const TimedMotionVector3f RandomMotionPath::initial() const {
    assert( !mUpdates.empty() );
    return mUpdates[0];
}

const TimedMotionVector3f* RandomMotionPath::nextUpdate(const Time& curtime) const {
    for(uint32 i = 0; i < mUpdates.size(); i++)
        if (mUpdates[i].time() > curtime) return &mUpdates[i];
    return NULL;
}

const TimedMotionVector3f RandomMotionPath::at(const Time& t) const {
    for(uint32 i = 1; i < mUpdates.size(); i++)
        if (mUpdates[i].time() > t) return mUpdates[i-1];
    return mUpdates[ mUpdates.size()-1 ];
}


} // namespace Sirikata
