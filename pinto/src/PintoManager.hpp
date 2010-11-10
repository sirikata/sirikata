/*  Sirikata
 *  PintoManager.hpp
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

#ifndef _SIRIKATA_PINTO_MANAGER_HPP_
#define _SIRIKATA_PINTO_MANAGER_HPP_

#include "PintoContext.hpp"
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/Stream.hpp>

#include <sirikata/space/ProxSimulationTraits.hpp>

#include <prox/QueryHandler.hpp>
#include <prox/QueryEventListener.hpp>
#include <prox/LocationUpdateListener.hpp>

#include "PintoManagerLocationServiceCache.hpp"

namespace Sirikata {

/** PintoManager oversees space servers, tracking the regions they cover and
 *  their largest objects.  It answers standing queries, much like the Pinto
 *  services in each space server, but answers them for entire servers.  Pinto
 *  services on each space server use it to determine which other servers they
 *  may need to query.
 */
class PintoManager : public Service, public Prox::QueryEventListener<ServerProxSimulationTraits> {
public:
    PintoManager(PintoContext* ctx);
    ~PintoManager();

    virtual void start();
    virtual void stop();

private:
    typedef Prox::QueryHandler<ServerProxSimulationTraits> ProxQueryHandler;
    typedef Prox::Query<ServerProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ServerProxSimulationTraits> QueryEvent;

    void newStreamCallback(Sirikata::Network::Stream* newStream, Sirikata::Network::Stream::SetCallbacks& setCallbacks);

    void handleClientConnection(Sirikata::Network::Stream* stream, Network::Stream::ConnectionStatus status, const std::string &reason);
    void handleClientReceived(Sirikata::Network::Stream* stream, Network::Chunk& data, const Network::Stream::PauseReceiveCallback& pause) ;
    void handleClientReadySend(Sirikata::Network::Stream* stream);

    // QueryEventListener Interface
    virtual void queryHasEvents(Query* query);

    // Tick the QueryHandler. We don't actually care about time since Servers
    // won't be in motion. Should only be called when the server data or query
    // data is updated.
    void tick();

    PintoContext* mContext;
    Network::IOStrand* mStrand;
    Network::StreamListener* mListener;

    struct ClientData {
        ClientData()
         : server(NullServerID),
           query(NULL)
        {}

        ServerID server;
        Query* query;
    };
    std::tr1::unordered_map<Sirikata::Network::Stream*, ClientData> mClients;
    // Reverse index for queryHasEvents callbacks
    typedef std::tr1::unordered_map<Query*, Sirikata::Network::Stream*> ClientsByQuery;
    ClientsByQuery mClientsByQuery;

    PintoManagerLocationServiceCache* mLocCache;
    ProxQueryHandler* mQueryHandler;
    Time mLastTime;
    Duration mDt;
};

} // namespace Sirikata

#endif //_SIRIKATA_PINTO_MANAGER_HPP_
