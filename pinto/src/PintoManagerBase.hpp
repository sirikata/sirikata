// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_PINTO_MANAGER_BASE_HPP_
#define _SIRIKATA_PINTO_MANAGER_BASE_HPP_

#include "PintoContext.hpp"
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/Stream.hpp>

#include "ProxSimulationTraits.hpp"

#include <prox/base/QueryEventListener.hpp>
#include <prox/base/AggregateListener.hpp>

#include "PintoManagerLocationServiceCache.hpp"

#include <sirikata/pintoloc/BaseProxCommandable.hpp>

namespace Sirikata {

/** PintoManagerBase is the base class for classes the answer queries looking
 *  for other servers that are relevant to object queries. It implements basic
 *  connectivity, leaving the implementation of queries and the particular
 *  format of requests and responses to implementations.
 */
class PintoManagerBase
    : public Service,
      public Prox::AggregateListener<ServerProxSimulationTraits>,
      Pinto::BaseProxCommandable
{
public:
    typedef Prox::Aggregator<ServerProxSimulationTraits> ProxAggregator;

    PintoManagerBase(PintoContext* ctx);
    virtual ~PintoManagerBase();

    virtual void start();
    virtual void stop();

private:
    void newStreamCallback(Sirikata::Network::Stream* newStream, Sirikata::Network::Stream::SetCallbacks& setCallbacks);

    void handleClientConnection(Sirikata::Network::Stream* stream, Network::Stream::ConnectionStatus status, const std::string &reason);
    void handleClientReceived(Sirikata::Network::Stream* stream, Network::Chunk& data, const Network::Stream::PauseReceiveCallback& pause) ;
    void handleClientReadySend(Sirikata::Network::Stream* stream);

protected:
    // Events that implementations might care about
    virtual void onConnected(Sirikata::Network::Stream* newStream);
    virtual void onInitialMessage(Sirikata::Network::Stream* stream);
    virtual void onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds);
    virtual void onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms);
    virtual void onQueryUpdate(Sirikata::Network::Stream* stream, const String& update);
    virtual void onDisconnected(Sirikata::Network::Stream* stream);


    // AggregateListener Interface
    virtual void aggregateCreated(ProxAggregator* handler, const ServerID& objid);
    virtual void aggregateChildAdded(ProxAggregator* handler, const ServerID& objid, const ServerID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateChildRemoved(ProxAggregator* handler, const ServerID& objid, const ServerID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateBoundsUpdated(ProxAggregator* handler, const ServerID& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateQueryDataUpdated(ProxAggregator* handler, const ServerID& objid, const String& extra_query_data);
    virtual void aggregateDestroyed(ProxAggregator* handler, const ServerID& objid);
    virtual void aggregateObserved(ProxAggregator* handler, const ServerID& objid, uint32 nobservers, uint32 nchildren);


    // Utility for implementations so they don't have to track ServerIDs
    ServerID streamServerID(Sirikata::Network::Stream*) const;


    PintoContext* mContext;
    Network::IOStrand* mStrand;
    Network::StreamListener* mListener;

    PintoManagerLocationServiceCache* mLocCache;

private:
    typedef std::tr1::unordered_map<Sirikata::Network::Stream*, ServerID> StreamServerIDMap;
    StreamServerIDMap mStreamServers;
}; // class PintoManagerBase

} // namespace Sirikata

#endif //_SIRIKATA_PINTO_MANAGER_BASE_HPP_
