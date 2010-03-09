/*  cbr
 *  ServerMessageQueue.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_SERVER_MESSAGE_QUEUE_HPP_
#define _CBR_SERVER_MESSAGE_QUEUE_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"
#include "CoordinateSegmentation.hpp"
#include "ServerWeightCalculator.hpp"

namespace CBR{

class ServerMessageQueue : public Network::SendListener, CoordinateSegmentation::Listener {
public:
    /** Implement the Sender interface to set a class up to feed messages into
     *  the ServerMessageQueue.
     */
    class Sender {
      public:
        virtual ~Sender() {}

        /** Invoked to check what the item returned would be if
         *  serverMessagePull were called.
         */
        virtual Message* serverMessageFront(ServerID dest) = 0;
        /** Invoked when the ServerMessageQueue is ready to accept a message
         *  from the sender, destined for the specified server. Return NULL if
         *  no elements are available.
         */
        virtual Message* serverMessagePull(ServerID dest) = 0;

        virtual bool serverMessageEmpty(ServerID dest) = 0;
    };

    ServerMessageQueue(SpaceContext* ctx, Network* net, Sender* sender, ServerWeightCalculator* swc);
    virtual ~ServerMessageQueue();

    /** Add an input queue using the specified weight. */
    virtual void addInputQueue(ServerID sid, float weight) = 0;
    /** Update the weight on an input queue. */
    virtual void updateInputQueueWeight(ServerID sid, float weight) = 0;
    /** Remove the specified input queue. */
    virtual void removeInputQueue(ServerID sid) = 0;

    /** Indicate that a new message is available upstream, destined for the
     *   specified server.
     */
    virtual void messageReady(ServerID sid) = 0;
  protected:
    // Network::SendListener Interface
    virtual void networkReadyToSend(const ServerID& from) = 0;
    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);

    // Tries to send the Message to the Network, and tags it for analysis if
    // successful. Helper method for implementations.
    bool trySend(const ServerID& addr, const Message* msg);

    SpaceContext* mContext;
    Network* mNetwork;
    TimeProfiler::Stage* mProfiler;
    Sender* mSender;
    ServerWeightCalculator* mServerWeightCalculator;
};
}

#endif //_CBR_SERVER_MESSAGE_QUEUE_HPP
