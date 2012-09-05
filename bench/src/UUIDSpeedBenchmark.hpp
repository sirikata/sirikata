// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_UUID_SPEED_BENCHMARK_HPP_
#define _SIRIKATA_UUID_SPEED_BENCHMARK_HPP_

#include "Benchmark.hpp"

namespace Sirikata {

/** Test the throughput of generating random UUIDs */
class UUIDSpeedBenchmark : public Benchmark {
  public:
    typedef std::tr1::function<void()> FinishedCallback;

    static Benchmark* create(const FinishedCallback& finished_cb, const String& _param) {
        return new UUIDSpeedBenchmark(finished_cb);
    }

    UUIDSpeedBenchmark(const FinishedCallback& finished_cb);

    virtual String name();

    virtual void start();
    virtual void stop();

  private:
    bool mForceStop;
}; // class UUIDSpeedBenchmark

} // namespace Sirikata

#endif //_SIRIKATA_UUID_SPEED_BENCHMARK_HPP_
