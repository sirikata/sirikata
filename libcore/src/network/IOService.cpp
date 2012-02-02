/*  Sirikata Network Utilities
 *  IOService.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/network/IOStrand.hpp>
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <sirikata/core/util/Timer.hpp>

namespace Sirikata {
namespace Network {

typedef boost::asio::io_service InternalIOService;

typedef boost::asio::deadline_timer deadline_timer;
#if BOOST_VERSION==103900
typedef deadline_timer* deadline_timer_ptr;
#else
typedef std::tr1::shared_ptr<deadline_timer> deadline_timer_ptr;
#endif
typedef boost::posix_time::microseconds posix_microseconds;
using std::tr1::placeholders::_1;

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
namespace {
typedef boost::mutex AllIOServicesMutex;
typedef boost::lock_guard<AllIOServicesMutex> AllIOServicesLockGuard;
AllIOServicesMutex gAllIOServicesMutex;
typedef std::tr1::unordered_set<IOService*> AllIOServicesSet;
AllIOServicesSet gAllIOServices;
} // namespace
#endif


IOService::IOService(const String& name)
 : mName(name)
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
   ,
   mTimersEnqueued(0),
   mEnqueued(0),
   mWindowedTimerLatencyStats(100),
   mWindowedHandlerLatencyStats(100)
#endif
{
    mImpl = new boost::asio::io_service(1);

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    gAllIOServices.insert(this);
#endif
}

IOService::~IOService(){
    delete mImpl;

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    gAllIOServices.erase(this);
#endif
}

IOStrand* IOService::createStrand(const String& name) {
    IOStrand* res = new IOStrand(*this, name);
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    LockGuard lock(mMutex);
    mStrands.insert(res);
#endif
    return res;
}

uint32 IOService::pollOne() {
    return (uint32) mImpl->poll_one();
}

uint32 IOService::poll() {
    return (uint32) mImpl->poll();
}

uint32 IOService::runOne() {
    return (uint32) mImpl->run_one();
}

uint32 IOService::run() {
    return (uint32) mImpl->run();
}

void IOService::runNoReturn() {
    mImpl->run();
}

void IOService::stop() {
    mImpl->stop();
}

void IOService::reset() {
    mImpl->reset();
}

void IOService::dispatch(const IOCallback& handler) {
    mImpl->dispatch(handler);
}

void IOService::post(const IOCallback& handler) {
#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mEnqueued++;
    mImpl->post(
        std::tr1::bind(&IOService::decrementCount, this, Timer::now(), handler)
    );
#else
    mImpl->post(handler);
#endif
}

namespace {
void handle_deadline_timer(const boost::system::error_code&e, const deadline_timer_ptr& timer, const IOCallback& handler) {
    if (e)
        return;

    handler();
}
} // namespace

void IOService::post(const Duration& waitFor, const IOCallback& handler) {
#if BOOST_VERSION==103900
    static bool warnOnce=true;
    if (warnOnce) {
        warnOnce=false;
        SILOG(core,error,"Using buggy version of boost (1.39.0), leaking deadline_timer to avoid crash");
    }
#endif
    deadline_timer_ptr timer(new deadline_timer(*mImpl, posix_microseconds(waitFor.toMicroseconds())));

#ifdef SIRIKATA_TRACK_EVENT_QUEUES
    mTimersEnqueued++;
    IOCallbackWithError orig_cb = std::tr1::bind(&handle_deadline_timer, _1, timer, handler);
    timer->async_wait(
        std::tr1::bind(&IOService::decrementTimerCount, this,
            _1, Timer::now(), waitFor, orig_cb
        )
    );
#else
    timer->async_wait(std::tr1::bind(&handle_deadline_timer, _1, timer, handler));
#endif
}



#ifdef SIRIKATA_TRACK_EVENT_QUEUES
void IOService::decrementTimerCount(const boost::system::error_code& e, const Time& start, const Duration& timer_duration, const IOCallbackWithError& cb) {
    mTimersEnqueued--;
    Time end = Timer::now();
    {
        LockGuard lock(mMutex);
        mWindowedTimerLatencyStats.sample((end - start) - timer_duration);
    }
    cb(e);
}

void IOService::decrementCount(const Time& start, const IOCallback& cb) {
    mEnqueued--;
    Time end = Timer::now();
    {
        LockGuard lock(mMutex);
        mWindowedHandlerLatencyStats.sample(end - start);
    }
    cb();
}

void IOService::destroyingStrand(IOStrand* child) {
    LockGuard lock(mMutex);
    assert(mStrands.find(child) != mStrands.end());
    mStrands.erase(child);
}

void IOService::reportStats() const {
    LockGuard lock(const_cast<Mutex&>(mMutex));

    SILOG(ioservice, info, "'" << name() <<  "' IOService Statistics");
    SILOG(ioservice, info, "  Timers: " << numTimersEnqueued() << " with " << timerLatency() << " recent latency");
    SILOG(ioservice, info, "  Event handlers: " << numEnqueued() << " with " << handlerLatency() << " recent latency");

    for(StrandSet::const_iterator it = mStrands.begin(); it != mStrands.end(); it++) {
        SILOG(ioservice, info, "  Child '" << (*it)->name() << "'");
        SILOG(ioservice, info, "    Timers: " << (*it)->numTimersEnqueued() << " with " << (*it)->timerLatency() << " recent latency");
        SILOG(ioservice, info, "    Event handlers: " << (*it)->numEnqueued() << " with " << (*it)->handlerLatency() << " recent latency");
    }

}


void IOService::reportAllStats() {
    AllIOServicesLockGuard lock(gAllIOServicesMutex);
    for(AllIOServicesSet::const_iterator it = gAllIOServices.begin(); it != gAllIOServices.end(); it++)
        (*it)->reportStats();
}
#endif

} // namespace Network
} // namespace Sirikata
