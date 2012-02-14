/*  Sirikata
 *  Poller.cpp
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
#include <sirikata/core/service/Poller.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#ifdef _WIN32
#pragma warning (disable:4355)//this within constructor initializer
#endif
namespace Sirikata {

Poller::Poller(Network::IOStrand* str, const Network::IOCallback& cb, const char* cb_tag, const Duration& max_rate, bool accurate)
 : mStrand(str),
   mTimer( Network::IOTimer::create(str->service()) ),
   mMaxRate(max_rate),
   mAccurate(accurate),
   mUnschedule(false),
   mUserCB(cb),
   mCBTag(cb_tag)
{
    Network::IOTimerWPtr wtimer(mTimer->shared_from_this());
    mCB = mStrand->wrap(std::tr1::bind(&Poller::handleExec, this, wtimer));
    mTimer->setCallback(mCB);
}

Poller::~Poller() {
}

void Poller::start() {
    // Always make the first callback run immediately
    mStrand->post(mCB, mCBTag);
}

void Poller::setupNextTimeout(const Duration& user_time) {
    if (mMaxRate != Duration::microseconds(0)) {
        if (user_time > mMaxRate) {
            // Uh oh, looks like we're going slower than the requested rate. If
            // this is hit consistently, you're probably doing it wrong.
            mStrand->post(mCB, mCBTag);
        }
        else {
            mTimer->wait(mMaxRate-user_time);
        }
    }
    else {
        mStrand->post( mCB, mCBTag );
    }
}

void Poller::stop() {
    mUnschedule = true;
}

void Poller::handleExec(const Network::IOTimerWPtr &wthis) {
    if (!wthis.lock())
        return;//deleted already
    static Duration null_offset = Duration::zero();
    Time start = mAccurate ? Time::now(null_offset) : Time::null();

    mUserCB();

    Time finish = mAccurate ? Time::now(null_offset) : Time::null();

    Duration user_time = finish - start;

    if (!mUnschedule)
        setupNextTimeout(user_time);
}

} // namespace Sirikata
