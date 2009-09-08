/*  cbr
 *  TimeProfiler.hpp
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

#ifndef _CBR_TIME_PROFILER_HPP_
#define _CBR_TIME_PROFILER_HPP_

#include "Utility.hpp"
#include "Timer.hpp"

namespace CBR {

/** A simple class which helps to time profiling to determine
 *  what fraction of time each component of a loop is taking.
 *  It assumes each step is performed every time.  Events with
 *  human-friendly names are added to the list and then the main
 *  loop simply indicates when each stage completes.  When complete,
 *  call the report method to get a simple analysis.
 */
class TimeProfiler {
public:
    TimeProfiler(const String& name);

    void addStage(const String& name);

    void startIteration();
    void finishedStage();

    void report() const;

private:
    struct StageInfo {
        StageInfo();

        String name;
        Duration minimum;
        Duration maximum;
        Duration sum;
        uint64 its;
    };

    String mName;

    typedef std::vector<StageInfo> StageList;
    StageList mStages;

    Time mLastFinishTime;
    uint32 mCurrentStage;
}; // class TimeProfiler

} // namespace CBR

#endif //_CBR_TIME_PROFILER_HPP_
