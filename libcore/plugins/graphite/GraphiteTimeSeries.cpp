/*  Sirikata
 *  GraphiteTimeSeries.cpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#include "GraphiteTimeSeries.hpp"
#include <sirikata/core/service/Context.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#define GRAPHITE_LOG(level, msg) SILOG(graphite, level, msg)

namespace Sirikata {
namespace Trace {

GraphiteTimeSeries::GraphiteTimeSeries(Context* ctx, const String& host, uint16 port)
 : TimeSeries(ctx),
   mHost(host),
   mPort(port),
   mResolver(NULL),
   mSocket(NULL),
   mConnecting(false),
   mTransmitting(false)
{
}

GraphiteTimeSeries::~GraphiteTimeSeries() {
    cleanup();
}

void GraphiteTimeSeries::connect() {
    using namespace boost::asio;

    cleanup();

    mConnecting = true;
    mResolver = new Network::TCPResolver(mContext->ioService);
    Network::TCPResolver::query query(mHost, boost::lexical_cast<String>(mPort));
    mResolver->async_resolve(
        query,
        boost::bind(&GraphiteTimeSeries::handleResolve, this, placeholders::error, placeholders::iterator)
    );
}


void GraphiteTimeSeries::handleResolve(const boost::system::error_code& err, Network::TCPResolver::iterator endpoint_iterator) {
    if (err) {
        GRAPHITE_LOG(error, "Failed to resolve hostname " << mHost);
        cleanup();
        return;
    }
    mSocket = new Network::TCPSocket(mContext->ioService);
    tryConnect(endpoint_iterator);
}

void GraphiteTimeSeries::tryConnect(Network::TCPResolver::iterator endpoint_iterator) {
    using namespace boost::asio;

    boost::asio::ip::tcp::endpoint ep = *endpoint_iterator;
    mSocket->async_connect(ep,
        boost::bind(&GraphiteTimeSeries::handleConnect, this, placeholders::error, ++endpoint_iterator)
    );
}

void GraphiteTimeSeries::handleConnect(const boost::system::error_code& err, Network::TCPResolver::iterator endpoint_iterator) {
    // Success
    if (!err) {
        mConnecting = false;
        return;
    }

    // Try next endpoint
    if (endpoint_iterator != Network::TCPResolver::iterator()) {
        mSocket->close();
        tryConnect(endpoint_iterator);
        return;
    }

    // Or just stop trying because we failed to connect
    GRAPHITE_LOG(error, "Failed to connect to " << mHost);
    mConnecting = false;
    cleanup();
}

void GraphiteTimeSeries::report(const String& name, float64 val) {
    // If we're connecting, ignore this update
    if (mConnecting) return;

    // We're just fully disconnected, trigger a connection request and
    // drop this update
    if (!mConnecting && (mSocket == NULL || !mSocket->is_open())) {
        connect();
        return;
    }

    // If we have too many outstanding updates, drop it
    if (mUpdates.size() > 50) return;

    // Otherwise, we should be fine to transmit
    static Time unix_epoch = Timer::getSpecifiedDate(String("1970-01-01 00:00:00.000"));
    String data =
        name + " " +
        boost::lexical_cast<String>(val) + " " +
        boost::lexical_cast<String>((int64)(mContext->recentRealTime() - unix_epoch).seconds()) + "\n";
    mUpdates.push(data);
    if (!mTransmitting && !mUpdates.empty()) startSend();
}

void GraphiteTimeSeries::startSend() {
    using namespace boost::asio;

    assert(!mConnecting && mSocket && mSocket->is_open() && !mUpdates.empty());

    mCurrentUpdate = mUpdates.front();
    mUpdates.pop();

    boost::asio::async_write(
        *mSocket,
        boost::asio::buffer(&mCurrentUpdate[0], mCurrentUpdate.size()),
        boost::bind(&GraphiteTimeSeries::handleSent, this, placeholders::error )
    );
}

void GraphiteTimeSeries::handleSent(const boost::system::error_code& err) {
    if (err) {
        GRAPHITE_LOG(error, "Error while sending update, resetting.");
        cleanup();
    }

    // If we're out of updates, mark as ready to transmit
    if (mUpdates.empty()) {
        mTransmitting = false;
        return;
    }

    // Otherwise, process next message
    startSend();
}

void GraphiteTimeSeries::cleanup() {
    if (mResolver) {
        delete mResolver;
        mResolver = NULL;
    }
    if (mSocket) {
        delete mSocket;
        mSocket = NULL;
    }
    mConnecting = false;
    mTransmitting = false;
}

} // namespace Trace
} // namespace Sirikata
