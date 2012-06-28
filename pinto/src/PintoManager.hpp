// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_PINTO_MANAGER_HPP_
#define _SIRIKATA_PINTO_MANAGER_HPP_

#include "PintoManagerBase.hpp"

#include <prox/geom/QueryHandler.hpp>
#include <prox/base/QueryEventListener.hpp>
#include <prox/base/LocationUpdateListener.hpp>

#include "PintoManagerLocationServiceCache.hpp"

namespace Sirikata {

/** PintoManager oversees space servers, tracking the regions they cover and
 *  their largest objects.  It answers standing queries, letting each server
 *  know what other servers have objects that might satisfy their aggregate
 *  queries.
 */
class PintoManager
    : public PintoManagerBase,
      public Prox::QueryEventListener<ServerProxSimulationTraits, Prox::Query<ServerProxSimulationTraits> >
{
public:
    PintoManager(PintoContext* ctx);
    virtual ~PintoManager();

private:
    typedef Prox::QueryHandler<ServerProxSimulationTraits> ProxQueryHandler;
    typedef Prox::Query<ServerProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ServerProxSimulationTraits> QueryEvent;

    // PintoManagerBase overrides
    virtual void onConnected(Sirikata::Network::Stream* newStream);
    virtual void onInitialMessage(Sirikata::Network::Stream* stream);
    virtual void onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds);
    virtual void onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms);
    virtual void onQueryUpdate(Sirikata::Network::Stream* stream, const String& update);
    virtual void onDisconnected(Sirikata::Network::Stream* stream);


    // QueryEventListener Interface
    virtual void queryHasEvents(Query* query);

    // Tick the QueryHandler. We don't actually care about time since Servers
    // won't be in motion. Should only be called when the server data or query
    // data is updated.
    void tick();

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

    ProxQueryHandler* mQueryHandler;
    Time mLastTime;
    Duration mDt;
};

} // namespace Sirikata

#endif //_SIRIKATA_PINTO_MANAGER_HPP_
