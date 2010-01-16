/*  cbr
 *  ServerMessageReceiver.hpp
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

#ifndef _CBR_SERVER_MESSAGE_RECEIVER_HPP_
#define _CBR_SERVER_MESSAGE_RECEIVER_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "TimeProfiler.hpp"
#include "Network.hpp"
#include "CoordinateSegmentation.hpp"

namespace CBR{

class SpaceContext;
class ServerIDMap;
class Message;

/** ServerMessageReceiver handles receiving ServerMessages from the Network.  It
 *  pulls messages from the network as its rate limiting permits and handles
 *  distributing available bandwidth fairly between connections.  As messages
 *  are received from the network it pushes them up to a listener which can
 *  handle them -- no queueing is performed internally.
 */
class ServerMessageReceiver : public Network::ReceiveListener, CoordinateSegmentation::Listener {
public:
    class Listener {
      public:
        virtual ~Listener() {}

        virtual void serverMessageReceived(Message* msg) = 0;
    };

    ServerMessageReceiver(SpaceContext* ctx, Network* net, ServerIDMap* sidmap, Listener* listener);
    virtual ~ServerMessageReceiver();

protected:
    virtual void setServerWeight(ServerID sid, float weight) = 0;

    // Network::ReceiveListener Interface
    virtual void networkReceivedConnection(const Address4& from) = 0;
    virtual void networkReceivedData(const Address4& from) = 0;
    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_segmentation);

    SpaceContext* mContext;
    Network* mNetwork;
    ServerIDMap* mServerIDMap;
    TimeProfiler::Stage* mProfiler;
    Listener* mListener;
};

} // namespace CBR

#endif //_CBR_SERVER_MESSAGE_RECEIVER_HPP_
