// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ObjectQueryHandler.hpp"
#include "ManualObjectQueryProcessor.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/core/prox/QueryHandlerFactory.hpp>

#include "Protocol_Prox.pbj.hpp"

#include <sirikata/pintoloc/PresencePropertiesLocUpdate.hpp>

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

ObjectQueryHandler::ObjectQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, const OHDP::SpaceNodeID& space, Network::IOStrandPtr prox_strand)
 : ObjectQueryHandlerBase(ctx, parent, space, prox_strand),
   mObjectQueries(),
   mObjectDistance(false),
   mObjectHandlerPoller(mProxStrand.get(), std::tr1::bind(&ObjectQueryHandler::tickQueryHandler, this), "ObjectQueryHandler Poller", Duration::milliseconds((int64)100)),
   mObjectResults( std::tr1::bind(&ObjectQueryHandler::handleDeliverEvents, this) )
{
    String object_handler_type = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_TYPE);
    if (object_handler_type == "dist" || object_handler_type == "rtreedist") mObjectDistance = true;
}

ObjectQueryHandler::~ObjectQueryHandler() {
    for(ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++)
        delete it->second.handler;
    mObjectQueryHandlers.clear();
    mInverseObjectQueryHandlers.clear();
}


// MAIN Thread Methods: The following should only be called from the main thread.

void ObjectQueryHandler::start() {
    mObjectHandlerPoller.start();
}

void ObjectQueryHandler::stop() {
    mObjectHandlerPoller.stop();
}


void ObjectQueryHandler::createdReplicatedIndex(ProxIndexID iid, ReplicatedLocationServiceCachePtr loc_cache, ServerID objects_from_server, bool dynamic_objects) {
    // Register for loc_cache updates immediately to make sure we don't miss any
    // while waiting for the real event handler to run. They'll just queue up
    // behind that handler.
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleCreatedReplicatedIndex, this, iid, loc_cache, objects_from_server, dynamic_objects),
        "ObjectQueryHandler::handleCreatedReplicatedIndex"
    );
    loc_cache->addListener(this);
}

void ObjectQueryHandler::removedReplicatedIndex(ProxIndexID iid) {
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleRemovedReplicatedIndex, this, iid),
        "ObjectQueryHandler::handleRemovedReplicatedIndex"
    );
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
    // We don't use loc caches here becuase the querier might not be in
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

int32 ObjectQueryHandler::objectQueries() const {
    return mObjectQueries.size();
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

void ObjectQueryHandler::handleNotifySubscribersLocUpdate(ReplicatedLocationServiceCache* loccache, const ObjectReference& oref) {
    SubscribersMap::iterator it = mSubscribers.find(oref);
    if (it == mSubscribers.end()) return;
    SubscriberSetPtr subscribers = it->second;

    // Make sure the object is still available and remains available
    // throughout all updates
    if (loccache->startSimpleTracking(oref)) {
        PresencePropertiesLocUpdate lu( oref, loccache->properties(oref) );

        for(SubscriberSet::iterator sub_it = subscribers->begin(); sub_it != subscribers->end(); sub_it++) {
            const ObjectReference& querier = *sub_it;

            // If we're delivering a result to ourselves, we want to include
            // epoch information.
            if (querier == oref) {
                PresencePropertiesLocUpdateWithEpoch lu_ep( oref, loccache->properties(oref), true, loccache->epoch(oref) );
                mParent->deliverLocationResult(SpaceObjectReference(mSpaceNodeID.space(), querier), lu_ep);
            }
            else {
                mParent->deliverLocationResult(SpaceObjectReference(mSpaceNodeID.space(), querier), lu);
            }
        }

        loccache->stopSimpleTracking(oref);
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

    assert(mInverseObjectQueryHandlers.find(handler) != mInverseObjectQueryHandlers.end());
    ProxIndexID indexid = mInverseObjectQueryHandlers[handler];
    if (nobservers == 1) {
        mParent->queriersAreObserving(mSpaceNodeID, indexid, objid);
    }
    else if (nobservers == 0) {
        mParent->queriersStoppedObserving(mSpaceNodeID, indexid, objid);
    }
}


ObjectQueryHandler::ProxQueryHandler* ObjectQueryHandler::getQueryHandler(const String& handler_name) {
    // We really just need to extract the ProxIndexID from the handler name of
    // the form "snid.object-queries.fromserver-indexid-staticdynamic", e.g.
    // "12345678-1111-1111-1111-defa01759ace:1.object-queries.0-5.static-objects"

    // Should be of the form xxx-queries.yyy-objects, containing only 1 .
    std::size_t dot_pos = handler_name.find('.');
    if (dot_pos == String::npos) return NULL;
    std::size_t dot2_pos = handler_name.find('.', dot_pos+1);
    if (dot2_pos == String::npos) return NULL;
    std::size_t dot3_pos = handler_name.find('.', dot2_pos+1);
    if (dot3_pos == String::npos) return NULL;

    //String snid_part = handler_name.substr(0, dot_pos);
    //String query_type_part = name.substr(dot_pos+1, dot2_pos-dot_pos);
    String tree_id_part = handler_name.substr(dot2_pos+1, dot3_pos-dot2_pos-1);
    // Now, within the tree part, after the '-' is the ProxIndexID. We don't
    // need the other part becuase within this ObjectQueryHandler it is the same
    // for all handlers
    std::size_t dash_pos = tree_id_part.find('-');
    if (dash_pos == String::npos) return NULL;

    String iid_part = tree_id_part.substr(dash_pos+1);

    ProxIndexID iid = boost::lexical_cast<ProxIndexID>(iid_part);
    ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.find(iid);
    if (it != mObjectQueryHandlers.end())
        return it->second.handler;
    return NULL;
}

void ObjectQueryHandler::commandListInfo(const OHDP::SpaceNodeID& snid, Command::Result& result) {
    for(ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++) {
        String key = String("handlers.object.") + boost::lexical_cast<String>(it->first) + ".";
        result.put(key + "name",
            // Server we're communicating wih, performing object queries
            snid.toString() + "." + String("object-queries.") +
            // The server the tree is replicated from - the index ID - static/dynamic
            boost::lexical_cast<String>(it->second.from) + "-" + boost::lexical_cast<String>(it->first) + "."+ (it->second.dynamic ? "dynamic" : "static") +
            "-objects");
        result.put(key + "queries", it->second.handler->numQueries());
        result.put(key + "objects", it->second.handler->numObjects());
        result.put(key + "nodes", it->second.handler->numNodes());
    }
}

void ObjectQueryHandler::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    String handler_name = cmd.getString("handler");
    ProxQueryHandler* handler = getQueryHandler(handler_name);
    if (handler == NULL) {
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
        return;
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



void ObjectQueryHandler::onObjectAdded(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
}

void ObjectQueryHandler::onObjectRemoved(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
}

void ObjectQueryHandler::onParentUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
}

void ObjectQueryHandler::onEpochUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onLocationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    updateQuery(obj, loccache->location(obj), loccache->bounds(obj).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults);

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onOrientationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onBoundsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    updateQuery(obj, loccache->location(obj), loccache->bounds(obj).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults);

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onMeshUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onPhysicsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}


// PROX Thread: Everything after this should only be called from within the prox thread.

void ObjectQueryHandler::handleCreatedReplicatedIndex(ProxIndexID iid, ReplicatedLocationServiceCachePtr loc_cache, ServerID objects_from_server, bool dynamic_objects) {
    assert(mObjectQueryHandlers.find(iid) == mObjectQueryHandlers.end());

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    using std::tr1::placeholders::_4;
    using std::tr1::placeholders::_5;
    using std::tr1::placeholders::_6;


    String object_handler_type = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_TYPE);
    String object_handler_options = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_OPTIONS);

    ProxQueryHandler* handler = QueryHandlerFactory<ObjectProxSimulationTraits>(object_handler_type, object_handler_options, false);
    // Aggregate listening -- really used to learn about queries
    // observing/no longer observing nodes in the tree so we know when to
    // coarsen/refine
    handler->setAggregateListener(this); // *Must* be before handler->initialize
    handler->initialize(
        loc_cache.get(), loc_cache.get(),
        !dynamic_objects, true /* replicated */
    );

    mObjectQueryHandlers[iid] = ReplicatedIndexQueryHandler(handler, loc_cache, objects_from_server, dynamic_objects);
    mInverseObjectQueryHandlers[handler] = iid;

    // There may already be queries to which this is relevant.
    // FIXME(ewencp) See the note in handleUpdateObjectQuery about all queries
    // getting registered with all indices. This needs to be smarter
    // eventually.
    for(ObjectQueryMap::iterator query_it = mObjectQueries.begin(); query_it != mObjectQueries.end(); query_it++) {
        ObjectReference objid = query_it->first;
        ObjectQueryDataPtr query_props = query_it->second;
        registerObjectQueryWithIndex(
            objid, iid, handler,
            // FIXME is loccache safe here? what if results for the object
            // haven't come in yet?
            query_props->loc, query_props->bounds,
            query_props->angle, query_props->max_results
        );
    }
}

void ObjectQueryHandler::handleRemovedReplicatedIndex(ProxIndexID iid) {
    assert(mObjectQueryHandlers.find(iid) != mObjectQueryHandlers.end());
    assert(mInverseObjectQueryHandlers.find( mObjectQueryHandlers[iid].handler ) != mInverseObjectQueryHandlers.end());

    mInverseObjectQueryHandlers.erase( mObjectQueryHandlers[iid].handler );

    mObjectQueryHandlers[iid].loccache->removeListener(this);
    delete mObjectQueryHandlers[iid].handler;
    mObjectQueryHandlers.erase(iid);
}


void ObjectQueryHandler::tickQueryHandler() {
    Time simT = mContext->simTime();

    for(ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++)
        it->second.handler->tick(simT);
}


void ObjectQueryHandler::generateObjectQueryEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    assert(mInvertedObjectQueries.find(query) != mInvertedObjectQueries.end());
    ObjectIndexQueryKey query_id = mInvertedObjectQueries[query];
    ObjectReference querier_id = query_id.first;
    ProxIndexID index_id = query_id.second;
    assert(mObjectQueryHandlers.find(index_id) != mObjectQueryHandlers.end());
    ReplicatedIndexQueryHandler& handler_data = mObjectQueryHandlers[index_id];

    QueryEventList evts;
    query->popEvents(evts);

    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();
        Sirikata::Protocol::Prox::ProximityUpdate* event_results = new Sirikata::Protocol::Prox::ProximityUpdate();

        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            ObjectReference objid = evt.additions()[aidx].id();
            if (handler_data.loccache->startSimpleTracking(objid)) { // If the cache already lost it, we can't do anything

                mContext->mainStrand->post(
                    std::tr1::bind(&ObjectQueryHandler::handleAddObjectLocSubscription, this, querier_id, objid),
                    "ObjectQueryHandler::handleAddObjectLocSubscription"
                );

                Sirikata::Protocol::Prox::IObjectAddition addition = event_results->add_addition();
                addition.set_object( objid.getAsUUID() );

                uint64 seqNo = handler_data.loccache->properties(objid).maxSeqNo();
                addition.set_seqno (seqNo);


                Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
                TimedMotionVector3f loc = handler_data.loccache->location(objid);
                motion.set_t(loc.updateTime());
                motion.set_position(loc.position());
                motion.set_velocity(loc.velocity());

                TimedMotionQuaternion orient = handler_data.loccache->orientation(objid);
                Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                msg_orient.set_t(orient.updateTime());
                msg_orient.set_position(orient.position());
                msg_orient.set_velocity(orient.velocity());

                Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
                AggregateBoundingInfo bnds = handler_data.loccache->bounds(objid);
                msg_bounds.set_center_offset(bnds.centerOffset);
                msg_bounds.set_center_bounds_radius(bnds.centerBoundsRadius);
                msg_bounds.set_max_object_size(bnds.maxObjectRadius);

                Transfer::URI mesh = handler_data.loccache->mesh(objid);
                if (!mesh.empty())
                    addition.set_mesh(mesh.toString());
                String phy = handler_data.loccache->physics(objid);
                if (phy.size() > 0)
                    addition.set_physics(phy);

                handler_data.loccache->stopSimpleTracking(objid);
            }
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            ObjectReference objid = evt.removals()[ridx].id();
            // Clear out seqno and let main strand remove loc
            // subcription
            mContext->mainStrand->post(
                std::tr1::bind(&ObjectQueryHandler::handleRemoveObjectLocSubscription, this, querier_id, objid),
                "ObjectQueryHandler::handleRemoveObjectLocSubscription"
            );

            Sirikata::Protocol::Prox::IObjectRemoval removal = event_results->add_removal();
            removal.set_object( objid.getAsUUID() );

            // It'd be nice if we didn't need this but the seqno might
            // not be available anymore and we still want to send the
            // removal.
            if (handler_data.loccache->startSimpleTracking(objid)) {
                uint64 seqNo = handler_data.loccache->properties(objid).maxSeqNo();
                removal.set_seqno (seqNo);
                handler_data.loccache->stopSimpleTracking(objid);
            }

            removal.set_type(
                (evt.removals()[ridx].permanent() == QueryEvent::Permanent)
                ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                : Sirikata::Protocol::Prox::ObjectRemoval::Transient
            );
        }
        evts.pop_front();

        mObjectResults.push( ProximityResultInfo(querier_id, event_results) );
    }
}

void ObjectQueryHandler::registerObjectQueryWithIndex(const ObjectReference& object, ProxIndexID index_id, ProxQueryHandler* handler, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results) {
    // We only add if we actually have all the necessary info, most importantly a real minimum angle.
    // This is necessary because we get this update for all location updates, even those for objects
    // which don't have subscriptions.
    if (angle == NoUpdateSolidAngle) return;

    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();
    ObjectQueryDataPtr query_data = mObjectQueries[object];

    float32 query_dist = 0.f;//TODO(ewencp) enable getting query
                             //distance from parameters
    Query* q = mObjectDistance ?
        handler->registerQuery(loc, region, ms, SolidAngle::Min, query_dist) :
        handler->registerQuery(loc, region, ms, angle);
    if (max_results != NoUpdateMaxResults && max_results > 0)
        q->maxResults(max_results);
    query_data->queries[index_id] = q;
    mInvertedObjectQueries[q] = std::make_pair(object, index_id);
    q->setEventListener(this);
}

void ObjectQueryHandler::handleUpdateObjectQuery(const ObjectReference& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results) {
    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();

    // In some cases (new queries) we only want to do any work here,
    // including reporting the update, if the user explicitly made a
    // change to query parameters since this will get called for all
    // objects triggering movement.
    bool explicit_query_params_update = ((angle != NoUpdateSolidAngle) || (max_results != NoUpdateMaxResults));

    if (mObjectQueries.find(object) == mObjectQueries.end()) {
        // If there's no existing query, so this was just because of a
        // location update -- don't record a query since it wouldn't
        // do anything anyway.
        if (!explicit_query_params_update) return;

        ObjectQueryDataPtr query_props(new ObjectQueryData());
        query_props->loc = loc;
        query_props->bounds = bounds;
        query_props->angle = angle;
        query_props->max_results = max_results;
        mObjectQueries[object] = query_props;
    }
    ObjectQueryDataPtr query_data = mObjectQueries[object];

    // Log, but only if this isn't just due to object movement
    if (explicit_query_params_update)
        QPLOG(detailed,"Update object query from " << object.toString() << ", min angle " << angle.asFloat() << ", max results " << max_results);

    // We always need to keep these two up to date as they are updated by
    // location updates to ensure we have up-to-date values if we start out with
    // 0 replicated trees.
    query_data->loc = loc;
    query_data->bounds = bounds;
    // And these ones get updated if they've got new values
    if (angle != NoUpdateSolidAngle)
        query_data->angle = angle;
    if (max_results != NoUpdateMaxResults)
        query_data->max_results = max_results;

    // We iterate over all query handlers instead of over just the ones we know
    // we have registered because this might be an initial
    // registration.
    // FIXME(ewencp) this is clearly wrong since we'll end up
    // registering with all replicated indices but we should only start with the
    // root. We need to detect and handle new registrations better.
    for(ReplicatedIndexQueryHandlerMap::iterator handler_it = mObjectQueryHandlers.begin(); handler_it != mObjectQueryHandlers.end(); handler_it++) {
        ProxIndexID index_id = handler_it->first;
        IndexQueryMap::iterator it = query_data->queries.find(index_id);

        if (it == query_data->queries.end()) {
            registerObjectQueryWithIndex(object, index_id, handler_it->second.handler, loc, bounds, angle, max_results);
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
    ObjectQueryMap::iterator obj_it = mObjectQueries.find(object);
    if (obj_it == mObjectQueries.end()) return;
    ObjectQueryDataPtr obj_query_data = obj_it->second;

    for(IndexQueryMap::iterator query_it = obj_query_data->queries.begin(); query_it != obj_query_data->queries.end(); query_it++) {
        Query* q = query_it->second;
        // Don't remove from queries list until after done with iteration
        mInvertedObjectQueries.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }
    obj_query_data->queries.clear();

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


} // namespace Manual
} // namespace OH
} // namespace Sirikata
