/*  Sirikata
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/service/TimeProfiler.hpp>

#define PROFILER_LOG(level,msg) SILOG(profiler,level,"[PROFILER] " << msg)

namespace Sirikata {

TimeProfiler::Stage::Stage(TimeProfiler* parent, const String& name)
 : mParent(parent),
   mName(name),
   mStartTime( Time::null() ),
   mMinimum( Duration::seconds(100000) ),
   mMaximum( Duration::zero() ),
   mSum( Duration::zero() ),
   mIts(0),
   mValid(true)
{
}

TimeProfiler::Stage::~Stage() {
    if (mValid)
        mParent->remove(this);
}

void TimeProfiler::Stage::invalidate() {
    mValid = false;
}

void TimeProfiler::Stage::started() {
    mStartTime = Timer::now();
}

void TimeProfiler::Stage::finished() {
    Time now = Timer::now();
    Duration dur = now - mStartTime;

    if (dur < mMinimum)
        mMinimum = dur;

    if (dur > mMaximum)
        mMaximum = dur;

    mSum += dur;
    mIts++;
}

String TimeProfiler::Stage::name() const {
    return mName;
}

Duration TimeProfiler::Stage::avg() const {
    return (mSum / (float64)mIts);
}

Duration TimeProfiler::Stage::minimum() const {
    return mMinimum;
}

Duration TimeProfiler::Stage::maximum() const {
    return mMaximum;
}

uint64 TimeProfiler::Stage::its() const {
    return mIts;
}

void TimeProfiler::Stage::report(const String& indent) const {
    PROFILER_LOG(info,"Stage: " << indent << name() << " -- Avg: " << avg() << " Min: " << minimum() << " Max:" << maximum() << " Sum: " << mSum << "  Its: " << its());
}

TimeProfiler::TimeProfiler(const String& name)
 : mName(name)
{
}

TimeProfiler::~TimeProfiler() {
    // Ownership of stages remains with the caller of addStage. Instead, we just
    // invalidate pointers to us in those stages that still exist

    for(GroupMap::iterator git = mGroups.begin(); git != mGroups.end(); git++) {
        StageList& stages = git->second;
        for(StageList::iterator it = stages.begin(); it != stages.end(); it++)
            (*it)->invalidate();
    }

    for(StageList::iterator it = mFreeStages.begin(); it != mFreeStages.end(); it++)
        (*it)->invalidate();
}

TimeProfiler::Stage* TimeProfiler::addStage(const String& name) {
    Stage* sinfo = new Stage(this, name);
    mFreeStages.push_back(sinfo);
    return sinfo;
}

TimeProfiler::Stage* TimeProfiler::addStage(const String& group_name, const String& name) {
    Stage* sinfo = new Stage(this, name);
    mGroups[group_name].push_back(sinfo);
    return sinfo;
}

void TimeProfiler::remove(Stage* stage) {
    // This could be handled better, but here we just iterate through all the
    // stages looking for the one we're removing.
    for(GroupMap::iterator git = mGroups.begin(); git != mGroups.end(); git++) {
        StageList& stages = git->second;
        for(StageList::iterator it = stages.begin(); it != stages.end(); it++) {
            if (*it == stage) {
                stages.erase(it);
                break;
            }
        }
    }
    for(StageList::iterator it = mFreeStages.begin(); it != mFreeStages.end(); it++) {
        if (*it == stage) {
            mFreeStages.erase(it);
            break;
        }
    }
}

void TimeProfiler::report() const {
    PROFILER_LOG(info,"Profiler report: " << mName);

    // Group stages
    for(GroupMap::const_iterator git = mGroups.begin(); git != mGroups.end(); git++) {
        String group_name = git->first;
        const StageList& stages = git->second;
        PROFILER_LOG(info,"Group: " << group_name);
        for(StageList::const_iterator it = stages.begin(); it != stages.end(); it++) {
            Stage* stage = *it;
            stage->report("-");
        }
    }

    // Free stages
    for(StageList::const_iterator it = mFreeStages.begin(); it != mFreeStages.end(); it++) {
        Stage* stage = *it;
        stage->report("");
    }
}

} // namespace Sirikata
