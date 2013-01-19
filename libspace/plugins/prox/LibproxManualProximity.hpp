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
#include "ManualReplicatedRequestManager.hpp"
#include <boost/tr1/tuple.hpp>
#include <sirikata/pintoloc/CopyableLocUpdate.hpp>

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
    virtual void stop();

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results);
    virtual void addQuery(UUID obj, const String& params);
    virtual void removeQuery(UUID obj);

    // PollingService Interface
    virtual void poll();

    // PintoServerQuerierListener Interface
    virtual void onPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update);
    virtual void onPintoServerLocUpdate(const LocUpdate& update);

    // LocationServiceListener Interface - Used for deciding when to switch
    // objects between static/dynamic, get raw loc updates from other
    // servers so they can be applied to the correct replicated indexes
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void onLocationUpdateFromServer(const ServerID sid, const Sirikata::Protocol::Loc::LocationUpdate& update);

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg);

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
    virtual void aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers, uint32 nchildren);

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

    // First handling stage of ProximityResults, getting them into
    // Loc. See handleUpdateServerQueryResultsToReplicatedTrees for
    // second half
    void handleUpdateServerQueryResultsToLocService(ServerID sid, const Sirikata::Protocol::Prox::ProximityResults& results);

    // MAIN Thread  OH queries:

    virtual int32 objectHostQueries() const;
    virtual int32 serverQueries() const;

    // ObjectHost message management
    void handleObjectHostSubstream(int success, OHDPSST::StreamPtr substream, SeqNoPtr seqNo);

    // Server queries management
    void updateServerQuery(ServerID sid, const String& raw_query);
    void updateServerQueryResults(ServerID sid, const Sirikata::Protocol::Prox::ProximityResults& results);

    std::deque<Message*> mServerResultsToSend; // actually both
                                               // results + commands
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
    void handleOnPintoServerLocUpdate(const CopyableLocUpdate& update);

    void handleUpdateServerQuery(ServerID sid, const String& raw_query);
    void handleUpdateServerQueryResultsToReplicatedTrees(ServerID sid, const Sirikata::Protocol::Prox::ProximityResults& results);
    void handleUpdateServerQueryResultsToRetryRequests(ServerID sid, const Sirikata::Protocol::Prox::ProximityResults& results);

    // Helpers for un/registering a server query
    void registerServerQuery(const ServerID& querier);
    void unregisterServerQuery(const ServerID& querier);

    SeqNoPtr getOrCreateSeqNoInfo(const ServerID server_id);
    void eraseSeqNoInfo(const ServerID server_id);

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
    virtual void commandListQueriers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);



    String mQueryHandlerType;
    String mQueryHandlerOptions;
    String mQueryHandlerNodeDataType;


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

    // Maintain two maps for ProxIndexIDs. We actually get 2
    // ProxIndexIDs for each replicated tree: 1 from the origin server
    // and one from the query handler we generate on top of
    // that. Since we might get the same ID from multiple origin
    // servers, those are not unique. We always want to report our
    // locally generated version to clients. However, that also means
    // we need to be able to map between the two/get back to the
    // original data.
    // Maintain a map of *locally generated* ProxIndexIDs to the
    // source server + remote index ID responsible for them. If we only
    // have the local ProxIndexID, this lets us traverse the above data
    // structure
    typedef std::pair<ServerID, ProxIndexID> NodeProxIndexID;
    typedef std::tr1::unordered_map<ProxIndexID, NodeProxIndexID> IndexToServerMap;
    IndexToServerMap mLocalToRemoteIndexMap;

    // And a map from aggregator (libprox handler) -> (ServerID, ProxIndexID) for when we
    // get aggregate callbacks. This ProxIndexID is the *origin
    // server's* identifier since we use it to get back to the
    // replicated data.
    typedef std::tr1::unordered_map<ProxAggregator*, NodeProxIndexID> InverseReplicatedIndexQueryHandlerMap;
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

    // These track objects on this server and respond to OH queries.
    OHQueryMap mOHQueries[NUM_OBJECT_CLASSES];
    InvertedOHQueryMap mInvertedOHQueries;
    // And their results
    Sirikata::ThreadSafeQueue<OHResult> mOHResults;
    typedef std::tr1::unordered_map<OHDP::NodeID, SeqNoPtr, OHDP::NodeID::Hasher> OHSeqNoInfoMap;
    OHSeqNoInfoMap mOHSeqNos;
    // For object hosts *only* we track requests that failed we we can try to
    // reapply them if new data comes in. This masks failures that occur when
    // we're still waiting for a request to another server (or top-level pinto)
    // to be fulfilled and results to be returned.
    ManualReplicatedRequestManager mOHRequestsManager;


    // Server-to-Server queries
    typedef std::tr1::unordered_map<ServerID, ProxQuery*> ServerQueryMap;
    typedef std::tr1::unordered_map<ProxQuery*, ServerID> InvertedServerQueryMap;
    ServerQueryMap mServerQueries[NUM_OBJECT_CLASSES];
    InvertedServerQueryMap mInvertedServerQueries;
    Sirikata::ThreadSafeQueue<Message*> mServerResults; // server query results + commands
    typedef std::tr1::unordered_map<ServerID, SeqNoPtr> ServerSeqNoInfoMap;
    ServerSeqNoInfoMap mServerSeqNos;


    // And these are the actual data structures for local queries --
    // handling both server queries (their entire query) and OH
    // queries (when they get into this this tree).

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

    ProxQueryHandlerData mLocalQueryHandler[NUM_OBJECT_CLASSES];
    PollerService mLocalHandlerPoller;
}; // class LibproxManualProximity

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_MANUAL_PROXIMITY_HPP_
