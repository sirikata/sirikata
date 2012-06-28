/*  Sirikata
 *  LibproxProximity.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LIBPROX_PROXIMITY_HPP_
#define _SIRIKATA_LIBPROX_PROXIMITY_HPP_

#include "LibproxProximityBase.hpp"
#include "ProxSimulationTraits.hpp"
#include <prox/geom/QueryHandler.hpp>
#include <prox/base/LocationUpdateListener.hpp>
#include <prox/base/AggregateListener.hpp>

#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>

namespace Sirikata {

class ProximityInputEvent;
class ProximityOutputEvent;

class LibproxProximity :
        public LibproxProximityBase,
        Prox::QueryEventListener<ObjectProxSimulationTraits, Prox::Query<ObjectProxSimulationTraits> >,
        Prox::AggregateListener<ObjectProxSimulationTraits>
{
private:
    typedef Prox::QueryHandler<ObjectProxSimulationTraits> ProxQueryHandler;
    typedef Prox::Aggregator<ObjectProxSimulationTraits> ProxAggregator;
public:
    // MAIN Thread: All public interface is expected to be called only from the main thread.
    typedef Prox::Query<ObjectProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ObjectProxSimulationTraits> QueryEvent;

    LibproxProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr);
    ~LibproxProximity();

    // MAIN Thread:

    // Service Interface overrides
    virtual void start();

    // ObjectSessionListener Interface
    virtual void newSession(ObjectSession* session);
    virtual void sessionClosed(ObjectSession* session);

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results);
    virtual void addQuery(UUID obj, const String& params);
    virtual void removeQuery(UUID obj);

    // PintoServerQuerierListener Interface
    virtual void onPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update);

    // LocationServiceListener Interface
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg);

    // MigrationDataClient Interface
    virtual std::string migrationClientTag();
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server);
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data);


    // PROX Thread:

    // AggregateListener Interface
    virtual void aggregateCreated(ProxAggregator* handler, const UUID& objid);
    virtual void aggregateChildAdded(ProxAggregator* handler, const UUID& objid, const UUID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateChildRemoved(ProxAggregator* handler, const UUID& objid, const UUID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateBoundsUpdated(ProxAggregator* handler, const UUID& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateDestroyed(ProxAggregator* handler, const UUID& objid);
    virtual void aggregateObserved(ProxAggregator* handler, const UUID& objid, uint32 nobservers);

    // QueryEventListener Interface
    void queryHasEvents(Query* query);


private:
    struct ProxQueryHandlerData;
    typedef std::tr1::unordered_set<ServerID> ServerSet;

    void handleObjectProximityMessage(const UUID& objid, void* buffer, uint32 length);

    // BOTH Threads - uses thread safe data

    // PintoServerQuerier management
    // Utility -- setup all known servers for a server query update
    void addAllServersForUpdate();
    // Get/add servers for sending and update of our aggregate query to
    void getServersForAggregateQueryUpdate(ServerSet* servers_out);
    void addServerForAggregateQueryUpdate(ServerID sid);
    // Initiate updates to aggregate queries and stats over all objects, used to
    // trigger updated requests to top-level pinto and other servers
    void updateAggregateQuery(const SolidAngle sa, uint32 max_count);
    // Number of servers we have active queries to
    uint32 numServersQueried();


    // MAIN Thread: These are utility methods which should only be called from the main thread.
    virtual int32 objectQueries() const;
    virtual int32 serverQueries() const;

    // Update queries based on current state.
    void poll();

    // Server queries requests, generated by receiving messages
    void updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa, uint32 max_results);
    void removeQuery(ServerID sid);

    // Object queries
    void updateQuery(UUID obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results);

    // Send a query add/update request to any servers we've marked as needing an
    // update
    void sendQueryRequests();


    // PROX Thread: These are utility methods which should only be called from the prox thread.

    // Handle various query events from the main thread
    void handleUpdateServerQuery(const ServerID& server, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results);
    void handleRemoveServerQuery(const ServerID& server);

    // Override for forced disconnections
    virtual void handleConnectedServer(ServerID server);
    virtual void handleDisconnectedServer(ServerID server);

    void handleUpdateObjectQuery(const UUID& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results, SeqNoPtr seqno);
    void handleRemoveObjectQuery(const UUID& object, bool notify_main_thread);
    void handleDisconnectedObject(const UUID& object);

    // Generate query events based on results collected from query handlers
    void generateServerQueryEvents(Query* query);
    void generateObjectQueryEvents(Query* query, bool do_first=false);

    // Decides whether a query handler should handle a particular object.
    bool handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const UUID& obj_id, bool local, bool aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize);
    // The real handler for moving objects between static/dynamic
    void handleCheckObjectClassForHandlers(const UUID& objid, bool is_static, ProxQueryHandlerData handlers[NUM_OBJECT_CLASSES]);
    virtual void trySwapHandlers(bool is_local, const UUID& objid, bool is_static);

    /**
       @param {uuid} obj_id The uuid of the object that we're sending proximity
       messages to.

       Gets or creates sequence number information for the given querier.
     */
    SeqNoPtr getOrCreateSeqNoInfo(const ServerID server_id);
    void eraseSeqNoInfo(const ServerID server_id);
    SeqNoPtr getSeqNoInfo(const UUID& obj_id);
    void eraseSeqNoInfo(const UUID& obj_id);

    typedef std::set<UUID> ObjectSet;
    typedef std::tr1::unordered_map<ServerID, Query*> ServerQueryMap;
    typedef std::tr1::unordered_map<Query*, ServerID> InvertedServerQueryMap;
    typedef std::tr1::unordered_map<UUID, Query*, UUID::Hasher> ObjectQueryMap;
    typedef std::tr1::unordered_set<Query*> FirstIterationObjectSet;
    typedef std::tr1::unordered_map<Query*, UUID> InvertedObjectQueryMap;

    typedef std::tr1::shared_ptr<ObjectSet> ObjectSetPtr;
    typedef std::tr1::unordered_map<ServerID, ObjectSetPtr> ServerQueryResultSet;


    // BOTH Threads - thread-safe data

    boost::mutex mServerSetMutex;
    // This tracks the servers we currently have subscriptions with
    ServerSet mServersQueried;
    // And this indicates whether we need to send new requests
    // out to other servers
    ServerSet mNeedServerQueryUpdate;



    // MAIN Thread - Should only be accessed in methods used by the main thread

    // The distance to use when doing range queries instead of solid angle queries.
    // FIXME we should have this configurable somehow
    float32 mDistanceQueryDistance;

    // Tracks object query angles for quick access in the main thread
    // NOTE: It really sucks that we're duplicating this information
    // but we'd have to provide a safe query map and query angle accessor
    // to avoid this.
    typedef std::map<UUID, SolidAngle> ObjectQueryAngleMap;
    ObjectQueryAngleMap mObjectQueryAngles;
    typedef std::map<UUID, uint32> ObjectQueryMaxCountMap;
    ObjectQueryMaxCountMap mObjectQueryMaxCounts;


    // Aggregate query info. Aggregate object stats are managed by
    // LibproxProximityBase.
    // This tracks the minimum object query size, which is used
    // as the angle for queries to other servers.
    SolidAngle mMinObjectQueryAngle;
    // And similarly, this tracks the maximum max_count query parameters as
    // conservative estimate of number of results needed from other servers.
    uint32 mMaxMaxCount;


    std::deque<Message*> mServerResultsToSend; // server query results waiting to be sent
    std::deque<Sirikata::Protocol::Object::ObjectMessage*> mObjectResultsToSend; // object query results waiting to be sent



    // PROX Thread - Should only be accessed in methods used by the prox thread

    void tickQueryHandler(ProxQueryHandlerData qh[NUM_OBJECT_CLASSES]);
    void rebuildHandlerType(ProxQueryHandlerData* handler, ObjectClass objtype);
    void rebuildHandler(ObjectClass objtype);

    // Command handlers
    virtual void commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    bool parseHandlerName(const String& name, ProxQueryHandlerData** handlers_out, ObjectClass* class_out);
    virtual void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

    typedef std::tr1::unordered_set<UUID, UUID::Hasher> ObjectIDSet;
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
    // These track local objects and answer queries from other
    // servers.
    ServerQueryMap mServerQueries[NUM_OBJECT_CLASSES];
    InvertedServerQueryMap mInvertedServerQueries;
    ProxQueryHandlerData mServerQueryHandler[NUM_OBJECT_CLASSES];
    bool mServerDistance; // Using distance queries
    // Results from queries to other servers, so we know what we need to remove
    // on forceful disconnection
    ServerQueryResultSet mServerQueryResults;
    PollerService mServerHandlerPoller;

    // These track all objects being reported to this server and
    // answer queries for objects connected to this server.
    ObjectQueryMap mObjectQueries[NUM_OBJECT_CLASSES];
    InvertedObjectQueryMap mInvertedObjectQueries;
    FirstIterationObjectSet mObjectQueriesFirstIteration;
    ProxQueryHandlerData mObjectQueryHandler[NUM_OBJECT_CLASSES];
    bool mObjectDistance; // Using distance queries
    PollerService mObjectHandlerPoller;

    // Pollers that trigger rebuilding of query data structures
    PollerService mStaticRebuilderPoller;
    PollerService mDynamicRebuilderPoller;

    // Track SeqNo info for each querier
    typedef std::tr1::unordered_map<ServerID, SeqNoPtr> ServerSeqNoInfoMap;
    ServerSeqNoInfoMap mServerSeqNos;
    typedef std::tr1::unordered_map<UUID, SeqNoPtr, UUID::Hasher> ObjectSeqNoInfoMap;
    ObjectSeqNoInfoMap mObjectSeqNos;


    // Threads: Thread-safe data used for exchange between threads
    Sirikata::ThreadSafeQueue<Message*> mServerResults; // server query results that need to be sent
    Sirikata::ThreadSafeQueue<Sirikata::Protocol::Object::ObjectMessage*> mObjectResults; // object query results that need to be sent

}; //class LibproxProximity

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_PROXIMITY_HPP_
