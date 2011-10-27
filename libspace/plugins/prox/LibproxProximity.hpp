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
#include <sirikata/space/ProxSimulationTraits.hpp>
#include <prox/QueryHandler.hpp>
#include <prox/LocationUpdateListener.hpp>
#include <prox/AggregateListener.hpp>

#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>

#include <sirikata/space/PintoServerQuerier.hpp>


namespace Sirikata {

class ProximityInputEvent;
class ProximityOutputEvent;

class LibproxProximity :
        public LibproxProximityBase,
        Prox::QueryEventListener<ObjectProxSimulationTraits>,
        PintoServerQuerierListener,
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

    // Service Interface overrides
    virtual void start();

    // ObjectSessionListener Interface
    virtual void newSession(ObjectSession* session);
    virtual void sessionClosed(ObjectSession* session);

    // Objects
    virtual void addQuery(UUID obj, SolidAngle sa, uint32 max_results);
    virtual void addQuery(UUID obj, const String& params);
    virtual void removeQuery(UUID obj);

    // QueryEventListener Interface
    void queryHasEvents(Query* query);

    // LocationServiceListener Interface
    virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);

    // CoordinateSegmentation::Listener Interface
    virtual void updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg);

    // MessageRecipient Interface
    virtual void receiveMessage(Message* msg);

    // MigrationDataClient Interface
    virtual std::string migrationClientTag();
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server);
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data);

    // PintoServerQuerierListener Interface
    virtual void addRelevantServer(ServerID sid);
    virtual void removeRelevantServer(ServerID sid);

    // AggregateListener Interface
    virtual void aggregateCreated(ProxAggregator* handler, const UUID& objid);
    virtual void aggregateChildAdded(ProxAggregator* handler, const UUID& objid, const UUID& child, const BoundingSphere3f& bnds);
    virtual void aggregateChildRemoved(ProxAggregator* handler, const UUID& objid, const UUID& child, const BoundingSphere3f& bnds);
    virtual void aggregateBoundsUpdated(ProxAggregator* handler, const UUID& objid, const BoundingSphere3f& bnds);
    virtual void aggregateDestroyed(ProxAggregator* handler, const UUID& objid);
    virtual void aggregateObserved(ProxAggregator* handler, const UUID& objid, uint32 nobservers);

    // SpaceNetworkConnectionListener Interface
    virtual void onSpaceNetworkConnected(ServerID sid);
    virtual void onSpaceNetworkDisconnected(ServerID sid);


private:
    enum ObjectClass {
        OBJECT_CLASS_STATIC = 0,
        OBJECT_CLASS_DYNAMIC = 1,
        NUM_OBJECT_CLASSES = 2
    };
    static const std::string& ObjectClassToString(ObjectClass c);






    void handleObjectProximityMessage(const UUID& objid, void* buffer, uint32 length);

    void updateAggregateLoc(const UUID& objid, const BoundingSphere3f& bnds);

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

    // Object sizes
    void updateObjectSize(const UUID& obj, float rad);
    void removeObjectSize(const UUID& obj);

    // Takes care of switching objects between static/dynamic
    void checkObjectClass(bool is_local, const UUID& objid, const TimedMotionVector3f& newval);

    // Setup all known servers for a server query update
    void addAllServersForUpdate();

    // Send a query add/update request to all the other servers
    void sendQueryRequests();


    // Handle various events in the main thread that are triggered in the prox thread
    void handleAddObjectLocSubscription(const UUID& subscriber, const UUID& observed);
    void handleRemoveObjectLocSubscription(const UUID& subscriber, const UUID& observed);
    void handleRemoveAllObjectLocSubscription(const UUID& subscriber);
    void handleAddServerLocSubscription(const ServerID& subscriber, const UUID& observed, SeqNoPtr seqPtr);
    void handleRemoveServerLocSubscription(const ServerID& subscriber, const UUID& observed);
    void handleRemoveAllServerLocSubscription(const ServerID& subscriber);


    // PROX Thread: These are utility methods which should only be called from the prox thread.

    // Handle various query events from the main thread
    void handleUpdateServerQuery(const ServerID& server, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results);
    void handleRemoveServerQuery(const ServerID& server);
    void handleConnectedServer(ServerID sid);
    void handleDisconnectedServer(ServerID sid);

    void handleUpdateObjectQuery(const UUID& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results, SeqNoPtr seqno);
    void handleRemoveObjectQuery(const UUID& object, bool notify_main_thread);
    void handleDisconnectedObject(const UUID& object);

    // Generate query events based on results collected from query handlers
    void generateServerQueryEvents(Query* query);
    void generateObjectQueryEvents(Query* query);

    // Decides whether a query handler should handle a particular object.
    bool handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const UUID& obj_id, bool local, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize);
    // The real handler for moving objects between static/dynamic
    void handleCheckObjectClass(bool is_local, const UUID& objid, const TimedMotionVector3f& newval);
    void handleCheckObjectClassForHandlers(const UUID& objid, bool is_static, ProxQueryHandler* handlers[NUM_OBJECT_CLASSES]);

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
    typedef std::tr1::unordered_map<Query*, UUID> InvertedObjectQueryMap;

    typedef std::tr1::shared_ptr<ObjectSet> ObjectSetPtr;
    typedef std::tr1::unordered_map<ServerID, ObjectSetPtr> ServerQueryResultSet;


    PintoServerQuerier* mServerQuerier;

    // To support a static/dynamic split but also support mixing them for
    // comparison purposes track which we are doing and, for most places, use a
    // simple index to control whether they point to different query handlers or
    // the same one.
    bool mSeparateDynamicObjects;
    int mNumQueryHandlers;

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

    // Track object sizes and the maximum of all of them.
    typedef std::tr1::unordered_map<UUID, float32, UUID::Hasher> ObjectSizeMap;
    ObjectSizeMap mObjectSizes;
    float32 mMaxObject;

    // This tracks the minimum object query size, which is used
    // as the angle for queries to other servers.
    SolidAngle mMinObjectQueryAngle;
    // And similarly, this tracks the maximum max_count query parameters as
    // conservative estimate of number of results needed from other servers.
    uint32 mMaxMaxCount;

    typedef std::tr1::unordered_set<ServerID> ServerSet;
    boost::mutex mServerSetMutex;
    // This tracks the servers we currently have subscriptions with
    ServerSet mServersQueried;
    // And this indicates whether we need to send new requests
    // out to other servers
    ServerSet mNeedServerQueryUpdate;

    std::deque<Message*> mServerResultsToSend; // server query results waiting to be sent
    std::deque<Sirikata::Protocol::Object::ObjectMessage*> mObjectResultsToSend; // object query results waiting to be sent



    // PROX Thread - Should only be accessed in methods used by the prox thread

    void tickQueryHandler(ProxQueryHandler* qh[NUM_OBJECT_CLASSES]);
    void rebuildHandler(ObjectClass objtype);

    // These track local objects and answer queries from other
    // servers.
    ServerQueryMap mServerQueries[NUM_OBJECT_CLASSES];
    InvertedServerQueryMap mInvertedServerQueries;
    ProxQueryHandler* mServerQueryHandler[NUM_OBJECT_CLASSES];
    bool mServerDistance; // Using distance queries
    // Results from queries to other servers, so we know what we need to remove
    // on forceful disconnection
    ServerQueryResultSet mServerQueryResults;
    Poller mServerHandlerPoller;

    // These track all objects being reported to this server and
    // answer queries for objects connected to this server.
    ObjectQueryMap mObjectQueries[NUM_OBJECT_CLASSES];
    InvertedObjectQueryMap mInvertedObjectQueries;
    ProxQueryHandler* mObjectQueryHandler[NUM_OBJECT_CLASSES];
    bool mObjectDistance; // Using distance queries
    Poller mObjectHandlerPoller;

    // Pollers that trigger rebuilding of query data structures
    Poller mStaticRebuilderPoller;
    Poller mDynamicRebuilderPoller;

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
