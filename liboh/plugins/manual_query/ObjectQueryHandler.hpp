// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MQ_OBJECT_QUERY_HANDLER_HPP_
#define _SIRIKATA_OH_MQ_OBJECT_QUERY_HANDLER_HPP_

#include "ObjectQueryHandlerBase.hpp"
#include <sirikata/pintoloc/ProxSimulationTraits.hpp>
#include <sirikata/pintoloc/ReplicatedLocationUpdateListener.hpp>
#include <sirikata/pintoloc/ReplicatedLocationServiceCache.hpp>
#include <prox/geom/QueryHandler.hpp>
#include <prox/base/AggregateListener.hpp>
#include <sirikata/core/queue/ThreadSafeQueueWithNotification.hpp>
#include <sirikata/core/service/PollerService.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/core/prox/Defs.hpp>
#include <sirikata/core/util/InstanceMethodNotReentrant.hpp>

namespace Sirikata {


namespace Protocol {
namespace Prox {
class ProximityUpdate;
}
}

namespace OH {
namespace Manual {

class ObjectQueryHandler :
        public ObjectQueryHandlerBase,
        Prox::QueryEventListener<ObjectProxSimulationTraits, Prox::Query<ObjectProxSimulationTraits> >,
        Prox::AggregateListener<ObjectProxSimulationTraits>,
        public ReplicatedLocationUpdateListener
{
private:
    typedef Prox::QueryHandler<ObjectProxSimulationTraits> ProxQueryHandler;
    typedef Prox::Aggregator<ObjectProxSimulationTraits> ProxAggregator;
public:
    // MAIN Thread: All public interface is expected to be called only from the main thread.
    typedef Prox::Query<ObjectProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ObjectProxSimulationTraits> QueryEvent;

    ObjectQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, const OHDP::SpaceNodeID& space, Network::IOStrandPtr prox_strand);
    ~ObjectQueryHandler();

    // MAIN Thread:

    // Service Interface overrides
    virtual void start();
    virtual void stop();

    int32 objectQueries() const;
    int32 objectQueryMessages() const;

    // Index/Tree replication events from server queries
    void createdReplicatedIndex(ProxIndexID iid, ReplicatedLocationServiceCachePtr loc_cache, ServerID objects_from_server, bool dynamic_objects);
    void removedReplicatedIndex(ProxIndexID iid);

    // Query settings. These pass in the HostedObject so they don't
    // have to rely on the ReplicatedLocationServiceCache for implicit
    // settings since it's possible the ReplicatedLocationServiceCache hasn't
    // received (or might not receive!) information about the object
    // yet. Note that we use SpaceObjectReference here so we can
    // perform lookups in HostedObject, even though everything in this
    // space is specific to a single SpaceID.
    //
    // There is no addQuery, you always use updateQuery and it registers the
    // query if necessary
    void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& params);
    void removeQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef);
    // (Potential) Querier Lifetimes
    void presenceConnected(const ObjectReference& sporef);
    void presenceDisconnected(const ObjectReference& sporef);

    // ReplicatedLocationUpdateListener Interface
    // We override this for two reasons. First, a few callbacks are used to
    // update query parameters (location & bounds). Second, we use these updates
    // to generate LocUpdates for queriers that are subscribed to the object.
    virtual void onObjectAdded(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onObjectRemoved(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onParentUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onEpochUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onLocationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onOrientationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onBoundsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onMeshUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);
    virtual void onPhysicsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj);

    // PROX Thread:

    void handleCreatedReplicatedIndex(Liveness::Token alive, ProxIndexID iid, ReplicatedLocationServiceCachePtr loc_cache, ServerID objects_from_server, bool dynamic_objects);
    void handleRemovedReplicatedIndex(Liveness::Token alive, ProxIndexID iid);

    // QueryEventListener Interface
    void queryHasEvents(Query* query);

    // AggregateListener Interface
    virtual void aggregateCreated(ProxAggregator* handler, const ObjectReference& objid);
    virtual void aggregateChildAdded(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateChildRemoved(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateBoundsUpdated(ProxAggregator* handler, const ObjectReference& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateDestroyed(ProxAggregator* handler, const ObjectReference& objid);
    virtual void aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers, uint32 nchildren);

    ProxQueryHandler* getQueryHandler(const String& handler_name);

    void commandListInfo(const OHDP::SpaceNodeID& snid, Command::Result& result);
    void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    void commandListQueriers(const OHDP::SpaceNodeID& snid, Command::Result& result);
    void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

private:

    // MAIN Thread: These are utility methods which should only be called from the main thread.

    // Update queries based on current state.
    void handleDeliverEvents();

    // Update subscribed queriers that the given object was updated. This
    // currently takes the brute force approach of updating all properties and
    // uses the most up-to-date info, even if that's actually newer than the
    // update that triggered this.
    void handleNotifySubscribersLocUpdate(Liveness::Token alive, ReplicatedLocationServiceCache* loccache, const ObjectReference& oref);

    // Object queries
    void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, SolidAngle sa, uint32 max_results);
    void updateQuery(const ObjectReference& obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results);


    // PROX Thread: These are utility methods which should only be called from the prox thread.

    // Reusable utility for registering the query with a particular
    // index. Useful since we may need to register in different conditions --
    // new query, new index, index becomes relevant to query, etc.
    void registerObjectQueryWithIndex(const ObjectReference& object, ProxIndexID index_id, ProxQueryHandler* handler, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results);
    // Utility for registering query with all indices for a ServerID. This just
    // calls registerObjectQueryWithIndex for each index on the server.
    void registerObjectQueryWithServer(const ObjectReference& object, ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results);
    void unregisterObjectQueryWithIndex(const ObjectReference& object, ProxIndexID index_id);
    void unregisterObjectQueryWithServer(const ObjectReference& object, ServerID sid);

    // Events on queries/objects
    void handleUpdateObjectQuery(Liveness::Token alive, const ObjectReference& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results);
    void handleRemoveObjectQuery(Liveness::Token alive, const ObjectReference& object, bool notify_main_thread);
    void handleDisconnectedObject(Liveness::Token alive, const ObjectReference& object);

    // Generate query events based on results collected from query handlers
    void generateObjectQueryEvents(Query* query);

    typedef std::set<ObjectReference> ObjectSet;
    typedef std::tr1::unordered_map<ProxIndexID, Query*> IndexQueryMap;
    typedef std::tr1::unordered_set<ServerID> ServerIDSet;
    // A single object query may have queries registered against many query
    // handlers for different replicated trees.
    struct ObjectQueryData {
        // We need to store the query parameters because a query may get
        // registered when no trees have been replicated yet.
        TimedMotionVector3f loc;
        BoundingSphere3f bounds;
        SolidAngle angle;
        uint32 max_results;
        // And we also keep track of all the individual queries against
        // replicated indices.
        IndexQueryMap queries;
        // And which entire Servers they are subscribed to.
        ServerIDSet servers;
    };
    typedef std::tr1::shared_ptr<ObjectQueryData> ObjectQueryDataPtr;
    typedef std::tr1::unordered_map<ObjectReference, ObjectQueryDataPtr, ObjectReference::Hasher> ObjectQueryMap;
    // Individual libprox queries are keyed by both the object who registered a
    // query and the replicated index the individual query operates on
    typedef std::pair<ObjectReference, ProxIndexID> ObjectIndexQueryKey;
    typedef std::tr1::unordered_map<Query*, ObjectIndexQueryKey> InvertedObjectQueryMap;

    // We replicate many object indices, so we need to generate many query
    // handlers, one for each replicated index. We'll track them by their unique
    // index IDs. Since the query handler can't track all the info we might care
    // about, we put it in a struct with a bit more metadata
    struct ReplicatedIndexQueryHandler {
        ReplicatedIndexQueryHandler(ProxQueryHandler* handler_, ReplicatedLocationServiceCachePtr loccache_, ServerID from_, bool dynamic_)
         : handler(handler_), loccache(loccache_), from(from_), dynamic(dynamic_) {}
        ReplicatedIndexQueryHandler()
         : handler(NULL), loccache(), from(NullServerID), dynamic(true) {}

        ProxQueryHandler* handler;
        ReplicatedLocationServiceCachePtr loccache;
        // The Server this tree was replicated from. This may not be unique.
        ServerID from;
        // Whether this tree includes dynamic objects
        bool dynamic;
    };
    typedef std::tr1::unordered_map<ProxIndexID, ReplicatedIndexQueryHandler> ReplicatedIndexQueryHandlerMap;
    // Sort of the inverse of the above: aggregator (libprox handler) ->
    // ProxIndexID lets us get back to the data
    typedef std::tr1::unordered_map<ProxAggregator*, ProxIndexID> InverseReplicatedIndexQueryHandlerMap;

    typedef std::tr1::shared_ptr<ObjectSet> ObjectSetPtr;


    // MAIN Thread - Should only be accessed in methods used by the main thread

    // We track subscriptions in the main thread since loc events need to be
    // executed in the main thread. This let's us just create one update and
    // reuse it across all subscribers.
    // Set of subscribers
    typedef std::tr1::unordered_set<ObjectReference, ObjectReference::Hasher> SubscriberSet;
    typedef std::tr1::shared_ptr<SubscriberSet> SubscriberSetPtr;
    // Map of object -> subscribers to that object
    typedef std::tr1::unordered_map<ObjectReference, SubscriberSetPtr, ObjectReference::Hasher> SubscribersMap;
    SubscribersMap mSubscribers;


    // PROX Thread - Should only be accessed in methods used by the prox thread

    void tickQueryHandler();

    // All queryHasEvents calls are going to not be reentrant unless you're very
    // careful, so the base class provides this so it's easy to verify it.
    InstanceMethodNotReentrant mQueryHasEventsNotRentrant;

    // These track all objects being reported to this server and
    // answer queries for objects connected to this server.
    ObjectQueryMap mObjectQueries;
    InvertedObjectQueryMap mInvertedObjectQueries;
    ReplicatedIndexQueryHandlerMap mObjectQueryHandlers;
    InverseReplicatedIndexQueryHandlerMap mInverseObjectQueryHandlers;
    bool mObjectDistance; // Using distance queries
    PollerService mObjectHandlerPoller;

    // Threads: Thread-safe data used for exchange between threads
    struct ProximityResultInfo {
        ProximityResultInfo(const ObjectReference& q, Sirikata::Protocol::Prox::ProximityUpdate* res)
         : querier(q), results(res)
        {}

        ObjectReference querier;
        Sirikata::Protocol::Prox::ProximityUpdate* results;
    };
    Sirikata::ThreadSafeQueueWithNotification<ProximityResultInfo> mObjectResults; // object query results that need to be sent

}; //class ObjectQueryHandler

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MQ_OBJECT_QUERY_HANDLER_HPP_
