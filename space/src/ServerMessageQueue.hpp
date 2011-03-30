/*  Sirikata
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

#ifndef _SIRIKATA_SERVER_MESSAGE_QUEUE_HPP_
#define _SIRIKATA_SERVER_MESSAGE_QUEUE_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/SpaceContext.hpp>
#include <sirikata/space/SpaceNetwork.hpp>
#include <sirikata/space/CoordinateSegmentation.hpp>
#include "RateEstimator.hpp"

namespace Sirikata{

class ServerMessageQueue : public SpaceNetwork::SendListener, CoordinateSegmentation::Listener {
public:
    /** Implement the Sender interface to set a class up to feed messages into
     *  the ServerMessageQueue.
     */
    class Sender {
      public:
        virtual ~Sender() {}

        /** Invoked when the ServerMessageQueue is ready to accept a message
         *  from the sender, destined for the specified server. Return NULL if
         *  no elements are available.
         */
        virtual Message* serverMessagePull(ServerID dest) = 0;

        virtual bool serverMessageEmpty(ServerID dest) = 0;
    };

    ServerMessageQueue(SpaceContext* ctx, SpaceNetwork* net, Sender* sender);
    virtual ~ServerMessageQueue();

    /** Indicate that a new message is available upstream, destined for the
     *   specified server.
     */
    virtual void messageReady(ServerID sid) = 0;

    // Get the total used weight feeding into this queue. (Sum of used_weight's
    // received via updateReceiverStats()).
    double totalUsedWeight();
    // Get the capacity of this receiver in bytes per second.
    double capacity();

    // Invoked by Forwarder when it needs to update the weight for a given
    // server.  Implementations shouldn't override, instead they should
    // implement the protected handleUpdateSenderStats which will occur on
    // receiver strand.
    void updateReceiverStats(ServerID sid, double total_weight, double used_weight);
    bool isBlocked() const{
        return mBlocked;
    }
  protected:
    //actually initiate a connection, must be called from the mSenderStrand
    void connect(const ServerID&dest);
    // SpaceNetwork::SendListener Interface
    virtual void networkReadyToSend(const ServerID& from) = 0;
    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);

    // ServerMessageReceiver Protected (Implementation) Interface
    virtual void handleUpdateReceiverStats(ServerID sid, double total_weight, double used_weight) = 0;

    // Tries to send the Message to the SpaceNetwork, and tags it for analysis if
    // successful. Helper method for implementations.
    // If sent, returns the size of the serialized packet.  Otherwise, returns 0.
    uint32 trySend(const ServerID& addr, const Message* msg);
    double mCapacityOverestimate;
    SpaceContext* mContext;
    Network::IOStrand* mSenderStrand;
    SpaceNetwork* mNetwork;
    TimeProfiler::Stage* mProfiler;
    Sender* mSender;
    typedef std::tr1::unordered_map<ServerID, SpaceNetwork::SendStream*> SendStreamMap;
    SendStreamMap mSendStreams;

    // Total weights are handled by the main strand since that's the only place
    // they are needed. Handling of used weights is implementation dependent and
    // goes to the receiver strand.
    typedef std::tr1::unordered_map<ServerID, double> WeightMap;
    WeightMap mUsedWeights;
    double mUsedWeightSum;

    SimpleRateEstimator mCapacityEstimator;
    bool mBlocked; // Implementations should set this when the network gets
                   // blocked.  It will cause us to report actual measured
                   // capacity instead of an overestimate.
};
}

#endif //_SIRIKATA_SERVER_MESSAGE_QUEUE_HPP
