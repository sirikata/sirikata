/*  Sirikata Network Utilities
 *  IOStrand.cpp
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
#include <sirikata/core/network/IOStrand.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/util/Timer.hpp>

namespace Sirikata {
namespace Network {

IOStrand::IOStrand(IOService& io, const String& name)
 : mService(io),
   mName(name)
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
   ,
   mTimersEnqueued(0),
   mEnqueued(0),
   mWindowedTimerLatencyStats(100),
   mWindowedHandlerLatencyStats(100)
#endif
{
    mImpl = new InternalIOStrand(io);
}

IOStrand::~IOStrand() {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mService.destroyingStrand(this);
#endif
    delete mImpl;
}

IOService& IOStrand::service() const {
    return mService;
}

void IOStrand::dispatch(const IOCallback& handler, const char* tag) {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mEnqueued++;
    {
        LockGuard lock(mMutex);
        if (mTagCounts.find(tag) == mTagCounts.end())
            mTagCounts[tag] = 0;
        mTagCounts[tag]++;
    }
    mService.dispatch(
        mImpl->wrap(
            std::tr1::bind(&IOStrand::decrementCount, this, Timer::now(), handler, tag)
        ),
        "(IOStrands)",tag
    );
#else
    mService.dispatch( mImpl->wrap( handler ) );
#endif
}

void IOStrand::post(const IOCallback& handler, const char* tag) {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mEnqueued++;
    {
        LockGuard lock(mMutex);
        if (mTagCounts.find(tag) == mTagCounts.end())
            mTagCounts[tag] = 0;
        mTagCounts[tag]++;
    }
    mService.post(
        mImpl->wrap(
            std::tr1::bind(&IOStrand::decrementCount, this, Timer::now(), handler, tag)
        ),
        "(IOStrands)", tag
    );
#else
    mService.post( mImpl->wrap( handler ) );
#endif
}

void IOStrand::post(const Duration& waitFor, const IOCallback& handler, const char* tag) {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mTimersEnqueued++;
    {
        LockGuard lock(mMutex);
        if (mTagCounts.find(tag) == mTagCounts.end())
            mTagCounts[tag] = 0;
        mTagCounts[tag]++;
    }
    mService.post(
        waitFor,
        mImpl->wrap(
            std::tr1::bind(&IOStrand::decrementTimerCount, this, Timer::now(), waitFor, handler, tag)
        ),
        "(IOStrands)",tag
    );
#else
    mService.post(waitFor, mImpl->wrap( handler ) );
#endif
}

IOCallback IOStrand::wrap(const IOCallback& handler) {
    return mImpl->wrap(handler);
}


#ifdef SIRIKATA_TRACK_EVENT_QUEUES
void IOStrand::decrementTimerCount(const Time& start, const Duration& timer_duration, const IOCallback& cb, const char* tag) {
    mTimersEnqueued--;
    Time end = Timer::now();
    {
        LockGuard lock(mMutex);
        mWindowedTimerLatencyStats.sample((end - start) - timer_duration);

        assert(mTagCounts.find(tag) != mTagCounts.end());
        mTagCounts[tag]--;
    }
    cb();
}

void IOStrand::decrementCount(const Time& start, const IOCallback& cb, const char* tag) {
    mEnqueued--;
    Time end = Timer::now();
    {
        LockGuard lock(mMutex);
        mWindowedHandlerLatencyStats.sample(end - start);

        assert(mTagCounts.find(tag) != mTagCounts.end());
        mTagCounts[tag]--;
    }
    cb();
}

IOStrand::TagCountMap IOStrand::enqueuedTags() const {
    LockGuard lock(const_cast<Mutex&>(mMutex));
    return mTagCounts;
};

#endif
} // namespace Network
} // namespace Sirikata
