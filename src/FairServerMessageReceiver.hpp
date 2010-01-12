/*  cbr
 *  FairServerMessageReceiver.hpp
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

#ifndef _CBR_FAIR_SERVER_MESSAGE_RECEIVER_HPP_
#define _CBR_FAIR_SERVER_MESSAGE_RECEIVER_HPP_

#include "ServerMessageReceiver.hpp"
#include "FairQueue.hpp"
#include "NetworkQueueWrapper.hpp"

namespace CBR {

/** FairServerMessageReceiver implements the ServerMessageReceiver interface
 *  using a FairQueue (without internal storage, using the Network layer as the
 *  queue storage) to fairly distribute receive bandwidth based on the specified
 *  weights.
 */
class FairServerMessageReceiver : public ServerMessageReceiver {
public:
    FairServerMessageReceiver(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, uint32 recv_bytes_per_sec);
    virtual ~FairServerMessageReceiver();

    virtual bool receive(Message** msg_out);
    virtual void setServerWeight(ServerID sid, float weight);
private:
    // Callback from Network indicating that a new queue has become active
    virtual void networkReceivedData(const Address4& from);

    // Handles
    void handleReceived(const Address4& from);

    // Internal service call -- generated either by a networkReceivedData event
    // or by a timer as we wait for enough bandwidth to be available to service
    // the next packet.
    void service();

    uint32 mRecvRate;

    IOTimerPtr mServiceTimer; // Timer used to generate another service callback
                              // when waiting for enough bytes to service next
                              // packet
    Time mLastServiceTime; // last time we called service, to properly track
                           // rate limiting

    FairQueue<Message, ServerID, NetworkQueueWrapper > mReceiveQueues;
    uint32 mRemainderReceiveBytes;
    Time mLastReceiveEndTime; // last packet receive finish time, if there are
                              // still messages waiting

    typedef std::set<ServerID> ReceiveServerSet;
    ReceiveServerSet mReceiveSet;
    std::queue<Message*> mReceiveQueue;

};

} // namespace CBR

#endif //_CBR_FAIR_SERVER_MESSAGE_RECEIVER_HPP_
