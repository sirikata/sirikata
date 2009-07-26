/*  Sirikata liboh -- Object Host
 *  QueryTracker.cpp
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
/*  Created on: Jul 25, 2009 */

#include <util/Platform.hpp>
#include <oh/Platform.hpp>
#include <ObjectHost_Sirikata.pbj.hpp>
#include "util/RoutableMessage.hpp"
#include "oh/QueryTracker.hpp"
#include "oh/SentMessage.hpp"

namespace Sirikata {

QueryTracker::~QueryTracker() {
    mForwardService = NULL; // can't resend a message.
    SentMessageMap sentMessageCopy;
    mSentMessages.swap(sentMessageCopy);
    for (SentMessageMap::iterator iter = sentMessageCopy.begin(); iter != sentMessageCopy.end(); ++iter) {
        SentMessage *messageInfo = iter->second;
        RoutableMessage msg;
        msg.set_source_object(messageInfo->getRecipient());
        msg.set_source_space(messageInfo->getSpace());
        msg.set_source_port(messageInfo->getPort());
        msg.body().set_return_status(RoutableMessageBody::NETWORK_FAILURE);
        msg.body().set_id(messageInfo->getId());
        messageInfo->gotResponse(msg);
    }
}

SentMessage *QueryTracker::create() {
    SentMessage *ret = new SentMessage(mNextQueryId++, this);
    mSentMessages.insert(SentMessageMap::value_type(ret->getId(), ret));
    return ret;
}

void QueryTracker::sendMessage(SentMessage *msg) {
    std::string bodyStr;
    msg->body().SerializeToString(&bodyStr);
    if (mForwardService) {
        mForwardService->processMessage(msg->header(), MemoryReference(bodyStr));
    }
}

void QueryTracker::processMessage(const RoutableMessageHeader &msgHeader, MemoryReference body) {
    RoutableMessage msg(msgHeader);
    msg.body().ParseFromArray(body.data(), body.size());
    processMessage(msg);
}

void QueryTracker::processMessage(const RoutableMessage &message) {
    if (!message.body().has_return_status()) {
        return; // Not a response message--shouldn't have gotten here.
    }
    if (!message.body().has_id()) {
        return; // invalid!
    }
    // This is a response message;
    int64 id = message.body().id();
    SentMessageMap::iterator iter = mSentMessages.find(id);
    if (iter != mSentMessages.end()) {
        if (iter->second->getSpace() == message.source_space() &&
            iter->second->getRecipient() == message.source_object())
        {
            bool keepQuery = iter->second->gotResponse(message);
            if (!keepQuery) {
                delete iter->second;
                mSentMessages.erase(iter);
            }
        } else {
            ObjectReference dest(ObjectReference::null());
            if (message.has_destination_object()) {
                dest = message.destination_object();
            }
            std::ostringstream os;
            os << "Response message with ID "<<id<<" to object "<<dest<<" should come from "<<iter->second->getRecipient() <<" but instead came from " <<message.source_object();
            SILOG(cppoh, warning, os.str());
        }
    }
}

}
