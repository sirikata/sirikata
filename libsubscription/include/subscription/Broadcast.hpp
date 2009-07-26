/*  Sirikata Subscription and Broadcasting System -- Broadcast Services
 *  Broadcast.hpp
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

#ifndef _SIRIKATA_BROADCAST_HPP_
#define _SIRIKATA_BROADCAST_HPP_

#include <network/Stream.hpp>
#include <network/StreamListener.hpp>
#include <util/UUID.hpp>

namespace Sirikata { namespace Subscription {

class SIRIKATA_SUBSCRIPTION_EXPORT Broadcast {
    Network::IOService*mIOService;
    std::tr1::unordered_map<Network::Address,std::tr1::weak_ptr<Network::Stream>,Network::Address::Hasher > mTopLevelStreams;
    class UniqueLock;
    UniqueLock* mUniqueLock;    
public:
    class BroadcastStreamCallbacks;
    class BroadcastStream :Noncopyable{
        const std::tr1::shared_ptr<Network::Stream> mTopLevelStream;
        Network::Stream*mStream;
        friend class BroadcastStreamCallbacks;
    public:
        BroadcastStream(const std::tr1::shared_ptr<Network::Stream> &tls,
                        Network::Stream*stream);

        Network::Stream*operator -> () {
            return mStream;
        }
        Network::Stream*operator * () {
            return mStream;
        }
        ~BroadcastStream();
    };
    void initiateHandshake(BroadcastStream*strm, const Network::Address&addy, const UUID&name);

    BroadcastStream *establishBroadcast(const Network::Address&addy, 
                                        const UUID&name,
                                        const std::tr1::function<void(BroadcastStream*,
                                                                      Network::Stream::ConnectionStatus, 
                                                                      const std::string&reason)>& cb);
    std::tr1::shared_ptr<BroadcastStream> establishBroadcast(const Network::Address&addy, 
                                                             const UUID&name,
                                                             const std::tr1::function<void(const std::tr1::weak_ptr<BroadcastStream>&,
                                                                                           Network::Stream::ConnectionStatus, 
                                                                                           const std::string&reason)>& cb);
    Broadcast(Network::IOService*service);
    ~Broadcast();
};

} }
#endif
