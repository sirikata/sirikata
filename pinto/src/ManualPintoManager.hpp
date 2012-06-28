// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_MANUAL_PINTO_MANAGER_HPP_
#define _SIRIKATA_MANUAL_PINTO_MANAGER_HPP_

#include "PintoManagerBase.hpp"

#include <prox/manual/QueryHandler.hpp>
#include <prox/base/QueryEventListener.hpp>
#include <prox/base/LocationUpdateListener.hpp>

#include "PintoManagerLocationServiceCache.hpp"

namespace Sirikata {

/** ManualPintoManager responds to queries from space servers for a top-level
 *  tree, representing the highest-level aggregates. Each query has a cut,
 *  ManualPintoManager accepts commands to control it, and replicates data about
 *  nodes along and above the cut to the client space server.
 */
class ManualPintoManager
    : public PintoManagerBase,
      public Prox::QueryEventListener<ServerProxSimulationTraits, Prox::ManualQuery<ServerProxSimulationTraits> >
{
public:
    ManualPintoManager(PintoContext* ctx);
    virtual ~ManualPintoManager();

private:
    typedef Prox::ManualQueryHandler<ServerProxSimulationTraits> ProxQueryHandler;
    typedef Prox::ManualQuery<ServerProxSimulationTraits> Query;
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
         : query(NULL),
           seqno(0)
        {}

        Query* query;
        uint64 seqno;
    };
    std::tr1::unordered_map<Sirikata::Network::Stream*, ClientData> mClients;
    // Reverse index for queryHasEvents callbacks
    typedef std::tr1::unordered_map<Query*, Sirikata::Network::Stream*> ClientsByQuery;
    ClientsByQuery mClientsByQuery;

    ProxQueryHandler* mQueryHandler;
    Time mLastTime;
    Duration mDt;
}; // class ManualPintoManager

} // namespace Sirikata

#endif //_SIRIKATA_MANUAL_PINTO_MANAGER_HPP_
