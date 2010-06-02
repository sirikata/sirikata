/*  Sirikata
 *  QueryTracker.hpp
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

#ifndef _SIRIKATA_QueryTracker_HPP_
#define _SIRIKATA_QueryTracker_HPP_

#include <core/odp/Port.hpp>

namespace Sirikata {

class SentMessage;
namespace Network {
class IOService;
}

class SIRIKATA_EXPORT QueryTracker : public MessageService {
public:
    /// Each sent query has an int64 id. This maps that id to a SentMessage structure.
    typedef std::tr1::unordered_map<int64, SentMessage*> SentMessageMap;

private:
    SentMessageMap mSentMessages;
    int64 mNextQueryId;
    Network::IOService *mIOService;
    MessageService *mForwardService;
    ODP::Port* mPort;
public:
    /** Constructs a QueryTracker.
     *  \param port the ODP::Port to listen to and respond on.  Ownership of the
     *              port is transferred to the QueryTracker.
     *  @param timerService  An IOService instance for creating timeouts. If
     *                       NULL, timeouts will not be honored.
     *  @param forwarder     A message service to forward messages to.
     */
    explicit QueryTracker(ODP::Port* port, Network::IOService *timerService, MessageService *forwarder=NULL) {
        mNextQueryId = 0;
        mIOService = timerService;
        mForwardService = forwarder;
        mPort = port;
    }
    /// Destructor: Cancels all ongoing queries with a NETWORK_FAILURE status.
    ~QueryTracker();

    /// Returns the next allocated ID. Used in the SentMessage(QueryTracker*) constrcutor.
    int64 allocateId() {
        return mNextQueryId++;
    }

    /// Adds a new SentMessage to the map.
    void insert(SentMessage *msg);

    /// Removes the SentMessage from this map.
    bool remove(SentMessage *msg);

    /// MessageService interface: sent messages will go through serv.
    bool forwardMessagesTo(MessageService*serv) {
        mForwardService = serv;
        return true;
    }
    /// MessageService interface: no messages are allowed to be sent after this call.
    bool endForwardingMessagesTo(MessageService*serv) {
        bool ret = (mForwardService == serv);
        mForwardService = NULL;
        return ret;
    }

    /// A response has been received from the other object.
    void processMessage(const RoutableMessageHeader&, MemoryReference message_body);

    /// Sends a message through the MessageService passed to forwardMessagesTo()
    void sendMessage(const RoutableMessageHeader &hdr, MemoryReference body);

    /// Gets the timer IOService, if any.
    Network::IOService *getIOService() {
        return mIOService;
    }
};

}

#endif
