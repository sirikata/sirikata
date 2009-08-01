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
#include "util/RoutableMessageHeader.hpp"
#include "QueryTracker.hpp"
#include "SentMessage.hpp"

namespace Sirikata {

QueryTracker::~QueryTracker() {
    mForwardService = NULL; // can't resend a message.
    SentMessageMap sentMessageCopy;
    mSentMessages.swap(sentMessageCopy);
	// FIXME: Need to have an "error" callback of some sort, but ideally before getting to the destructor
	/*
    for (SentMessageMap::iterator iter = sentMessageCopy.begin(); iter != sentMessageCopy.end(); ++iter) {
        SentMessage *messageInfo = iter->second;
        RoutableMessageHeader msg;
        msg.set_source_object(messageInfo->getRecipient());
        msg.set_source_space(messageInfo->getSpace());
        msg.set_source_port(messageInfo->getPort());
        msg.set_return_status(RoutableMessageHeader::NETWORK_FAILURE);
        msg.set_reply_id(messageInfo->getId());
        messageInfo->processMessage(msg, MemoryReference(NULL,0));
    }
	*/
}

void QueryTracker::insert(SentMessage *ret) {
    mSentMessages.insert(SentMessageMap::value_type(ret->getId(), ret));
}

bool QueryTracker::remove(SentMessage *ret) {
    int64 id = ret->getId();
    SentMessageMap::iterator iter = mSentMessages.find(id);
    if (iter != mSentMessages.end()) {
        mSentMessages.erase(iter);
        return true;
    }
    return false;
}

void QueryTracker::sendMessage(const RoutableMessageHeader &msgHeader, MemoryReference bodyStr) {
    if (mForwardService) {
        mForwardService->processMessage(msgHeader, bodyStr);
    }
}

void QueryTracker::processMessage(const RoutableMessageHeader &msgHeader, MemoryReference body) {
    if (!msgHeader.has_reply_id()) {
        SILOG(cppoh, error, "QueryTracker::processMessage called for non-reply");
        return; // Not a response message--shouldn't have gotten here.
    }
    // This is a response message;
    int64 id = msgHeader.reply_id();
    SentMessageMap::iterator iter = mSentMessages.find(id);
    if (iter != mSentMessages.end()) {
        if (iter->second->getSpace() == msgHeader.source_space() &&
            iter->second->getRecipient() == msgHeader.source_object())
        {
            iter->second->processMessage(msgHeader, body);
        } else {
            ObjectReference dest(ObjectReference::null());
            if (msgHeader.has_destination_object()) {
                dest = msgHeader.destination_object();
            }
            std::ostringstream os;
            os << "Response message with ID "<<id<<" to object "<<dest<<
                " should come from "<<iter->second->getRecipient() <<
                " but instead came from " <<msgHeader.source_object();
            SILOG(cppoh, warning, os.str());
        }
    } else {
        SILOG(cppoh, warning, "Got a reply for unknown query ID "<<id);
    }
}

}
