/*  Sirikata liboh -- Object Host
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

#include <util/Platform.hpp>
#include <oh/Platform.hpp>
#include <ObjectHost_Sirikata.pbj.hpp>
#include <oh/ProxyObject.hpp>
#include <oh/ProxyManager.hpp>
#include "util/RoutableMessage.hpp"
#include "oh/SentMessage.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/error.hpp>

#include <network/TCPDefinitions.hpp> // For "class IOService" definition...


namespace Sirikata {

class SentMessage::TimerHandler {
    boost::asio::deadline_timer mTimer;
    SentMessage *mSentMessage;
    int64 mMessageId;

    void timedOut(const boost::system::error_code &error) {
        std::auto_ptr<TimerHandler> scopedDestructor(this);
        if (error == boost::asio::error::operation_aborted) {
            return; // don't care if the timer was cancelled.
        }
        RoutableMessage msg;
        msg.set_source_object(mSentMessage->getRecipient());
        msg.set_source_space(mSentMessage->getSpace());
        msg.set_source_port(mSentMessage->getPort());
        msg.body().set_return_status(RoutableMessageBody::TIMEOUT_FAILURE);
        msg.body().set_id(mSentMessage->getId());
        mSentMessage->gotResponse(msg);
    }
public:
    TimerHandler(Network::IOService *io, SentMessage *messageInfo, int num_seconds)
            : mTimer(*static_cast<boost::asio::io_service*>(io), boost::posix_time::seconds(num_seconds)) {

        mSentMessage = messageInfo;
        mTimer.async_wait(
            boost::bind(&TimerHandler::timedOut, this, boost::asio::placeholders::error));
    }

    void cancel() {
        mMessageId = -1;
        mTimer.cancel();
    }
};

bool SentMessage::gotResponse(const RoutableMessage &msg) {
    unsetTimeout();
    bool keepQuery = mResponseCallback(this, msg);
    return keepQuery;
}

SentMessage::SentMessage(int64 newId, QueryTracker *tracker)
    : mTimerHandle(NULL), mId(newId), mTracker(tracker)
{
    mBody = new RoutableMessageBody;
    body().set_id(mId);
}

SentMessage::~SentMessage() {
    unsetTimeout();
    delete mBody;
}

void SentMessage::send() {
    if (mTracker) {
        mTracker->sendMessage(this);
    }
}

void SentMessage::unsetTimeout() {
    if (mTimerHandle) {
        mTimerHandle->cancel();
        mTimerHandle = 0;
    }
}

void SentMessage::setTimeout(int timeout) {
    unsetTimeout();
    if (mTracker) {
        Network::IOService *io = mTracker->getIOService();
        if (io) {
            mTimerHandle = new TimerHandler(io, this, timeout);
        }
    }
}

ProxyObjectPtr SentMessage::getRecipientProxy(const ProxyManager*pm) const {
    if (pm) {
        ProxyObjectPtr proxyPtr (pm->getProxyObject(SpaceObjectReference(getSpace(), getRecipient())));
        return proxyPtr;
    }
    return ProxyObjectPtr();
}


}
