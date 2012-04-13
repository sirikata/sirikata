// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MQ_OBJECT_QUERY_HANDLER_HPP_
#define _SIRIKATA_OH_MQ_OBJECT_QUERY_HANDLER_HPP_

#include "ObjectQueryHandlerBase.hpp"
#include "ProxSimulationTraits.hpp"
#include "OHLocationUpdateListener.hpp"
#include <prox/geom/QueryHandler.hpp>
#include <sirikata/core/queue/ThreadSafeQueueWithNotification.hpp>
#include <sirikata/core/service/PollerService.hpp>
#include <sirikata/oh/HostedObject.hpp>

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
        public OHLocationUpdateListener
{
private:
    typedef Prox::QueryHandler<ObjectProxSimulationTraits> ProxQueryHandler;
public:
    // MAIN Thread: All public interface is expected to be called only from the main thread.
    typedef Prox::Query<ObjectProxSimulationTraits> Query;
    typedef Prox::QueryEvent<ObjectProxSimulationTraits> QueryEvent;

    ObjectQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, const SpaceID& space, Network::IOStrandPtr prox_strand, OHLocationServiceCachePtr loc_cache);
    ~ObjectQueryHandler();

    // MAIN Thread:

    // Service Interface overrides
    virtual void start();
    virtual void stop();

    // Query settings. These pass in the HostedObject so they don't
    // have to rely on the OHLocationServiceCache for implicit
    // settings since it's possible the OHLocationServiceCache hasn't
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

    // OHLocationUpdateListener Interface
    // We override this for two reasons. First, a few callbacks are used to
    // update query parameters (location & bounds). Second, we use these updates
    // to generate LocUpdates for queriers that are subscribed to the object.
    virtual void onObjectAdded(const ObjectReference& obj);
    virtual void onObjectRemoved(const ObjectReference& obj);
    virtual void onParentUpdated(const ObjectReference& obj);
    virtual void onEpochUpdated(const ObjectReference& obj);
    virtual void onLocationUpdated(const ObjectReference& obj);
    virtual void onOrientationUpdated(const ObjectReference& obj);
    virtual void onBoundsUpdated(const ObjectReference& obj);
    virtual void onMeshUpdated(const ObjectReference& obj);
    virtual void onPhysicsUpdated(const ObjectReference& obj);

    // PROX Thread:

    // QueryEventListener Interface
    void queryHasEvents(Query* query);

    ProxQueryHandler* getQueryHandler(const String& handler_name);
    void commandListInfo(const OHDP::SpaceNodeID& snid, Command::Result& result);
    void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

private:

    // MAIN Thread: These are utility methods which should only be called from the main thread.
    int32 objectQueries() const;

    // Update queries based on current state.
    void handleDeliverEvents();

    // Update subscribed queriers that the given object was updated. This
    // currently takes the brute force approach of updating all properties and
    // uses the most up-to-date info, even if that's actually newer than the
    // update that triggered this.
    void handleNotifySubscribersLocUpdate(const ObjectReference& oref);

    // Object queries
    void updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, SolidAngle sa, uint32 max_results);
    void updateQuery(const ObjectReference& obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results);

    // Takes care of switching objects between static/dynamic
    void checkObjectClass(const ObjectReference& objid, const TimedMotionVector3f& newval);



    // PROX Thread: These are utility methods which should only be called from the prox thread.

    void handleUpdateObjectQuery(const ObjectReference& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results);
    void handleRemoveObjectQuery(const ObjectReference& object, bool notify_main_thread);
    void handleDisconnectedObject(const ObjectReference& object);

    // Generate query events based on results collected from query handlers
    void generateObjectQueryEvents(Query* query);

    // Decides whether a query handler should handle a particular object.
    bool handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const ObjectReference& obj_id, bool local, bool aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize);
    // The real handler for moving objects between static/dynamic
    void handleCheckObjectClass(const ObjectReference& objid, const TimedMotionVector3f& newval);
    void handleCheckObjectClassForHandlers(const ObjectReference& objid, bool is_static, ProxQueryHandler* handlers[NUM_OBJECT_CLASSES]);

    typedef std::set<ObjectReference> ObjectSet;
    typedef std::tr1::unordered_map<ObjectReference, Query*, ObjectReference::Hasher> ObjectQueryMap;
    typedef std::tr1::unordered_map<Query*, ObjectReference> InvertedObjectQueryMap;

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

    void tickQueryHandler(ProxQueryHandler* qh[NUM_OBJECT_CLASSES]);

    // These track all objects being reported to this server and
    // answer queries for objects connected to this server.
    ObjectQueryMap mObjectQueries[NUM_OBJECT_CLASSES];
    InvertedObjectQueryMap mInvertedObjectQueries;
    ProxQueryHandler* mObjectQueryHandler[NUM_OBJECT_CLASSES];
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
