// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ObjectQueryHandler.hpp"
#include "ManualObjectQueryProcessor.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

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

static String NoUpdateCustomQueryString("BOGUSQUERY");

namespace {

// Returns true only if the query parses and at least one value is extracted so
// that we can tell if this is really a traditional query or another
// JSON-formatted custom query type
bool parseQueryRequest(const String& query, SolidAngle* qangle_out, uint32* max_results_out) {
    if (query.empty())
        return false;

    namespace json = json_spirit;
    json::Value parsed_query;
    if (!json::read(query, parsed_query))
        return false;

    // Must have at least one value, remaining get set to default
    // values NOTE: Make sure you don't conflict with these parameter
    // names since their presence will let this pass and use solid
    // angle queries!
    if ((!parsed_query.contains("angle") || !parsed_query.get("angle").isReal()) &&
        (!parsed_query.contains("max_results") || !parsed_query.get("max_results").isInt()))
    {
        // Must contain at least one update
        return false;
    }

    *qangle_out = SolidAngle( parsed_query.getReal("angle", NoUpdateSolidAngle.asFloat()) );
    *max_results_out = parsed_query.getInt("max_results", (int)NoUpdateMaxResults);

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
    Liveness::letDie();
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
        std::tr1::bind(&ObjectQueryHandler::handleCreatedReplicatedIndex, this, livenessToken(), iid, loc_cache, objects_from_server, dynamic_objects),
        "ObjectQueryHandler::handleCreatedReplicatedIndex"
    );
    loc_cache->addListener(this);
}

void ObjectQueryHandler::removedReplicatedIndex(ProxIndexID iid) {
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleRemovedReplicatedIndex, this, livenessToken(), iid),
        "ObjectQueryHandler::handleRemovedReplicatedIndex"
    );
}



void ObjectQueryHandler::presenceConnected(const ObjectReference& objid) {
}

void ObjectQueryHandler::presenceDisconnected(const ObjectReference& objid) {
    // Prox strand may  have some state to clean up
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleDisconnectedObject, this, livenessToken(), objid),
        "ObjectQueryHandler::handleDisconnectedObject"
    );
}

void ObjectQueryHandler::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& obj, const String& params) {
    SolidAngle sa;
    uint32 max_results = 0;
    if (parseQueryRequest(params, &sa, &max_results))
        updateQuery(ho, obj, sa, max_results, NoUpdateCustomQueryString);
    else
        updateQuery(ho, obj, NoUpdateSolidAngle, NoUpdateMaxResults, params);
}

void ObjectQueryHandler::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& obj, SolidAngle sa, uint32 max_results, const String& custom_query_string) {
    // We don't use loc caches here becuase the querier might not be in
    // there yet
    SequencedPresencePropertiesPtr querier_props = ho->presenceRequestedLocation(obj);
    TimedMotionVector3f loc = querier_props->location();
    AggregateBoundingInfo bounds = querier_props->bounds();
    assert(bounds.singleObject());
    updateQuery(obj.object(), loc, bounds.fullBounds(), sa, max_results, custom_query_string);
}

void ObjectQueryHandler::updateQuery(const ObjectReference& obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results, const String& custom_query_string) {
    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleUpdateObjectQuery, this, livenessToken(), obj, loc, bounds, sa, max_results, custom_query_string),
        "ObjectQueryHandler::handleUpdateObjectQuery"
    );
}

void ObjectQueryHandler::removeQuery(HostedObjectPtr ho, const SpaceObjectReference& obj) {
    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleRemoveObjectQuery, this, livenessToken(), obj.object(), true),
        "ObjectQueryHandler::handleRemoveObjectQuery"
    );
}

int32 ObjectQueryHandler::objectQueries() const {
    return mObjectQueries.size();
}

int32 ObjectQueryHandler::objectQueryMessages() const {
    // const_cast because mObjectResults needs to take a lock internally, so
    // size is non-const
    return const_cast<ObjectQueryHandler*>(this)->mObjectResults.size();
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
        // These are the end result of regular queries (no replication), so
        // reparent events can be ignored.
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

void ObjectQueryHandler::handleNotifySubscribersLocUpdate(Liveness::Token alive, ReplicatedLocationServiceCache* loccache, const ObjectReference& oref) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

    SubscribersMap::iterator it = mSubscribers.find(oref);
    if (it == mSubscribers.end()) return;
    SubscriberSetPtr subscribers = it->second;

    // Make sure the object is still available and remains available
    // throughout all updates
    if (loccache->startRefcountTracking(oref)) {
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

        loccache->stopRefcountTracking(oref);
    }
}



void ObjectQueryHandler::queryHasEvents(Query* query) {
    InstanceMethodNotReentrant nr(mQueryHasEventsNotRentrant);

    generateObjectQueryEvents(query);
}

// AggregateListener Interface
// This class only uses this information to track observed nodes so we can
// let the parent class know when to refine/coarsen the query with the server.
void ObjectQueryHandler::aggregateObjectCreated(ProxAggregator* handler, const ObjectReference& objid) {}

void ObjectQueryHandler::aggregateObjectDestroyed(ProxAggregator* handler, const ObjectReference& objid) {}

void ObjectQueryHandler::aggregateCreated(ProxAggregator* handler, const ObjectReference& objid) {}

void ObjectQueryHandler::aggregateChildAdded(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {}

void ObjectQueryHandler::aggregateChildRemoved(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {}

void ObjectQueryHandler::aggregateBoundsUpdated(ProxAggregator* handler, const ObjectReference& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {}

void ObjectQueryHandler::aggregateQueryDataUpdated(ProxAggregator* handler, const ObjectReference& objid, const String& qd) {}

void ObjectQueryHandler::aggregateDestroyed(ProxAggregator* handler, const ObjectReference& objid) {
    // Allow canceling of unobserved timeouts for this node
    assert(mInverseObjectQueryHandlers.find(handler) != mInverseObjectQueryHandlers.end());
    ProxIndexID indexid = mInverseObjectQueryHandlers[handler];
    mParent->replicatedNodeRemoved(mSpaceNodeID, indexid, objid);
}

void ObjectQueryHandler::aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers, uint32 nchildren) {
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
    if (nobservers == 1 && nchildren == 0) {
        mParent->queriersAreObserving(mSpaceNodeID, indexid, objid);
    }
    else if (nobservers == 0 && nchildren > 0) {
        mParent->queriersStoppedObserving(mSpaceNodeID, indexid, objid);
    }
}


ObjectQueryHandler::ProxQueryHandler* ObjectQueryHandler::getQueryHandler(const String& handler_name) {
    // We really just need to extract the ProxIndexID from the handler name of
    // the form "snid.object-queries.server-x-index-y-staticdynamic", e.g.
    // "12345678-1111-1111-1111-defa01759ace:1.object-queries.server-0-index-5.static-objects"

    std::size_t dot_pos = handler_name.find('.');
    if (dot_pos == String::npos) return NULL;
    std::size_t dot2_pos = handler_name.find('.', dot_pos+1);
    if (dot2_pos == String::npos) return NULL;
    std::size_t dot3_pos = handler_name.find('.', dot2_pos+1);
    if (dot3_pos == String::npos) return NULL;

    //String snid_part = handler_name.substr(0, dot_pos);
    //String query_type_part = name.substr(dot_pos+1, dot2_pos-dot_pos);
    String tree_id_part = handler_name.substr(dot2_pos+1, dot3_pos-dot2_pos-1);
    // Now, within the tree part we need to extract the server ID and
    // index ID. We don't need the other part becuase within this
    // ObjectQueryHandler it is the same for all handlers
    std::size_t dash_pos = tree_id_part.find('-');
    if (dash_pos == String::npos) return NULL;
    std::size_t dash2_pos = tree_id_part.find('-', dash_pos+1);
    if (dash2_pos == String::npos) return NULL;
    std::size_t dash3_pos = tree_id_part.find('-', dash2_pos+1);
    if (dash3_pos == String::npos) return NULL;

    String iid_part = tree_id_part.substr(dash3_pos+1);

    ProxIndexID iid = boost::lexical_cast<ProxIndexID>(iid_part);
    ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.find(iid);
    if (it != mObjectQueryHandlers.end())
        return it->second.handler;
    return NULL;
}

void ObjectQueryHandler::commandListInfo(const OHDP::SpaceNodeID& snid, Command::Result& result) {
    // handlers.[queriertype].[queriedtype].property
    // in this case the form will be handlers.objects.server-x-index-y.property
    for(ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++) {
        String staticdynamic = (it->second.dynamic ? "dynamic" : "static");
        String key = String("handlers.object.") +
            "server-" + boost::lexical_cast<String>(it->second.from) + "-index-" + boost::lexical_cast<String>(it->first) + "-" + staticdynamic + ".";
        result.put(key + "name",
            // Server we're communicating wih, performing object queries
            snid.toString() + "." + String("object-queries.") +
            // The server the tree is replicated from - the index ID - static/dynamic
            "server-" + boost::lexical_cast<String>(it->second.from) + "-index-" + boost::lexical_cast<String>(it->first) + "." + staticdynamic +
            "-objects");
        result.put(key + "indexid", it->second.handler->handlerID());
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

void ObjectQueryHandler::commandListQueriers(const OHDP::SpaceNodeID& snid, Command::Result& result) {
    // NOTE: we're just one of multiple sources of querier info, so we just fill
    // info in, our parent initializes and finishes by sending the result
    Command::Result& object_queriers = result.get("queriers.object");

    for(ObjectQueryMap::iterator qit = mObjectQueries.begin(); qit != mObjectQueries.end(); qit++) {
        // Each object queries a bunch of replicated trees
        Command::Result data = Command::EmptyResult();

        for(IndexQueryMap::iterator index_it = qit->second->queries.begin(); index_it != qit->second->queries.end(); index_it++) {
            ProxIndexID index_id = index_it->first;
            Query* query = index_it->second;
            // index_it gives us the index we're quering into, but we still need
            // to look it up
            assert(mObjectQueryHandlers.find(index_id) != mObjectQueryHandlers.end());
            ReplicatedIndexQueryHandler& handler_data = mObjectQueryHandlers[index_id];

            String staticdynamic = (handler_data.dynamic ? "dynamic" : "static");
            String path =
                // Server we're communicating wih, performing object queries
                snid.toString() + "_" + String("object-queries_") +
                // The server the tree is replicated from - the index ID - static/dynamic
                "server-" + boost::lexical_cast<String>(handler_data.from) + "-index-" + boost::lexical_cast<String>(index_id) + "_" + staticdynamic +
                "-objects";
            data.put(path + ".results", query->numResults());
            data.put(path + ".size", query->size());
        }
        object_queriers.put(qit->first.toString(), data);
    }
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
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onLocationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    updateQuery(obj, loccache->location(obj), loccache->bounds(obj).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults, NoUpdateCustomQueryString);

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onOrientationUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onBoundsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    updateQuery(obj, loccache->location(obj), loccache->bounds(obj).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults, NoUpdateCustomQueryString);

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onMeshUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onPhysicsUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}

void ObjectQueryHandler::onQueryDataUpdated(ReplicatedLocationServiceCache* loccache, const ObjectReference& obj) {
    mContext->mainStrand->post(
        std::tr1::bind(&ObjectQueryHandler::handleNotifySubscribersLocUpdate, this, livenessToken(), loccache, obj),
        "ObjectQueryHandler::handleNotifySubscribersLocUpdate"
    );
}


// PROX Thread: Everything after this should only be called from within the prox thread.

void ObjectQueryHandler::handleCreatedReplicatedIndex(Liveness::Token alive, ProxIndexID iid, ReplicatedLocationServiceCachePtr loc_cache, ServerID objects_from_server, bool dynamic_objects) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

    assert(mObjectQueryHandlers.find(iid) == mObjectQueryHandlers.end());

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    using std::tr1::placeholders::_4;
    using std::tr1::placeholders::_5;
    using std::tr1::placeholders::_6;


    String object_handler_type = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_TYPE);
    String object_handler_options = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_OPTIONS);
    String object_handler_node_data = GetOptionValue<String>(OPT_MANUAL_QUERY_HANDLER_NODE_DATA);

    ProxQueryHandler* handler = ObjectProxGeomQueryHandlerFactory.getConstructor(object_handler_type, object_handler_node_data)(object_handler_options, false);
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
    for(ObjectQueryMap::iterator query_it = mObjectQueries.begin(); query_it != mObjectQueries.end(); query_it++) {
        ObjectReference objid = query_it->first;
        ObjectQueryDataPtr query_props = query_it->second;
        if (query_props->servers.find(objects_from_server) == query_props->servers.end()) continue;

        registerObjectQueryWithIndex(
            objid, iid, handler,
            // FIXME is loccache safe here? what if results for the object
            // haven't come in yet?
            query_props->loc, query_props->bounds,
            query_props->angle, query_props->max_results,
            query_props->custom_query_string
        );
    }
}

void ObjectQueryHandler::handleRemovedReplicatedIndex(Liveness::Token alive, ProxIndexID iid) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

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
            assert(handler_data.loccache->tracking(objid));

            mContext->mainStrand->post(
                std::tr1::bind(&ObjectQueryHandler::handleAddObjectLocSubscription, this, livenessToken(), querier_id, objid),
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
            // No need for set_query_data since this is going to an object.
            String phy = handler_data.loccache->physics(objid);
            if (phy.size() > 0)
                addition.set_physics(phy);

            // If we've reached a leaf in the TL-Pinto tree, we need to move
            // down to the next tree
            if (handler_data.from == NullServerID && evt.additions()[aidx].type() == QueryEvent::Normal) {
                // Need to unpack real ID from the UUID
                ServerID leaf_server = (ServerID)objid.getAsUUID().asUInt32();
                ObjectQueryDataPtr query_data = mObjectQueries[querier_id];
                registerObjectQueryWithServer(querier_id, leaf_server, query_data->loc, query_data->bounds, query_data->angle, query_data->max_results, query_data->custom_query_string);
            }
        }
        for(uint32 pidx = 0; pidx < evt.reparents().size(); pidx++) {
            ObjectReference objid = evt.reparents()[pidx].id();
            Sirikata::Protocol::Prox::INodeReparent reparent = event_results->add_reparent();
            reparent.set_object( objid.getAsUUID() );
            uint64 seqNo = handler_data.loccache->properties(objid).maxSeqNo();
            reparent.set_seqno (seqNo);
            reparent.set_old_parent(evt.reparents()[pidx].oldParent().getAsUUID());
            reparent.set_new_parent(evt.reparents()[pidx].newParent().getAsUUID());
            reparent.set_type(
                (evt.reparents()[pidx].type() == QueryEvent::Normal) ?
                Sirikata::Protocol::Prox::NodeReparent::Object :
                Sirikata::Protocol::Prox::NodeReparent::Aggregate
            );
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            ObjectReference objid = evt.removals()[ridx].id();
            // Clear out seqno and let main strand remove loc
            // subcription
            mContext->mainStrand->post(
                std::tr1::bind(&ObjectQueryHandler::handleRemoveObjectLocSubscription, this, livenessToken(), querier_id, objid),
                "ObjectQueryHandler::handleRemoveObjectLocSubscription"
            );

            Sirikata::Protocol::Prox::IObjectRemoval removal = event_results->add_removal();
            removal.set_object( objid.getAsUUID() );

            // It'd be nice if we didn't need this but the seqno might
            // not be available anymore and we still want to send the
            // removal.
            assert(handler_data.loccache->tracking(objid));
            uint64 seqNo = handler_data.loccache->properties(objid).maxSeqNo();
            removal.set_seqno (seqNo);

            removal.set_type(
                (evt.removals()[ridx].permanent() == QueryEvent::Permanent)
                ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                : Sirikata::Protocol::Prox::ObjectRemoval::Transient
            );

            // If we've moved above a leaf in the TL-Pinto tree, we need to move
            // away from that tree.
            if (handler_data.from == NullServerID && evt.additions()[ridx].type() == QueryEvent::Normal) {
                // Need to unpack real ID from the UUID
                ServerID leaf_server = (ServerID)objid.getAsUUID().asUInt32();
                unregisterObjectQueryWithServer(querier_id, leaf_server);
            }
        }
        evts.pop_front();

        mObjectResults.push( ProximityResultInfo(querier_id, event_results) );
    }
}

void ObjectQueryHandler::registerObjectQueryWithIndex(const ObjectReference& object, ProxIndexID index_id, ProxQueryHandler* handler, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results, const String& custom_query_string) {
    // We only want to register a query if we actually have all the necessary
    // info. We sometimes might not because we get this update for all location
    // updates, even those for objects which don't have subscriptions. For
    // traditional queries, this meant most importantly that we had a real minimum
    // angle. To support both traditional/custom queries, this condition depends
    // on the type of query handler we have
    if ((!handler->customQueryType() && angle == NoUpdateSolidAngle) ||
        (handler->customQueryType() && (custom_query_string == NoUpdateCustomQueryString || custom_query_string.empty())))
        return;

    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();
    ObjectQueryDataPtr query_data = mObjectQueries[object];

    float32 query_dist = 0.f;//TODO(ewencp) enable getting query
                             //distance from parameters
    Query* q;
    if (handler->customQueryType()) {
        assert(custom_query_string != NoUpdateCustomQueryString);
        q = handler->registerCustomQuery(custom_query_string);
    }
    else {
        q = mObjectDistance ?
            handler->registerQuery(loc, region, ms, SolidAngle::Min, query_dist) :
            handler->registerQuery(loc, region, ms, angle);
    }
    if (max_results != NoUpdateMaxResults && max_results > 0)
        q->maxResults(max_results);
    query_data->queries[index_id] = q;
    mInvertedObjectQueries[q] = std::make_pair(object, index_id);
    q->setEventListener(this);
}

void ObjectQueryHandler::registerObjectQueryWithServer(const ObjectReference& object, ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results, const String& custom_query_string) {
    // FIXME we don't currently have any index maintaining this information, so
    // we need to scan through all of them to find ones matching the ServerID
    for(ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++) {
        ProxIndexID prox_index_id = it->first;
        ReplicatedIndexQueryHandler& rep_index_query_handler = it->second;
        if (rep_index_query_handler.from != sid) continue;
        registerObjectQueryWithIndex(object, prox_index_id, rep_index_query_handler.handler, loc, bounds, angle, max_results, custom_query_string);
    }

    ObjectQueryDataPtr query_data = mObjectQueries[object];
    query_data->servers.insert(sid);
}

void ObjectQueryHandler::unregisterObjectQueryWithIndex(const ObjectReference& object, ProxIndexID index_id) {
    ObjectQueryDataPtr query_data = mObjectQueries[object];
    assert(query_data->queries.find(index_id) != query_data->queries.end());
    Query* q = query_data->queries[index_id];
    delete q;
    mInvertedObjectQueries.erase(q);
    query_data->queries.erase(index_id);
}

void ObjectQueryHandler::unregisterObjectQueryWithServer(const ObjectReference& object, ServerID sid) {
    // FIXME we don't currently have any index maintaining this information, so
    // we need to scan through all of them to find ones matching the ServerID
    // which also have queries registered by the object
    ObjectQueryDataPtr query_data = mObjectQueries[object];
    for(ReplicatedIndexQueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++) {
        ProxIndexID prox_index_id = it->first;
        ReplicatedIndexQueryHandler& rep_index_query_handler = it->second;
        if (rep_index_query_handler.from != sid) continue;
        if (query_data->queries.find(prox_index_id) == query_data->queries.end()) continue;
        unregisterObjectQueryWithIndex(object, prox_index_id);
    }

    query_data->servers.erase(sid);
}

void ObjectQueryHandler::handleUpdateObjectQuery(Liveness::Token alive, const ObjectReference& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results, const String& custom_query_string) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();

    // In some cases (new queries) we only want to do any work here,
    // including reporting the update, if the user explicitly made a
    // change to query parameters since this will get called for all
    // objects triggering movement.
    bool explicit_query_params_update = ((angle != NoUpdateSolidAngle) || (max_results != NoUpdateMaxResults) || (custom_query_string != NoUpdateCustomQueryString));
    bool new_query = false;

    if (mObjectQueries.find(object) == mObjectQueries.end()) {
        // If there's no existing query, so this was just because of a
        // location update -- don't record a query since it wouldn't
        // do anything anyway.
        if (!explicit_query_params_update) return;

        ObjectQueryDataPtr query_props(new ObjectQueryData());
        query_props->loc = loc;
        query_props->bounds = bounds;
        query_props->angle = (angle != NoUpdateSolidAngle) ? angle : SolidAngle::Max;
        query_props->max_results = (max_results != NoUpdateMaxResults) ? max_results : 100000;
        if (custom_query_string != NoUpdateCustomQueryString)
            query_props->custom_query_string = custom_query_string;
        mObjectQueries[object] = query_props;

        new_query = true;
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
    if (custom_query_string != NoUpdateCustomQueryString)
        query_data->custom_query_string = custom_query_string;

    // If we've got a new query, start it at the root. If we already have a
    // registered query, just update all queries we have a record of.
    if (new_query) {
        registerObjectQueryWithServer(object, NullServerID, loc, bounds, angle, max_results, custom_query_string);
    }
    else {
        for(IndexQueryMap::iterator index_query_it = query_data->queries.begin(); index_query_it != query_data->queries.end(); index_query_it++) {
            Query* query = index_query_it->second;
            query->position(loc);
            query->region( region );
            query->maxSize( ms );
            if (angle != NoUpdateSolidAngle)
                query->angle(angle);
            if (max_results != NoUpdateMaxResults && max_results > 0)
                query->maxResults(max_results);
            if (custom_query_string != NoUpdateCustomQueryString)
                query->customQuery(custom_query_string);
        }
    }
}

void ObjectQueryHandler::handleRemoveObjectQuery(Liveness::Token alive, const ObjectReference& object, bool notify_main_thread) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

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
            std::tr1::bind(&ObjectQueryHandler::handleRemoveAllObjectLocSubscription, this, livenessToken(), object),
            "ObjectQueryHandler::handleRemoveAllObjectLocSubscription"
        );
    }
}

void ObjectQueryHandler::handleDisconnectedObject(Liveness::Token alive, const ObjectReference& object) {
    // Clear out query state if it exists
    handleRemoveObjectQuery(alive, object, false);
}


} // namespace Manual
} // namespace OH
} // namespace Sirikata
