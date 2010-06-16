/*  Sirikata
 *  BenchmarkRunner.cpp
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
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/util/Thread.hpp>

namespace Sirikata {

BenchmarkRunner::BenchmarkRunner(BenchmarkFactory& bf, const Duration& timeout)
        : mFactory(bf),
          mTimeout(timeout),
          mIOService( Network::IOServiceFactory::makeIOService() ),
          mForcefulStop(false)
{
}

BenchmarkRunner::~BenchmarkRunner() {
    Network::IOServiceFactory::destroyIOService(mIOService);
}

void BenchmarkRunner::run(const String& _name) {
    run(_name, "");
}

void BenchmarkRunner::run(const String& _name, const String& _param) {
    mForcefulStop = false;
    mIOService->reset();

    Benchmark* bm = mFactory.getConstructor(_name)(
        std::tr1::bind(&BenchmarkRunner::handleBenchmarkFinished, this),
        _param
                                                   );

    Thread bm_thread(
        std::tr1::bind(&BenchmarkRunner::benchmarkThread, this, bm)
                     );

    Network::IOTimerPtr timer = Network::IOTimer::create(mIOService);
    timer->wait(
        mTimeout,
        std::tr1::bind(&BenchmarkRunner::handleTimeout, this, bm)
                );

    mIOService->run();
    timer->cancel();

    bm_thread.join();

    delete bm;
}

void BenchmarkRunner::benchmarkThread(Benchmark* bm) {
    SILOG(benchmark,info,"BENCHMARK START - " << bm->name());

    bm->start();

    if (!mForcefulStop)
        SILOG(benchmark,info,"BENCHMARK END - " << bm->name());
}

void BenchmarkRunner::handleBenchmarkFinished() {
    mIOService->stop();
}

void BenchmarkRunner::handleTimeout(Benchmark* bm) {
    mForcefulStop = true;

    SILOG(benchmark,info,"BENCHMARK TIMEOUT - " << bm->name());

    // Forcefully stop
    bm->stop();

    // And exit of IOService will consequently call delete bm
}

} // namespace Sirikata
