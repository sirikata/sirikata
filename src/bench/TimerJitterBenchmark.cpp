/*  cbr
 *  TimerJitterBenchmark.cpp
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

#include "TimerJitterBenchmark.hpp"
#include "Timer.hpp"

#define ITERATIONS 1000000

namespace CBR {

TimerJitterBenchmark::TimerJitterBenchmark(const FinishedCallback& finished_cb)
        : Benchmark(finished_cb),
          mForceStop(false)
{
}

String TimerJitterBenchmark::name() {
    return "timer-jitter";
}

void TimerJitterBenchmark::start() {
    mForceStop = false;

    Time start_time = Timer::now();

    // We want variance of time_1 - time_0, i.e. if we expect timestamps at a
    // given rate, what the variance (accuracy) of these is.  We compute this as
    // E(X^2) - E(X)^2.
    int64 diff = 0;
    int64 diff2 = 0;

    Time tlast = Timer::now();
    for(uint32 ii = 0; ii < ITERATIONS && !mForceStop; ii++) {
        Time tnow = Timer::now();
        int64 since_last_nano = (tnow - tlast).toMicroseconds()*1000;

        diff += since_last_nano;
        diff2 += since_last_nano*since_last_nano;

        tlast = tnow;
    }

    if (mForceStop)
        return;

    Time end_time = Timer::now();
    Duration dur = end_time - start_time;

    double diff_avg = diff / double(ITERATIONS);
    double diff2_avg = diff2 / double(ITERATIONS);

    int64 diff_var = diff2_avg - (diff_avg*diff_avg);

    SILOG(benchmark,info,
          ITERATIONS << " timer invokations, " << dur << ": "
          << "stddev " << sqrt(diff_var) << " ns"
          );

    notifyFinished();
}

void TimerJitterBenchmark::stop() {
    mForceStop = true;
}


} // namespace CBR
