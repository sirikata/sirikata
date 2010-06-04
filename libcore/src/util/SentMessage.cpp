/*  Sirikata Network Utilities
 *  SentMessage.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
/*  Created on: Jul 18, 2009 */

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/SentMessage.hpp>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOTimer.hpp>

namespace Sirikata {
void  checkThreadId(void *mDebugThreadId){
    boost::thread::id*prev_thread_id=(boost::thread::id*)mDebugThreadId;
    assert(*prev_thread_id==boost::this_thread::get_id());
}
void SentMessage::timedOut() {
    RoutableMessageHeader msg;
    checkThreadId(mDebugThreadId);
    if (this->header().has_destination_object()) {
        msg.set_source_object(this->header().destination_object());
    }
    if (this->header().has_destination_space()) {
        msg.set_source_space(this->header().destination_space());
    }
    msg.set_source_port(this->header().destination_port());
    msg.set_return_status(RoutableMessageHeader::TIMEOUT_FAILURE);
    msg.set_reply_id(this->getId());
    this->mTimerHandle.reset();

    // Make sure this is on the same thread--
    // We can use mQueryTracker->sendMessage(msg) to send it via the same loop
    this->mResponseCallback(this, msg, MemoryReference(NULL,0));
}

void SentMessage::processMessage(const RoutableMessageHeader &header, MemoryReference body) {
    unsetTimeout();
    checkThreadId(mDebugThreadId);
    mResponseCallback(this, header, body);
}

SentMessage::SentMessage(int64 newId, QueryTracker *tracker)
    : mId(newId), mTracker(tracker), mDebugThreadId(NULL)
{
    header().set_id(mId);
    tracker->insert(this);
    assert (mDebugThreadId=new boost::thread::id(boost::this_thread::get_id()));
}

SentMessage::SentMessage(int64 newId, QueryTracker *tracker, const QueryCallback& cb)
    : mId(newId), mResponseCallback(cb), mTracker(tracker), mDebugThreadId(NULL)
{
    header().set_id(mId);
    assert (mDebugThreadId=new boost::thread::id(boost::this_thread::get_id()));
    tracker->insert(this);
}

SentMessage::SentMessage(QueryTracker *tracker, const QueryCallback& cb)
    : mId(tracker->allocateId()), mResponseCallback(cb), mTracker(tracker),mDebugThreadId(NULL)
{
    header().set_id(mId);
    assert (mDebugThreadId=new boost::thread::id(boost::this_thread::get_id()));
    tracker->insert(this);
}

SentMessage::~SentMessage() {
    unsetTimeout();
    mTracker->remove(this);
    checkThreadId(mDebugThreadId);
    if (mDebugThreadId) {
        delete (boost::thread::id*)mDebugThreadId;
    }
}

static SpaceID nul = SpaceID::null();
const SpaceID &SentMessage::getSpace() const {
    if (header().has_destination_space()) {
        return header().destination_space();
    } else {
        return nul;
    }
}


void SentMessage::send(MemoryReference bodystr) {
    mHeader.set_id(mId);
    mTracker->sendMessage(header(), bodystr);
    checkThreadId(mDebugThreadId);
}

void SentMessage::unsetTimeout() {
    if (mTimerHandle) {
        mTimerHandle->cancel();
        Network::IOTimerWPtr tmp(mTimerHandle);
        mTimerHandle.reset();
        assert("Unsetting timeout should have destroyed the timeout"&&!tmp.lock());
    }
}

void SentMessage::setTimeout(const Duration& timeout) {
    unsetTimeout();
    if (mTracker) {
        checkThreadId(mDebugThreadId);
        Network::IOService *io = mTracker->getIOService();
        if (io) {
            Network::IOTimerWPtr tmp(mTimerHandle);
            mTimerHandle = Network::IOTimer::create(io);
            assert("Unsetting timeout should have destroyed the timeout"&&!tmp.lock());

            mTimerHandle->setCallback(std::tr1::bind(&SentMessage::timedOut, this));
            mTimerHandle->wait(timeout);
        }
    }
}

}
