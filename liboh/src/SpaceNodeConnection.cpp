/*  Sirikata
 *  SpaceNodeConnection.cpp
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

#include <sirikata/oh/SpaceNodeConnection.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

using namespace Sirikata;
using namespace Sirikata::Network;

namespace Sirikata {

SpaceNodeConnection::SpaceNodeConnection(ObjectHostContext* ctx, Network::IOStrand* ioStrand, TimeProfiler::Stage* handle_read_stage, OptionSet *streamOptions, const SpaceID& spaceid, ServerID sid, const Network::Address& addr, ConnectionEventCallback ccb, ReceiveCallback rcb)
 : mContext(ctx),
   mHandleReadStage(handle_read_stage),
   mSpace(spaceid),
   mServer(sid),
   socket(Sirikata::Network::StreamFactory::getSingleton().getConstructor(GetOptionValue<String>("ohstreamlib"))(ioStrand,streamOptions)),
   mAddr(addr),
   mConnecting(false),
   receive_queue(GetOptionValue<int32>("object-host-receive-buffer"), std::tr1::bind(&ObjectMessage::size, std::tr1::placeholders::_1)),
   mConnectCB(ccb),
   mReceiveCB(rcb)
{
    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(GetOptionValue<String>("ohstreamlib")),0);
}

SpaceNodeConnection::~SpaceNodeConnection() {
    delete socket;
}

bool SpaceNodeConnection::push(const ObjectMessage& msg) {
    TIMESTAMP_START(tstamp, (&msg));

    std::string data;
    msg.serialize(&data);

    // Try to push to the network
    bool success = socket->send(
        //Sirikata::MemoryReference(&(data[0]), data.size()),
        Sirikata::MemoryReference(data),
        Sirikata::Network::ReliableOrdered
                                );
    if (success) {
        TIMESTAMP_END(tstamp, Trace::OH_HIT_NETWORK);
    }
    else {
        TIMESTAMP_END(tstamp, Trace::OH_DROPPED_AT_SEND);
        TRACE_DROP(OH_DROPPED_AT_SEND);
    }

    return success;
}

ObjectMessage* SpaceNodeConnection::pull() {
    return receive_queue.pull();
}

bool SpaceNodeConnection::empty() {
    return receive_queue.empty();
}

void SpaceNodeConnection::handleRead(Chunk& chunk, const Sirikata::Network::Stream::PauseReceiveCallback& pause) {
    mHandleReadStage->started();

    // Parse
    ObjectMessage* msg = new ObjectMessage();
    bool parse_success = msg->ParseFromArray(&(*chunk.begin()), chunk.size());
    if (!parse_success) {
        LOG_INVALID_MESSAGE(session, error, chunk);
        delete msg;
        return;
    }

    TIMESTAMP_START(tstamp, msg);

    // Mark as received
    TIMESTAMP_END(tstamp, Trace::OH_NET_RECEIVED);

    // NOTE: We can't record drops here or we incur a lot of overhead in parsing...
    bool pushed = receive_queue.push(msg);

    if (pushed) {
        // handleConnectionRead() could be called from any thread/strand. Everything that is not
        // thread safe that could result from a new message needs to happen in the main strand,
        // so just post the whole handler there.
        if (receive_queue.wentNonEmpty())
            mReceiveCB(this);
    }
    else {
        TIMESTAMP_END(tstamp, Trace::OH_DROPPED_AT_RECEIVE_QUEUE);
        TRACE_DROP(OH_DROPPED_AT_RECEIVE_QUEUE);
        delete msg;
    }

    mHandleReadStage->finished();

    // No matter what, we've "handled" the data, either for real or by dropping.
}

void SpaceNodeConnection::connect() {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    mConnecting = true;

    socket->connect(mAddr,
        &Sirikata::Network::Stream::ignoreSubstreamCallback,
        mContext->mainStrand->wrap( std::tr1::bind(&SpaceNodeConnection::handleConnectionEvent, this, _1, _2) ),
        std::tr1::bind(&SpaceNodeConnection::handleRead, this, _1, _2),
        &Sirikata::Network::Stream::ignoreReadySendCallback
    );
}

void SpaceNodeConnection::handleConnectionEvent(
    const Network::Stream::ConnectionStatus status,
    const std::string&reason)
{
    mConnecting = false;

    invokeAndClearCallbacks( status == Network::Stream::Connected );
    mConnectCB(status, reason);
}

void SpaceNodeConnection::invokeAndClearCallbacks(bool connected) {
    SpaceNodeConnection* param = (connected ? this : NULL);
    for(ConnectionCallbackList::iterator it = mConnectCallbacks.begin(); it != mConnectCallbacks.end(); it++)
        (*it)(param);
    mConnectCallbacks.clear();
}

void SpaceNodeConnection::shutdown() {
    socket->close();
}

}
