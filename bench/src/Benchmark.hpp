/*  Sirikata
 *  Benchmark.hpp
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

#ifndef _SIRIKATA_BENCHMARK_HPP_
#define _SIRIKATA_BENCHMARK_HPP_

#include <sirikata/cbrcore/PollingService.hpp>

namespace Sirikata {

/** Benchmark is a service which performs a single micro-benchmark. Each
 *  benchmark is a service -- it should start its test when start() is called
 *  and should try its best to respect the call to stop().  If even that
 *  fails, the benchmark will be forcibly destroyed.
 *
 *  The benchmark is passed a callback which it should invoke when the
 *  benchmark has shutdown cleanly. This allows the benchmark to finish at
 *  its own pace and clean up gracefully, unless the benchmark takes too
 *  long
 */
class Benchmark : public Service {
  public:
    typedef std::tr1::function<void()> FinishedCallback;

    /** Construct a benchmark which will invoke the specified callback when it
     *  has completed.
     */
    Benchmark(const FinishedCallback& finished_cb)
            : mFinishedCallback(finished_cb)
    {}

    virtual ~Benchmark() {}

    /** Get the name of this benchmark, used for reporting. */
    virtual String name() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

  protected:
    /** Notify the parent of this benchmark that the run has finished.  This
     *  should be used by benchmark implementations to indicate when they have
     *  finished and cleaned up without issues.
     */
    void notifyFinished() const {
        mFinishedCallback();
    }

  private:
    FinishedCallback mFinishedCallback;
}; // class Benchmark

} // namespace Sirikata

#endif //_SIRIKATA_BENCHMARK_HPP_
