/*  Sirikata Network Utilities
 *  IOTimer.cpp
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
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/network/Asio.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace Sirikata {
namespace Network {

class IOTimer::TimedOut {
public:
    static void timedOut(
            const boost::system::error_code &error,
            IOTimerWPtr wthis, uint64 tokenVal)
    {
        IOTimerPtr sharedThis (wthis.lock());
        if (!sharedThis) {
            return; // we've been deleted already.
        }
        if (error == boost::asio::error::operation_aborted) {
            return; // don't care if the timer was cancelled.
        }

        if (sharedThis->mStrand != NULL) sharedThis->chk.serializedEnter();
        IOTimer*st=&*sharedThis;

        //token only gets incremented if was canceled.  Therefore, if these two
        //are equal, means wasn't canceled.
        if (tokenVal == st->mCanceled)
            sharedThis->mFunc();
        if (sharedThis->mStrand != NULL) sharedThis->chk.serializedExit();
    }
};

IOTimer::IOTimer(IOService& io)
 : mTimer(new DeadlineTimer(io)),
   mStrand(NULL),
   mFunc(),
   mCanceled(0)
{
}

IOTimer::IOTimer(IOService& io, const IOCallback& cb)
 : mTimer(new DeadlineTimer(io)),
   mStrand(NULL),
   mFunc(),
   mCanceled(0)
{
    setCallback(cb);
}

IOTimer::IOTimer(IOStrand* ios)
 : mTimer(new DeadlineTimer(ios->service())),
   mStrand(ios),
   mFunc(),
   mCanceled(0)
{
}

IOTimer::IOTimer(IOStrand* ios, const IOCallback& cb)
 : mTimer(new DeadlineTimer(ios->service())),
   mStrand(ios),
   mFunc(),
   mCanceled(0)
{
    setCallback(cb);
}

IOTimerPtr IOTimer::create(IOService* io) {
    return IOTimerPtr(new IOTimer(*io));
}

IOTimerPtr IOTimer::create(IOService& io) {
    return IOTimerPtr(new IOTimer(io));
}

IOTimerPtr IOTimer::create(IOStrand* ios) {
    return IOTimerPtr(new IOTimer(ios));
}

IOTimerPtr IOTimer::create(IOStrand& ios) {
    return IOTimerPtr(new IOTimer(&ios));
}

IOTimerPtr IOTimer::create(IOService* io, const IOCallback& cb) {
    return IOTimerPtr(new IOTimer(*io, cb));
}

IOTimerPtr IOTimer::create(IOService& io, const IOCallback& cb) {
    return IOTimerPtr(new IOTimer(io, cb));
}

IOTimerPtr IOTimer::create(IOStrand* ios, const IOCallback& cb) {
    return IOTimerPtr(new IOTimer(ios, cb));
}

IOTimerPtr IOTimer::create(IOStrand& ios, const IOCallback& cb) {
    return IOTimerPtr(new IOTimer(&ios, cb));
}


IOTimer::~IOTimer() {
    if (mStrand != NULL) chk.serializedEnter();
    cancel();
    delete mTimer;
    if (mStrand != NULL) chk.serializedExit();
}

void IOTimer::wait(const Duration &num_seconds) {
    mTimer->expires_from_now(boost::posix_time::microseconds(num_seconds.toMicroseconds()));
    IOTimerWPtr weakThisPtr(this->shared_from_this());
    if (mStrand == NULL) {
        mTimer->async_wait(
            boost::bind(
                &IOTimer::TimedOut::timedOut,
                boost::asio::placeholders::error,
                weakThisPtr,
                mCanceled.read()
            )
        );
    }
    else {
        mTimer->async_wait(
            mStrand->wrap(
                boost::bind(
                    &IOTimer::TimedOut::timedOut,
                    boost::asio::placeholders::error,
                    weakThisPtr,
                    mCanceled.read()
                )
            )
        );
    }
}

void IOTimer::wait(const Duration &num_seconds, const IOCallback& cb) {
    setCallback(cb);
    wait(num_seconds);
}

void IOTimer::setCallback(const IOCallback& cb) {
    mFunc = cb;
}

void IOTimer::cancel() {
    if (mStrand != NULL) chk.serializedEnter();
    mCanceled++;
    mTimer->cancel();
    if (mStrand != NULL) chk.serializedExit();
}
Duration IOTimer::expiresFromNow() {
    return Duration::microseconds(mTimer->expires_from_now().total_microseconds());
}
} // namespace Network
} // namespace Sirikata
