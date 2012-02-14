// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_TRACE_WINDOWED_STATS_HPP_
#define _SIRIKATA_CORE_TRACE_WINDOWED_STATS_HPP_

#include <sirikata/core/util/CircularBuffer.hpp>

namespace Sirikata {
namespace Trace {

/** Tracks a window of a fixed number of recent samples and can report
 *  statistics about them, e.g. average, max, min, etc. The sample type must
 *  specify basic math operations.
 */
template<typename SampleType>
class WindowedStats {
public:
    WindowedStats(std::size_t nsamples)
     : mSamples(nsamples)
    {
    }

    /** Record a sample. */
    void sample(const SampleType& s) {
        mSamples.push(s);
    }

    SampleType average() const {
        SampleType sum;
        if (mSamples.empty()) return sum;
        for(std::size_t i = 0; i < mSamples.size(); i++)
            sum = sum + mSamples[i];
        return sum / mSamples.size();
    }

    const CircularBuffer<SampleType>& getSamples() const
    {
        return mSamples;
    }
    
private:
    CircularBuffer<SampleType> mSamples;
}; // class RecentStats

} // namespace Trace
} // namespace Sirikata

#endif //_SIRIKATA_CORE_TRACE_WINDOWED_STATS_HPP_
