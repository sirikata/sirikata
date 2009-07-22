/*  Sirikata liboh -- Object Host
 *  HostedObject_Query.cpp
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
#include "util/RoutableMessage.hpp"
#include "oh/HostedObject.hpp"
#include "oh/ObjectHost.hpp"

#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/error.hpp>

#include <network/TCPDefinitions.hpp> // For "class IOService" definition...


namespace Sirikata {
class HostedObject::SentMessage::TimerHandler {
    boost::asio::deadline_timer mTimer;
    HostedObjectWPtr mHostedObject;
    int64 mMessageId;

    void timedOut(const boost::system::error_code &error) {
        std::auto_ptr<TimerHandler> scopedDestructor(this);
        if (error == boost::asio::error::operation_aborted) {
            return; // don't care if the timer was cancelled.
        }
        HostedObjectPtr obj = mHostedObject.lock();
        if (!obj) {
            return;
        }
        SentMessageMap::iterator iter = obj->mSentMessages.find(mMessageId);
        if (iter == obj->mSentMessages.end()) {
            return;
        }
        SentMessage *messageInfo = iter->second;
        RoutableMessage msg;
        msg.set_source_object(messageInfo->getRecipient());
        msg.set_source_space(messageInfo->getSpace());
        msg.set_source_port(messageInfo->getPort());
        msg.body().set_return_status(RoutableMessageBody::TIMEOUT_FAILURE);
        msg.body().set_id(messageInfo->getId());
        messageInfo->gotResponse(obj, iter, msg);
    }
public:
    TimerHandler(const HostedObjectPtr &object, SentMessage *messageInfo, int num_seconds)
            : mTimer(*object->getObjectHost()->getSpaceIO(),
                     boost::posix_time::seconds(num_seconds)) {
        mHostedObject = object;

        if (num_seconds > 0) {
            mTimer.async_wait(
                boost::bind(&TimerHandler::timedOut, this, boost::asio::placeholders::error));
        }
    }

    void cancel() {
        mMessageId = -1;
        mHostedObject = HostedObjectWPtr();
        mTimer.cancel();
    }
};

void HostedObject::SentMessage::gotResponse(const HostedObjectPtr &hostedObj, const SentMessageMap::iterator &iter, const RoutableMessage &msg) {
    if (mTimerHandle) {
        mTimerHandle->cancel();
        mTimerHandle = NULL;
    }

    bool keepQuery = mResponseCallback(hostedObj, this, msg);

    if (!keepQuery) {
        hostedObj->mSentMessages.erase(iter);
        delete this;
    }
}

HostedObject::SentMessage::SentMessage(const HostedObjectPtr &parent)
        : mTimerHandle(NULL)
{
    mBody = new RoutableMessageBody;
    mParent = parent;
    mId = parent->mNextQueryId++;
    body().set_id(mId);
    parent->mSentMessages.insert(SentMessageMap::value_type(mId, this));
}

HostedObject::SentMessage::~SentMessage() {
    delete mBody;
}


void HostedObject::SentMessage::send(const HostedObjectPtr &parent, int timeout) {
    if (timeout > 0) {
        mTimerHandle = new TimerHandler(parent, this, timeout);
    }
    std::string bodyStr;
    body().SerializeToString(&bodyStr);
    parent->send(header(), MemoryReference(bodyStr));
}

}
