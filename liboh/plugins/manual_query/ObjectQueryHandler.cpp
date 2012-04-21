// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ObjectQueryHandler.hpp"
#include "ManualObjectQueryProcessor.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/core/prox/QueryHandlerFactory.hpp>

#include "Protocol_Prox.pbj.hpp"

#include <sirikata/oh/PresencePropertiesLocUpdate.hpp>

#include <json_spirit/json_spirit.h>

#define QPLOG(level,msg) SILOG(manual-query-processor,level,msg)

namespace Sirikata {
namespace OH {
namespace Manual {

static SolidAngle NoUpdateSolidAngle = SolidAngle(0.f);
// Note that this is different from sentinals indicating infinite
// results. Instead, this indicates that we shouldn't even request the update,
// leaving it as is, because we don't actually have a new value.
static uint32 NoUpdateMaxResults = ((uint32)INT_MAX)+1;

namespace {

bool parseQueryRequest(const String& query, SolidAngle* qangle_out, uint32* max_results_out) {
    if (query.empty())
        return false;

    namespace json = json_spirit;
    json::Value parsed_query;
    if (!json::read(query, parsed_query))
        return false;

    *qangle_out = SolidAngle( parsed_query.getReal("angle", SolidAngle::MaxVal) );
    *max_results_out = parsed_query.getInt("max_results", 0);

    return true;
}

}

ObjectQueryHandler::ObjectQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, const OHDP::SpaceNodeID& space, Network::IOStrandPtr prox_strand, OHLocationServiceCachePtr loc_cache)
 : ObjectQueryHandlerBase(ctx, parent, space, prox_strand, loc_cache),
   mObjectQueries(),
   mObjectDistance(false),
   mObjectHandlerPoller(mProxStrand.get(), std::tr1::bind(&ObjectQueryHandler::tickQueryHandler, this, mObjectQueryHandler), "ObjectQueryHandler Poller", Duration::milliseconds((int64)100)),
   mObjectResults( std::tr1::bind(&ObjectQueryHandler::handleDeliverEvents, this) )
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    using std::tr1::placeholders::_4;
    using std::tr1::placeholders::_5;
    using std::tr1::placeholders::_6;

    // Object Queries
    String object_handler_type = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_TYPE);
    String object_handler_options = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_OPTIONS);
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mObjectQueryHandler[i] = NULL;
            continue;
        }
        mObjectQueryHandler[i] = QueryHandlerFactory<ObjectProxSimulationTraits>(object_handler_type, object_handler_options, false);
        // Aggregate listening -- really used to learn about queries
        // observing/no longer observing nodes in the tree so we know when to
        // coarsen/refine
        mObjectQueryHandler[i]->setAggregateListener(this); // *Must* be before handler->initialize
        bool object_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mObjectQueryHandler[i]->initialize(
            mLocCache.get(), mLocCache.get(),
            object_static_objects, true /* replicated */,
            std::tr1::bind(&ObjectQueryHandler::handlerShouldHandleObject, this, object_static_objects, true, _1, _2, _3, _4, _5, _6)
        );
    }
    if (object_handler_type == "dist" || object_handler_type == "rtreedist") mObjectDistance = true;
}

ObjectQueryHandler::~ObjectQueryHandler() {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        delete mObjectQueryHandler[i];
    }
}


// MAIN Thread Methods: The following should only be called from the main thread.

void ObjectQueryHandler::start() {
    mLocCache->addListener(this);

    mObjectHandlerPoller.start();
}

void ObjectQueryHandler::stop() {
    mObjectHandlerPoller.stop();

    mLocCache->removeListener(this);
}

void ObjectQueryHandler::presenceConnected(const ObjectReference& objid) {
}

void ObjectQueryHandler::presenceDisconnected(const ObjectReference& objid) {
    // Prox strand may  have some state to clean up
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleDisconnectedObject, this, objid),
        "ObjectQueryHandler::handleDisconnectedObject"
    );
}

void ObjectQueryHandler::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& obj, const String& params) {
    SolidAngle sa;
    uint32 max_results;
    if (parseQueryRequest(params, &sa, &max_results))
        updateQuery(ho, obj, sa, max_results);
}

void ObjectQueryHandler::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& obj, SolidAngle sa, uint32 max_results) {
    // We don't use mLocCache here becuase the querier might not be in
    // there yet
    SequencedPresencePropertiesPtr querier_props = ho->presenceRequestedLocation(obj);
    TimedMotionVector3f loc = querier_props->location();
    AggregateBoundingInfo bounds = querier_props->bounds();
    assert(bounds.singleObject());
    updateQuery(obj.object(), loc, bounds.fullBounds(), sa, max_results);
}

void ObjectQueryHandler::updateQuery(const ObjectReference& obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results) {
    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleUpdateObjectQuery, this, obj, loc, bounds, sa, max_results),
        "ObjectQueryHandler::handleUpdateObjectQuery"
    );
}

void ObjectQueryHandler::removeQuery(HostedObjectPtr ho, const SpaceObjectReference& obj) {
    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleRemoveObjectQuery, this, obj.object(), true),
        "ObjectQueryHandler::handleRemoveObjectQuery"
    );
}

void ObjectQueryHandler::checkObjectClass(const ObjectReference& objid, const TimedMotionVector3f& newval) {
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleCheckObjectClass, this, objid, newval),
        "ObjectQueryHandler::handleCheckObjectClass"
    );
}

int32 ObjectQueryHandler::objectQueries() const {
    return mObjectQueries[OBJECT_CLASS_STATIC].size();
}


void ObjectQueryHandler::handleDeliverEvents() {
    // Get and ship object results
    std::deque<ProximityResultInfo> object_results_copy;
    mObjectResults.swap(object_results_copy);

    while(!object_results_copy.empty()) {
        ProximityResultInfo info = object_results_copy.front();
        object_results_copy.pop_front();

        // Deal with subscriptions
        for(int aidx = 0; aidx < info.results->addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = info.results->addition(aidx);
            ObjectReference viewed(addition.object());
            if (!mSubscribers[viewed]) mSubscribers[viewed] = SubscriberSetPtr(new SubscriberSet());
            if (mSubscribers[viewed]->find(info.querier) == mSubscribers[viewed]->end())
                mSubscribers[viewed]->insert(info.querier);
        }
        for(int ridx = 0; ridx < info.results->removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = info.results->removal(ridx);
            ObjectReference viewed(removal.object());
            if (mSubscribers.find(viewed) == mSubscribers.end()) continue;
            if (mSubscribers[viewed]->find(info.querier) != mSubscribers[viewed]->end()) {
                mSubscribers[viewed]->erase(info.querier);
                if (mSubscribers[viewed]->empty()) mSubscribers.erase(viewed);
            }
        }

        // And deliver the results
        mParent->deliverProximityResult(SpaceObjectReference(mSpaceNodeID.space(), info.querier), *(info.results));
        delete info.results;
    }
}

void ObjectQueryHandler::handleNotifySubscribersLocUpdate(const ObjectReference& oref) {
    SubscribersMap::iterator it = mSubscribers.find(oref);
    if (it == mSubscribers.end()) return;
    SubscriberSetPtr subscribers = it->second;

    // Make sure the object is still available and remains available
    // throughout all updates
    if (mLocCache->startSimpleTracking(oref)) {
        PresencePropertiesLocUpdate lu( oref, mLocCache->properties(oref) );

        for(SubscriberSet::iterator sub_it = subscribers->begin(); sub_it != subscribers->end(); sub_it++) {
            const ObjectReference& querier = *sub_it;

            // If we're delivering a result to ourselves, we want to include
            // epoch information.
            if (querier == oref) {
                PresencePropertiesLocUpdateWithEpoch lu_ep( oref, mLocCache->properties(oref), true, mLocCache->epoch(oref) );
                mParent->deliverLocationResult(SpaceObjectReference(mSpaceNodeID.space(), querier), lu_ep);
            }
            else {
                mParent->deliverLocationResult(SpaceObjectReference(mSpaceNodeID.space(), querier), lu);
            }
        }

        mLocCache->stopSimpleTracking(oref);
    }
}



void ObjectQueryHandler::queryHasEvents(Query* query) {
    generateObjectQueryEvents(query);
}

// AggregateListener Interface
// This class only uses this information to track observed nodes so we can
// let the parent class know when to refine/coarsen the query with the server.
void ObjectQueryHandler::aggregateCreated(ProxAggregator* handler, const ObjectReference& objid) {}

void ObjectQueryHandler::aggregateChildAdded(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {}

void ObjectQueryHandler::aggregateChildRemoved(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {}

void ObjectQueryHandler::aggregateBoundsUpdated(ProxAggregator* handler, const ObjectReference& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {}

void ObjectQueryHandler::aggregateDestroyed(ProxAggregator* handler, const ObjectReference& objid) {}

void ObjectQueryHandler::aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers) {
    // We care about nodes being observed (i.e. having cuts through them)
    // because it means that somebody local still cares about them, so we should
    // try to keep it around as long as the server lets us. The two events we
    // care about is when the number of observers becomes 0 (all observers have
    // left) or 1 (we have an observer, so we start trying to keep it around).
    //
    // Note that we don't have to post anything here currently because we should
    // be in the same strand as the parent
    if (nobservers == 1) {
        mParent->queriersAreObserving(mSpaceNodeID, objid);
    }
    else if (nobservers == 0) {
        mParent->queriersStoppedObserving(mSpaceNodeID, objid);
    }
}


ObjectQueryHandler::ProxQueryHandler* ObjectQueryHandler::getQueryHandler(const String& handler_name) {
    if (handler_name.find("object-queries.static-objects") != String::npos)
        return mObjectQueryHandler[OBJECT_CLASS_STATIC];
    if (handler_name.find("object-queries.dynamic-objects") != String::npos)
        return mObjectQueryHandler[OBJECT_CLASS_DYNAMIC];

    return NULL;
}

void ObjectQueryHandler::commandListInfo(const OHDP::SpaceNodeID& snid, Command::Result& result) {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i] == NULL) continue;
        String key = String("handlers.object.") + ObjectClassToString((ObjectClass)i) + ".";
        result.put(key + "name", snid.toString() + "." + String("object-queries.") + ObjectClassToString((ObjectClass)i) + "-objects");
        result.put(key + "queries", mObjectQueryHandler[i]->numQueries());
        result.put(key + "objects", mObjectQueryHandler[i]->numObjects());
        result.put(key + "nodes", mObjectQueryHandler[i]->numNodes());
    }
}

void ObjectQueryHandler::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    String handler_name = cmd.getString("handler");
    ProxQueryHandler* handler = getQueryHandler(handler_name);
    if (handler == NULL) {
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
    }

    result.put( String("nodes"), Command::Array());
    Command::Array& nodes_ary = result.getArray("nodes");
    for(ProxQueryHandler::NodeIterator nit = handler->nodesBegin(); nit != handler->nodesEnd(); nit++) {
        nodes_ary.push_back( Command::Object() );
        nodes_ary.back().put("id", nit.id().toString());
        nodes_ary.back().put("parent", nit.parentId().toString());
        BoundingSphere3f bounds = nit.bounds(mContext->simTime());
        nodes_ary.back().put("bounds.center.x", bounds.center().x);
        nodes_ary.back().put("bounds.center.y", bounds.center().y);
        nodes_ary.back().put("bounds.center.z", bounds.center().z);
        nodes_ary.back().put("bounds.radius", bounds.radius());
        nodes_ary.back().put("cuts", nit.cuts());
    }

    cmdr->result(cmdid, result);
}

void ObjectQueryHandler::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put("error", "Rebuilding manual proximity processors doesn't make sense, they use replicated trees.");
    cmdr->result(cmdid, result);
}



void ObjectQueryHandler::onObjectAdded(const ObjectReference& obj) {
}

void ObjectQueryHandler::onObjectRemoved(const ObjectReference& obj) {
}

void ObjectQueryHandler::onParentUpdated(const ObjectReference& obj) {
}

void ObjectQueryHandler::onEpochUpdated(const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onLocationUpdated(const ObjectReference& obj) {
    updateQuery(obj, mLocCache->location(obj), mLocCache->bounds(obj).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults);
    if (mSeparateDynamicObjects)
        checkObjectClass(obj, mLocCache->location(obj));

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onOrientationUpdated(const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onBoundsUpdated(const ObjectReference& obj) {
    updateQuery(obj, mLocCache->location(obj), mLocCache->bounds(obj).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults);

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onMeshUpdated(const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onPhysicsUpdated(const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}


// PROX Thread: Everything after this should only be called from within the prox thread.

void ObjectQueryHandler::tickQueryHandler(ProxQueryHandler* qh[NUM_OBJECT_CLASSES]) {
    Time simT = mContext->simTime();
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (qh[i] != NULL)
            qh[i]->tick(simT);
    }
}


void ObjectQueryHandler::generateObjectQueryEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    assert(mInvertedObjectQueries.find(query) != mInvertedObjectQueries.end());
    ObjectReference query_id = mInvertedObjectQueries[query];

    QueryEventList evts;
    query->popEvents(evts);

    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();
        Sirikata::Protocol::Prox::ProximityUpdate* event_results = new Sirikata::Protocol::Prox::ProximityUpdate();

        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            ObjectReference objid = evt.additions()[aidx].id();
            if (mLocCache->startSimpleTracking(objid)) { // If the cache already lost it, we can't do anything

                mContext->mainStrand->post(
                    std::tr1::bind(&ObjectQueryHandler::handleAddObjectLocSubscription, this, query_id, objid),
                    "ObjectQueryHandler::handleAddObjectLocSubscription"
                );

                Sirikata::Protocol::Prox::IObjectAddition addition = event_results->add_addition();
                addition.set_object( objid.getAsUUID() );

                uint64 seqNo = mLocCache->properties(objid).maxSeqNo();
                addition.set_seqno (seqNo);


                Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
                TimedMotionVector3f loc = mLocCache->location(objid);
                motion.set_t(loc.updateTime());
                motion.set_position(loc.position());
                motion.set_velocity(loc.velocity());

                TimedMotionQuaternion orient = mLocCache->orientation(objid);
                Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                msg_orient.set_t(orient.updateTime());
                msg_orient.set_position(orient.position());
                msg_orient.set_velocity(orient.velocity());

                Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
                AggregateBoundingInfo bnds = mLocCache->bounds(objid);
                msg_bounds.set_center_offset(bnds.centerOffset);
                msg_bounds.set_center_bounds_radius(bnds.centerBoundsRadius);
                msg_bounds.set_max_object_size(bnds.maxObjectRadius);

                Transfer::URI mesh = mLocCache->mesh(objid);
                if (!mesh.empty())
                    addition.set_mesh(mesh.toString());
                String phy = mLocCache->physics(objid);
                if (phy.size() > 0)
                    addition.set_physics(phy);

                mLocCache->stopSimpleTracking(objid);
            }
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            ObjectReference objid = evt.removals()[ridx].id();
            // Clear out seqno and let main strand remove loc
            // subcription
            mContext->mainStrand->post(
                std::tr1::bind(&ObjectQueryHandler::handleRemoveObjectLocSubscription, this, query_id, objid),
                "ObjectQueryHandler::handleRemoveObjectLocSubscription"
            );

            Sirikata::Protocol::Prox::IObjectRemoval removal = event_results->add_removal();
            removal.set_object( objid.getAsUUID() );

            // It'd be nice if we didn't need this but the seqno might
            // not be available anymore and we still want to send the
            // removal.
            if (mLocCache->startSimpleTracking(objid)) {
                uint64 seqNo = mLocCache->properties(objid).maxSeqNo();
                removal.set_seqno (seqNo);
                mLocCache->stopSimpleTracking(objid);
            }

            removal.set_type(
                (evt.removals()[ridx].permanent() == QueryEvent::Permanent)
                ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                : Sirikata::Protocol::Prox::ObjectRemoval::Transient
            );
        }
        evts.pop_front();

        mObjectResults.push( ProximityResultInfo(query_id, event_results) );
    }
}

void ObjectQueryHandler::handleUpdateObjectQuery(const ObjectReference& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results) {
    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();

    QPLOG(detailed,"Update object query from " << object.toString() << ", min angle " << angle.asFloat() << ", max results " << max_results);

    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i] == NULL) continue;

        ObjectQueryMap::iterator it = mObjectQueries[i].find(object);

        if (it == mObjectQueries[i].end()) {
            // We only add if we actually have all the necessary info, most importantly a real minimum angle.
            // This is necessary because we get this update for all location updates, even those for objects
            // which don't have subscriptions.
            if (angle != NoUpdateSolidAngle) {
                float32 query_dist = 0.f;//TODO(ewencp) enable getting query
                                         //distance from parameters
                Query* q = mObjectDistance ?
                    mObjectQueryHandler[i]->registerQuery(loc, region, ms, SolidAngle::Min, query_dist) :
                    mObjectQueryHandler[i]->registerQuery(loc, region, ms, angle);
                if (max_results != NoUpdateMaxResults && max_results > 0)
                    q->maxResults(max_results);
                mObjectQueries[i][object] = q;
                mInvertedObjectQueries[q] = object;
                q->setEventListener(this);
            }
        }
        else {
            Query* query = it->second;
            query->position(loc);
            query->region( region );
            query->maxSize( ms );
            if (angle != NoUpdateSolidAngle)
                query->angle(angle);
            if (max_results != NoUpdateMaxResults && max_results > 0)
                query->maxResults(max_results);
        }
    }
}

void ObjectQueryHandler::handleRemoveObjectQuery(const ObjectReference& object, bool notify_main_thread) {
    // Clear out queries
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i] == NULL) continue;

        ObjectQueryMap::iterator it = mObjectQueries[i].find(object);
        if (it == mObjectQueries[i].end()) continue;

        Query* q = it->second;
        mObjectQueries[i].erase(it);
        mInvertedObjectQueries.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }

    // Optionally let the main thread know to clear its communication state
    if (notify_main_thread) {
        mContext->mainStrand->post(
            std::tr1::bind(&ObjectQueryHandler::handleRemoveAllObjectLocSubscription, this, object),
            "ObjectQueryHandler::handleRemoveAllObjectLocSubscription"
        );
    }
}

void ObjectQueryHandler::handleDisconnectedObject(const ObjectReference& object) {
    // Clear out query state if it exists
    handleRemoveObjectQuery(object, false);
}

bool ObjectQueryHandler::handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const ObjectReference& obj_id, bool is_local, bool is_aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize) {
    // We just need to decide whether the query handler should handle
    // the object. We need to consider local vs. replica and static
    // vs. dynamic.  All must 'vote' for handling the object for us to
    // say it should be handled, so as soon as we find a negative
    // response we can return false.

    // First classify by local vs. replica. Only say no on a local
    // handler looking at a replica.
    if (!is_local && !is_global_handler) return false;

    // If we're not doing the static/dynamic split, then this is a non-issue
    if (!mSeparateDynamicObjects) return true;

    // If we are splitting them, check velocity against is_static_handler. The
    // value here as arbitrary, just meant to indicate such small movement that
    // the object is effectively
    bool is_static = velocityIsStatic(pos.velocity());
    if ((is_static && is_static_handler) ||
        (!is_static && !is_static_handler))
        return true;
    else
        return false;
}

void ObjectQueryHandler::handleCheckObjectClassForHandlers(const ObjectReference& objid, bool is_static, ProxQueryHandler* handlers[NUM_OBJECT_CLASSES]) {
    if ( (is_static && handlers[OBJECT_CLASS_STATIC]->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_DYNAMIC]->containsObject(objid)) )
        return;

    // Validate that the other handler has the object.
    assert(
        (is_static && handlers[OBJECT_CLASS_DYNAMIC]->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_STATIC]->containsObject(objid))
    );

    // If it wasn't in the right place, switch it.
    int swap_out = is_static ? OBJECT_CLASS_DYNAMIC : OBJECT_CLASS_STATIC;
    int swap_in = is_static ? OBJECT_CLASS_STATIC : OBJECT_CLASS_DYNAMIC;
    QPLOG(debug, "Swapping " << objid.toString() << " from " << ObjectClassToString((ObjectClass)swap_out) << " to " << ObjectClassToString((ObjectClass)swap_in));

    bool agg = mLocCache->aggregate(objid);
    ObjectReference parentid = mLocCache->parent(objid);

    // FIXME parentid could be null because there is no parent
    // information or because this is a root. Need to somehow
    // distinguish between these two. Or only support this with parent
    // info?
    if (agg) {
        handlers[swap_out]->removeNode(objid);
        handlers[swap_in]->addNode(objid, parentid);
    }
    else {
        handlers[swap_out]->removeObject(objid);
        if (parentid == ObjectReference::null())
            handlers[swap_in]->addObject(objid);
        else
            handlers[swap_in]->addObject(objid, parentid);
    }
}

void ObjectQueryHandler::handleCheckObjectClass(const ObjectReference& objid, const TimedMotionVector3f& newval) {
    assert(mSeparateDynamicObjects == true);

    // Basic approach: we need to check if the object has switched between
    // static/dynamic. We need to do this for both the local (object query) and
    // global (server query) handlers.
    bool is_static = velocityIsStatic(newval.velocity());
    handleCheckObjectClassForHandlers(objid, is_static, mObjectQueryHandler);
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
