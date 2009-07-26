/*  Sirikata Subscription and Broadcasting System -- Subscription Services
 *  Server.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _SIRIKATA_SUBSCRIPTION_SERVER_HPP_
#define _SIRIKATA_SUBSCRIPTION_SERVER_HPP_
#include <network/Stream.hpp>
#include <network/StreamListener.hpp>
#include <util/UUID.hpp>
namespace Sirikata { namespace Subscription {

namespace Protocol {
class Subscribe;
}
class SubscriptionState;

class SIRIKATA_SUBSCRIPTION_EXPORT Server:protected std::tr1::enable_shared_from_this<Server> {
    class UniqueLock;
    std::tr1::unordered_map<UUID,SubscriptionState*,UUID::Hasher>mSubscriptions;
    std::tr1::unordered_map<SubscriptionState*,UUID>mBroadcasters;
    UniqueLock*mBroadcastersSubscriptionsLock;
    Network::StreamListener*mBroadcastListener;
    Network::IOService*mBroadcastIOService;
    Network::StreamListener*mSubscriberListener;
    void subscriberStreamCallback(Network::Stream*,Network::Stream::SetCallbacks&);
    void subscriberBytesReceivedCallback(const std::tr1::shared_ptr<std::tr1::shared_ptr<Network::Stream> >&,const Network::Chunk&);
    void subscriberBytesReceivedCallbackOnBroadcastIOService(const std::tr1::shared_ptr<std::tr1::shared_ptr<Network::Stream> >&stream,const Protocol::Subscribe&subscriptionRequest);
    static void subscriberConnectionCallback(const std::tr1::shared_ptr<std::tr1::shared_ptr<Network::Stream> >&,Network::Stream::ConnectionStatus,const std::string&reason);
    void broadcastConnectionCallback(SubscriptionState*,Network::Stream::ConnectionStatus,const std::string&reason);
    void broadcastStreamCallback(Network::Stream*,Network::Stream::SetCallbacks&);
    void broadcastBytesReceivedCallback(SubscriptionState*, const Network::Chunk&);
    static void poll(const std::tr1::weak_ptr<Server> &, const UUID&);
public:

    Server(Network::IOService*broadcastIOSerivce, Network::StreamListener*broadcastListener, const Network::Address& broadcastAddress, Network::StreamListener*subscriberListener, const Network::Address&subscriberAddress);
    ~Server();
    void initiatePolling(const UUID&, const Duration&waitFor);
};


} }
#endif
