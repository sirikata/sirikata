// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_
#define _SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_

#include "LibproxProximityBase.hpp"
#include <prox/manual/QueryHandler.hpp>
#include <prox/base/LocationUpdateListener.hpp>
#include <prox/base/AggregateListener.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/pintoloc/ManualReplicatedClient.hpp>
#include <boost/tr1/tuple.hpp>

namespace Sirikata {

/** Implementation of Proximity using manual traversal of the object tree. The
 *  Proximity library provides the tree and a simple interface to manage cuts and
 *  the client manages the cut.
 */
class LibproxManualProximity :
        public LibproxProximityBase,
        Prox::AggregateListener<ObjectProxSimulationTraits>,
        Prox::QueryEventListener<ObjectProxSimulationTraits, Prox::ManualQuery<ObjectProxSimulationTraits> >,
        Pinto::Manual::ReplicatedClientWithID<ServerID>::Parent
{
private:
    typedef Prox::ManualQueryHandler<ObjectProxSimulationTraits> ProxQueryHandler;
    typedef std::tr1::shared_ptr<ProxQueryHandler> ProxQueryHandlerPtr;
    typedef Prox::Aggregator<ObjectProxSimulationTraits> ProxAggregator;
    typedef Prox::ManualQuery<ObjectProxSimulationTraits> ProxQuery;
    typedef Prox::QueryEvent<ObjectProxSimulationTraits> ProxQueryEvent;
    typedef Prox::LocationServiceCache<ObjectProxSimulationTraits> ProxLocCache;

    typedef std::pair<OHDP::NodeID, Sirikata::Protocol::Object::ObjectMessage*> OHResult;
public:
    LibproxManualProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr);
    ~LibproxManualProximity();

    // MAIN Thread:

    // Service Interface overrides
    virtual void start();

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results);
    virtual void addQuery(UUID obj, const String& params);
    virtual void removeQuery(UUID obj);

    // PollingService Interface
    virtual void poll();

    // PintoServerQuerierListener Interface
    virtual void onPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update);

    // LocationServiceListener Interface - Used for deciding when to switch
    // objects between static/dynamic
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);

    // MigrationDataClient Interface
    virtual std::string migrationClientTag();
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server);
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data);

    // ObjectHostSessionListener Interface
    virtual void onObjectHostSession(const OHDP::NodeID& id, ObjectHostSessionPtr oh_sess);
    virtual void onObjectHostSessionEnded(const OHDP::NodeID& id);


    // PROX Thread:

    // AggregateListener Interface
    virtual void aggregateCreated(ProxAggregator* handler, const ObjectReference& objid);
    virtual void aggregateChildAdded(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateChildRemoved(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateBoundsUpdated(ProxAggregator* handler, const ObjectReference& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateDestroyed(ProxAggregator* handler, const ObjectReference& objid);
    virtual void aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers);

    // QueryEventListener Interface
    void queryHasEvents(ProxQuery* query);

private:
    typedef std::tr1::unordered_set<ObjectReference, ObjectReference::Hasher> ObjectIDSet;
    typedef std::tr1::unordered_set<OHDP::NodeID, OHDP::NodeID::Hasher> OHQuerierSet;

    struct ProxQueryHandlerData;
    typedef Pinto::Manual::ReplicatedClientWithID<ServerID> ReplicatedClient;
    typedef ReplicatedClient::Parent ReplicatedClientParent;
    typedef std::tr1::shared_ptr<ReplicatedClient> ReplicatedClientPtr;

    // MAIN Thread:


    // MAIN Thread  Server-to-server queries (including top-level
    // pinto):

    // ReplicatedClientWithID Interface Part I
    virtual void sendReplicatedClientProxMessage(ReplicatedClient* client, const ServerID& evt_src_server, const String& msg);


    // MAIN Thread  OH queries:

    virtual int32 objectHostQueries() const;

    // ObjectHost message management
    void handleObjectHostSubstream(int success, OHDPSST::Stream::Ptr substream, SeqNoPtr seqNo);

    std::deque<OHResult> mOHResultsToSend;


    // PROX Thread:

    void tickQueryHandlers();

    // PROX Thread -- Server-to-server and top-level pinto

    // ReplicatedClientWithID Interface Part II (only invoked through prox
    // strand because the methods that invoke these are only called
    // from this class in the prox strand)
    virtual void onCreatedReplicatedIndex(ReplicatedClient* client, const ServerID& evt_src_server, ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects);
    virtual void onDestroyedReplicatedIndex(ReplicatedClient* client, const ServerID& evt_src_server, ProxIndexID proxid);

    // Server events, some from the main thread
    // Override for forced disconnections
    virtual void handleForcedDisconnection(ServerID server);

    // Real handlers for PintoServerQuerierListener and
    // ReplicatedClientWithID -- these generate/destroy LocCaches and
    // Query Handlers
    void handleOnPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update);

    // PROX Thread -- OH queries

    // Real handler for OH requests, in the prox thread
    void handleObjectHostProxMessage(const OHDP::NodeID& id, const String& data, SeqNoPtr seqNo);
    // Real handler for OH disconnects
    void handleObjectHostSessionEnded(const OHDP::NodeID& id);
    void destroyQuery(const OHDP::NodeID& id);

    // Helpers for un/registering a query against an entire Server
    void registerOHQueryWithServerHandlers(const OHDP::NodeID& querier, ServerID queried_node);
    void unregisterOHQueryWithServerHandlers(const OHDP::NodeID& querier, ServerID queried_node);
    // Helpers for un/registering a query against a single index
    void registerOHQueryWithServerIndexHandler(const OHDP::NodeID& querier, ServerID queried_node, ProxIndexID queried_index);
    void unregisterOHQueryWithServerIndexHandler(const OHDP::NodeID& querier, ServerID queried_node, ProxIndexID queried_index);

    // Decides whether a query handler should handle a particular object.
    bool handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const ObjectReference& obj_id, bool is_local, bool is_aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize);
    // The real handler for moving objects between static/dynamic
    void handleCheckObjectClassForHandlers(const ObjectReference& objid, bool is_static, ProxQueryHandlerData handlers[NUM_OBJECT_CLASSES]);
    virtual void trySwapHandlers(bool is_local, const ObjectReference& objid, bool is_static);

    SeqNoPtr getSeqNoInfo(const OHDP::NodeID& node);
    void eraseSeqNoInfo(const OHDP::NodeID& node);

    // Command handlers
    virtual void commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    bool parseHandlerName(const String& name, ProxQueryHandler** handler_out);
    virtual void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

    // The layout is complicated and nested here because we have many
    // layers -- queriers, space servers, indices, etc. First, we'll
    // start with how the replicated data and query handlers are
    // managed.

    // ReplicateClients for nodes will manage the basic replication of
    // remote trees -- they are the "session" with the remote node,
    // manage the cut(s), and notify us when indices are
    // added/destroyed on a remote node.
    // No new types here, just ReplicatedClientPtr

    // On top of the replicated clients we need to build query
    // processors for OH queries. Each server needs a replicated
    // client, and within that we may have many replicated indices and
    // handlers:
    // First, for any given replicated index, we need to provide a
    // query handler for it, hold on to the loccache for it, and track
    // its basic properties. Note that this only tracks info about the
    // handler, not about the registered queries.
    struct ReplicatedIndexQueryHandler {
        ReplicatedIndexQueryHandler(ReplicatedLocationServiceCachePtr loccache_, ProxQueryHandlerPtr handler_)
         : loccache(loccache_), handler(handler_) {}
        ReplicatedIndexQueryHandler()
        {}

        // Prereqs and properties:
        ReplicatedLocationServiceCachePtr loccache;

        // The handler and its data:
        ProxQueryHandlerPtr handler;
        // Additions and removals that need to be processed on the
        // next tick. These need to be handled carefully since they
        // can be due to swapping between handlers. If they are
        // processed in the wrong order we could end up generating
        // [addition, removal] instead of [removal, addition] for
        // queriers.
        ObjectIDSet additions;
        ObjectIDSet removals;
    };
    // Next, we collect these for each server. However, here we start
    // tracking registered queries as well. We keep track of which
    // queries got to requesting data from this node (i.e. they
    // refined to get to this server) so on the first or new
    // replicated trees we can actually register queries for them.
    typedef std::tr1::unordered_map<ProxIndexID, ReplicatedIndexQueryHandler> ReplicatedIndexQueryHandlerMap;
    struct ReplicatedServerData {
        // The client that interacts with the other server to manage
        // replicated data, and lets us know about new indices.
        ReplicatedClientPtr client;
        // Handlers for individual replicated indices
        ReplicatedIndexQueryHandlerMap handlers;
        // Queriers that have refined to this tree.
        OHQuerierSet queriers;
    };
    // Finally, use a map of these, one for each server, to actually
    // instantiate all of this
    typedef std::tr1::unordered_map<ServerID, ReplicatedServerData> ReplicatedServerDataMap;
    ReplicatedServerDataMap mReplicatedServerDataMap;

    // Also maintain a map of ProxIndexIDs to their source servers. If we only
    // have the ProxIndexID, this lets us traverse the above data structure
    typedef std::tr1::unordered_map<ProxIndexID, ServerID> IndexToServerMap;
    IndexToServerMap mIndexToServerMap;

    // And a map from aggregator (libprox handler) -> ProxIndexID for when we
    // get aggregate callbacks
    typedef std::tr1::unordered_map<ProxAggregator*, ProxIndexID> InverseReplicatedIndexQueryHandlerMap;
    InverseReplicatedIndexQueryHandlerMap mAggregatorToIndexMap;

    // Now, with all the data replicated and query handlers setup, we
    // can move on to the management of the queries themselves.

    // We need a record of each individual query based on which tree its in,
    // i.e. based on the space server and index ID. We could just have a map of
    // <OHID, SSID, IndexID> -> ProxQuery. However, for destroying complete
    // queries, we need the set of all individual queries associated with an OH
    // query, and for disconnecting space servers we ultimately need to clear
    // out all individual queries with an associate space server. So instead, we
    // build the equivalent but more indirect
    // OHID -> (SSID -> (IndexID -> ProxQuery)).
    typedef std::tr1::unordered_map<ProxIndexID, ProxQuery*> IndexToQueryMap;
    typedef std::tr1::unordered_map<ServerID, IndexToQueryMap> ServerToIndexToQueryMap;
    typedef std::tr1::unordered_map<OHDP::NodeID, ServerToIndexToQueryMap, OHDP::NodeID::Hasher> OHQueryToServerToIndexToQueryMap;
    OHQueryToServerToIndexToQueryMap mOHRemoteQueries;
    // We also need to keep track of the inverted index so when we get results
    // from the query process and are given a ProxQuery*, we can get back to the
    // data.
    typedef std::tr1::tuple<OHDP::NodeID, ServerID, ProxIndexID> OHRemoteQueryID;
    typedef std::tr1::unordered_map<ProxQuery*, OHRemoteQueryID> InvertedOHRemoteQueryMap;
    InvertedOHRemoteQueryMap mInvertedOHRemoteQueries;

    // A single query actually consists of many subqueries against
    // individual nodes, e.g. a query from OH1 consists of queries
    // against top-level pinto and ss1, ss2, ss4.
    // We maintain a map of these, one per registered OH query
    typedef std::tr1::unordered_map<OHDP::NodeID, ProxQuery*, OHDP::NodeID::Hasher> OHQueryMap;
    // And finally, we need the reverse index -- given an individual
    // query we get in a callback, which OH query does it belong to,
    // and which space server is it registered with?
    typedef std::tr1::unordered_map<ProxQuery*, OHDP::NodeID> InvertedOHQueryMap;

    struct ProxQueryHandlerData {
        ProxQueryHandler* handler;
        // Additions and removals that need to be processed on the
        // next tick. These need to be handled carefully since they
        // can be due to swapping between handlers. If they are
        // processed in the wrong order we could end up generating
        // [addition, removal] instead of [removal, addition] for
        // queriers.
        ObjectIDSet additions;
        ObjectIDSet removals;
    };

    // These track objects on this server and respond to OH queries.
    OHQueryMap mOHQueries[NUM_OBJECT_CLASSES];
    InvertedOHQueryMap mInvertedOHQueries;
    ProxQueryHandlerData mOHQueryHandler[NUM_OBJECT_CLASSES];

    PollerService mOHHandlerPoller;
    Sirikata::ThreadSafeQueue<OHResult> mOHResults;

    typedef std::tr1::unordered_map<OHDP::NodeID, SeqNoPtr, OHDP::NodeID::Hasher> OHSeqNoInfoMap;
    OHSeqNoInfoMap mOHSeqNos;

}; // class LibproxManualProximity

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_
