// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "UUIDSpeedBenchmark.hpp"
#include <sirikata/core/util/UUID.hpp>

#define ITERATIONS 10000

namespace Sirikata {

UUIDSpeedBenchmark::UUIDSpeedBenchmark(const FinishedCallback& finished_cb)
        : Benchmark(finished_cb),
          mForceStop(false)
{
}

String UUIDSpeedBenchmark::name() {
    return "uuid-speed";
}

void UUIDSpeedBenchmark::start() {
    mForceStop = false;

    // Check throughput of timer calls
    Time start_time = Timer::now();

    UUID dummy_u = UUID::null();
    for(uint32 ii = 0; ii < ITERATIONS && !mForceStop; ii++)
        dummy_u = UUID::random();

    if (mForceStop)
        return;

    Time end_time = Timer::now();
    Duration dur = end_time - start_time;

    SILOG(benchmark,info,
          ITERATIONS << " random UUIDs, " << dur << ": "
          << (dur.toMicroseconds()*1000/float(ITERATIONS)) << "ns/call, "
          << float(ITERATIONS)/dur.toSeconds() << " calls/s");

    notifyFinished();
}

void UUIDSpeedBenchmark::stop() {
    mForceStop = true;
}


} // namespace Sirikata
