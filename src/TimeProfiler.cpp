/*  cbr
 *  TimeProfiler.cpp
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


#include "TimeProfiler.hpp"

#define PROFILER_LOG(level,msg) SILOG(profiler,level,"[PROFILER] " << msg)

namespace CBR {

TimeProfiler::StageInfo::StageInfo()
 : minimum( Duration::seconds(100000) ),
   maximum( Duration::zero() ),
   sum( Duration::zero() ),
   its(0)
{
}

TimeProfiler::TimeProfiler(const String& name)
 : mName(name),
   mLastFinishTime(Time::null()),
   mCurrentStage(0)
{
}

void TimeProfiler::addStage(const String& name) {
    StageInfo sinfo;
    sinfo.name = name;
    mStages.push_back(sinfo);
}

void TimeProfiler::startIteration() {
    mLastFinishTime = Timer::now();
    mCurrentStage = 0;
}

void TimeProfiler::finishedStage() {
    Time now = Timer::now();
    Duration dur = now - mLastFinishTime;

    if (dur < mStages[mCurrentStage].minimum)
        mStages[mCurrentStage].minimum = dur;

    if (dur > mStages[mCurrentStage].maximum)
        mStages[mCurrentStage].maximum = dur;

    mStages[mCurrentStage].sum += dur;
    mStages[mCurrentStage].its++;


    mLastFinishTime = now;
    mCurrentStage++;
}

void TimeProfiler::report() const {
    PROFILER_LOG(info,"Profiler report: " << mName);
    for(StageList::const_iterator it = mStages.begin(); it != mStages.end(); it++) {
        PROFILER_LOG(info,"Stage: " << it->name << " -- Avg: " << (it->sum / (float64)it->its).toSeconds() << "s Min: " << it->minimum.toSeconds() << "s Max:" << it->maximum.toSeconds() << "s");
    }
}

} // namespace CBR
