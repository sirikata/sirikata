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

namespace Sirikata {

/** Implementation of Proximity using manual traversal of the object tree. The
 *  Proximity library provides the tree and a simple interface to manage cuts and
 *  the client manages the cut.
 */
class LibproxManualProximity :
        public LibproxProximityBase,
        Prox::AggregateListener<ObjectProxSimulationTraits>,
        Prox::QueryEventListener<ObjectProxSimulationTraits, Prox::ManualQuery<ObjectProxSimulationTraits> >
{
private:
    typedef Prox::ManualQueryHandler<ObjectProxSimulationTraits> ProxQueryHandler;
    typedef Prox::Aggregator<ObjectProxSimulationTraits> ProxAggregator;
    typedef Prox::ManualQuery<ObjectProxSimulationTraits> ProxQuery;
    typedef Prox::QueryEvent<ObjectProxSimulationTraits> ProxQueryEvent;

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
    virtual void aggregateCreated(ProxAggregator* handler, const UUID& objid);
    virtual void aggregateChildAdded(ProxAggregator* handler, const UUID& objid, const UUID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateChildRemoved(ProxAggregator* handler, const UUID& objid, const UUID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateBoundsUpdated(ProxAggregator* handler, const UUID& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size);
    virtual void aggregateDestroyed(ProxAggregator* handler, const UUID& objid);
    virtual void aggregateObserved(ProxAggregator* handler, const UUID& objid, uint32 nobservers);

    // QueryEventListener Interface
    void queryHasEvents(ProxQuery* query);

private:
    struct ProxQueryHandlerData;

    // MAIN Thread:

    virtual int32 objectHostQueries() const;

    // ObjectHost message management
    void handleObjectHostSubstream(int success, OHDPSST::Stream::Ptr substream, SeqNoPtr seqNo);


    std::deque<OHResult> mOHResultsToSend;

    // PROX Thread:

    void tickQueryHandler(ProxQueryHandlerData qh[NUM_OBJECT_CLASSES]);

    // Real handler for OH requests, in the prox thread
    void handleObjectHostProxMessage(const OHDP::NodeID& id, const String& data, SeqNoPtr seqNo);
    // Real handler for OH disconnects
    void handleObjectHostSessionEnded(const OHDP::NodeID& id);
    void destroyQuery(const OHDP::NodeID& id);

    // Decides whether a query handler should handle a particular object.
    bool handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const UUID& obj_id, bool is_local, bool is_aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize);
    // The real handler for moving objects between static/dynamic
    void handleCheckObjectClassForHandlers(const UUID& objid, bool is_static, ProxQueryHandlerData handlers[NUM_OBJECT_CLASSES]);
    virtual void trySwapHandlers(bool is_local, const UUID& objid, bool is_static);

    SeqNoPtr getSeqNoInfo(const OHDP::NodeID& node);
    void eraseSeqNoInfo(const OHDP::NodeID& node);

    // Command handlers
    virtual void commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    bool parseHandlerName(const String& name, ProxQueryHandlerData** handlers_out, ObjectClass* class_out);
    virtual void commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    virtual void commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

    typedef std::tr1::unordered_map<OHDP::NodeID, ProxQuery*, OHDP::NodeID::Hasher> OHQueryMap;
    typedef std::tr1::unordered_map<ProxQuery*, OHDP::NodeID> InvertedOHQueryMap;

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
