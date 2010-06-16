/*  Sirikata
 *  BenchmarkRunner.hpp
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

#ifndef _SIRIKATA_BENCHMARK_RUNNER_HPP_
#define _SIRIKATA_BENCHMARK_RUNNER_HPP_

#include "Benchmark.hpp"
#include "BenchmarkFactory.hpp"

namespace Sirikata {

/** BenchmarkRunner handles running benchmarks with a specified timeout. */
class BenchmarkRunner {
  public:
    /** Create a BenchmarkRunner that will create benchmarks using the
     *  specified factory and kill benchmarks after the specified timeout.
     *  \param bf the factory to use to instantiate each Benchmark
     *  \param timeout maximum time to wait before forcibly killing it
     */
    BenchmarkRunner(BenchmarkFactory& bf, const Duration& timeout);

    ~BenchmarkRunner();

    /** Run an individual benchmark.
     *  \param _name the name of the benchmark to run
     */
    void run(const String& _name);

    /** Run an individual benchmark with an argument.
     *  \param _name the name of the benchmark to run
     *  \param _param the parameter to pass to the benchmark
     */
    void run(const String& _name, const String& _param);

  private:
    /** Main process for benchmarking thread. */
    void benchmarkThread(Benchmark* bm);

    /** Handle a graceful benchmark completion of a benchmark. */
    void handleBenchmarkFinished();

    /** Handle a timeout on a benchmark run by forcefully shutting down
     *  the benchmark.
     */
    void handleTimeout(Benchmark* bm);

    BenchmarkFactory& mFactory;
    Duration mTimeout;

    Network::IOService* mIOService;
    bool mForcefulStop;
}; // class BenchmarkRunner

} // namespace Sirikata

#endif //_SIRIKATA_BENCHMARK_RUNNER_HPP_
