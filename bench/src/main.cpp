/*  Sirikata
 *  main.cpp
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

#include "BenchmarkRunner.hpp"

#include "TimerSpeedBenchmark.hpp"
#include "TimerJitterBenchmark.hpp"
#include "TimerMonotonicityBenchmark.hpp"
#include "SSTBenchmark.hpp"

using namespace Sirikata;

typedef std::list<String> BenchmarkList;

void runAll(BenchmarkRunner& runner, const BenchmarkList all_benchmarks) {
    for(std::list<String>::const_iterator it = all_benchmarks.begin();
        it != all_benchmarks.end();
        it++)
        runner.run(*it);
}

int main(int argc, char** argv) {
    BenchmarkFactory factory;
    BenchmarkList all_benchmarks;

// Adds a benchmark to the factory and the list
#define ADD_BENCHMARK(name, create_cb) \
    factory.registerConstructor(#name, create_cb); \
    all_benchmarks.push_back(#name)

    // Benchmark registrations
    ADD_BENCHMARK(timer-speed, TimerSpeedBenchmark::create);
    ADD_BENCHMARK(timer-jitter, TimerJitterBenchmark::create);
    ADD_BENCHMARK(timer-monotonicity, TimerMonotonicityBenchmark::create);

    ADD_BENCHMARK(ping, SSTBenchmark::create);
    BenchmarkRunner runner(factory, Duration::seconds(30.f));


    // No arguments, defaults to all benchmarks
    if (argc < 2) {
        runAll(runner, all_benchmarks);
        return 0;
    }

    // Otherwise, run each one that's specified
    for(int32 argsi = 1; argsi < argc; argsi+=2) {
        String arg = argv[argsi];

        if (arg == "all") {
            runAll(runner, all_benchmarks);
        }
        else {
            runner.run(arg,argsi+1<argc?argv[argsi+1]:"");
        }
    }

    return 0;
}
