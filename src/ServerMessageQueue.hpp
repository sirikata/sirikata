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

namespace CBR{

class ServerIDMap;

class ServerMessageQueue : public Network::SendListener, CoordinateSegmentation::Listener {
public:
    class Listener {
      public:
        virtual ~Listener() {}

        /** Invoked when a server Message is successfully pushed to the
         *  network. The pointer will only be valid for the duration of the call
         *  -- it should not be saved and used later by the recipient.
         */
        virtual void serverMessageSent(Message* msg) = 0;
    };

    ServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, Listener* listener);
    virtual ~ServerMessageQueue();

    /** Try to add the given message to this queue.  Note that only messages
     *  destined for other servers should be enqueued.  This class will not
     *  route messages to the ServerMessageReceiver.
     *  \param msg the message to try to push onto the queue.
     *  \returns true if the message was added, false otherwise
     *  \note If successful, the queue takes possession of the message and ensures it is disposed of.
     *        If unsuccessful, the message is still owned by the caller.
     */
    virtual bool addMessage(Message* msg)=0;
    /** Check if a message could be pushed on the queue.  If this returns true,
     *  an immediate subsequent call to addMessage() will always be
     *  successful. The message should not be destined for this server.
     *  \param msg the message to try to push onto the queue.
     *  \returns true if the message was added, false otherwise
     */
    virtual bool canAddMessage(const Message* msg)=0;

    virtual void service() = 0;

    /** Add an input queue using the specified weight. */
    virtual void addInputQueue(ServerID sid, float weight) = 0;
    /** Update the weight on an input queue. */
    virtual void updateInputQueueWeight(ServerID sid, float weight) = 0;
    /** Remove the specified input queue. */
    virtual void removeInputQueue(ServerID sid) = 0;
  protected:
    // Network::SendListener Interface
    virtual void networkReadyToSend(const Address4& from) = 0;
    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);

    // Tries to send the Message to the Network, and tags it for analysis if
    // successful. Helper method for implementations.
    bool trySend(const Address4& addr, const Message* msg);

    SpaceContext* mContext;
    Network* mNetwork;
    ServerIDMap* mServerIDMap;
    TimeProfiler::Stage* mProfiler;
    Listener* mListener;
};
}

#endif //_CBR_SERVER_MESSAGE_QUEUE_HPP
