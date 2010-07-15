/*  Sirikata
 *  SpaceNodeConnection.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LIBOH_SPACE_NODE_CONNECTION_HPP_
#define _SIRIKATA_LIBOH_SPACE_NODE_CONNECTION_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/service/TimeProfiler.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>

#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceID.hpp>

#include "QueueRouterElement.hpp"

namespace Sirikata {

// Connections to servers
struct SIRIKATA_OH_EXPORT SpaceNodeConnection {
  public:
    typedef std::tr1::function<void(SpaceNodeConnection*)> GotSpaceConnectionCallback;
    typedef std::tr1::function<void(SpaceNodeConnection*)> ReceiveCallback;

    typedef std::tr1::function<void(const Network::Stream::ConnectionStatus, const std::string&)> ConnectionEventCallback;

    SpaceNodeConnection(ObjectHostContext* ctx, Network::IOStrand* ioStrand, TimeProfiler::Stage* handle_read_stage, OptionSet *streamOptions, const SpaceID& spaceid, ServerID sid, const Network::Address& addr, ConnectionEventCallback ccb, ReceiveCallback rcb);
    ~SpaceNodeConnection();

    // Push a packet to be sent out
    bool push(const ObjectMessage& msg);

    // Pull a packet from the receive queue
    ObjectMessage* pull();

    bool empty();
    void shutdown();

    const SpaceID& space() const { return mSpace; }
    const ServerID& server() const { return mServer; }

    void connect();

    bool connecting() const { return mConnecting; }

    void addCallback(GotSpaceConnectionCallback cb) { mConnectCallbacks.push_back(cb); }
    void invokeAndClearCallbacks(bool connected);
  private:
    // Thread Safe
    ObjectHostContext* mContext;
    TimeProfiler::Stage* mHandleReadStage;
    SpaceID mSpace;
    ServerID mServer;
    Network::Stream* socket;
    Network::Address mAddr;

    // Callback for connection event
    void handleConnectionEvent(const Network::Stream::ConnectionStatus status, const std::string&reason);

    // Callback for when the connection receives data
    void handleRead(Sirikata::Network::Chunk& chunk, const Sirikata::Network::Stream::PauseReceiveCallback& pause);

    // Main Strand
    typedef std::vector<GotSpaceConnectionCallback> ConnectionCallbackList;
    ConnectionCallbackList mConnectCallbacks;
    bool mConnecting;

    // IO Strand
    QueueRouterElement<ObjectMessage> receive_queue;

    ConnectionEventCallback mConnectCB;
    ReceiveCallback mReceiveCB;
};

} // namespace Sirikata


#endif //_SIRIKATA_LIBOH_SPACE_NODE_CONNECTION_HPP_
