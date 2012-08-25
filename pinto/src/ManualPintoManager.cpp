// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualPintoManager.hpp"

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_MasterPinto.pbj.hpp"
#include <json_spirit/json_spirit.h>

#include <prox/manual/RTreeManualQueryHandler.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <sirikata/core/command/Commander.hpp>

using namespace Sirikata::Network;

#define PINTO_LOG(lvl, msg) SILOG(pinto,lvl,msg)

namespace Sirikata {

ManualPintoManager::ManualPintoManager(PintoContext* ctx)
 : PintoManagerBase(ctx),
   mLastTime(Time::null()),
   mDt(Duration::milliseconds((int64)1))
{
    mQueryHandler = new Prox::RTreeManualQueryHandler<ServerProxSimulationTraits>(10);
    bool static_objects = false, replicated = false;
    mQueryHandler->setAggregateListener(this); // *Must* be before handler->initialize
    mQueryHandler->initialize(
        mLocCache, mLocCache,
        static_objects, replicated
    );
}

ManualPintoManager::~ManualPintoManager() {
    delete mQueryHandler;
}

void ManualPintoManager::onConnected(Stream* newStream) {
    mClients[newStream] = ClientData();
}

void ManualPintoManager::onInitialMessage(Stream* stream) {
    TimedMotionVector3f default_loc( Time::null(), MotionVector3f( Vector3f::zero(), Vector3f::zero() ) );
    BoundingSphere3f default_region(BoundingSphere3f::null());
    float32 default_max = 0.f;
    // FIXME max_results
    Query* query = mQueryHandler->registerQuery(default_loc, default_region, default_max);
    ClientData& cdata = mClients[stream];
    cdata.query = query;
    mClientsByQuery[query] = stream;
    query->setEventListener(this);

    tick();
}

void ManualPintoManager::onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->region(bounds);

    sendLocUpdate(streamServerID(stream));
}

void ManualPintoManager::onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->maxSize(ms);

    tick();

    sendLocUpdate(streamServerID(stream));
}

void ManualPintoManager::onQueryUpdate(Stream* stream, const String& data) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    ServerID id = streamServerID(stream);

    // "Updates" are individual commands sent by the client, encoded
    // in json. Currently only 'refine' and 'coarsen'. (init and
    // destroy are implicit with the connection).
    namespace json = json_spirit;
    json::Value query_params;
    if (!json::read(data, query_params)) {
        PINTO_LOG(error, "Error parsing query request: " << data);
        return;
    }

    String action = query_params.getString("action", String(""));
    if (action.empty()) return;
    // We'll get these because it makes implementation easier to always send
    // them rather than having to filter. In this case they don't mean anything,
    // but for the local pinto implementation they are important since there
    // aren't any connection events that are implicit init/destroy messages, so
    // we just ignore them here.
    if (action == "init" || action == "destroy")
        return;

    if (action.empty() || (action != "refine" && action != "coarsen") ||
        !query_params.contains("nodes") || !query_params.get("nodes").isArray())
    {
        PINTO_LOG(detailed, "Invalid refine/coarsen request " << id);
        return;
    }

    if (action == "refine") {
        PINTO_LOG(detailed, "Refine query for " << id);

        json::Array json_nodes = query_params.getArray("nodes");
        BOOST_FOREACH(json::Value& v, json_nodes) {
            // The values have been converted to object IDs as
            // strings, we need to convert them back to server IDs
            if (!v.isString()) continue;
            cdata.query->refine((ServerID)(UUID(v.getString(),UUID::HumanReadable()).asUInt32()));
        }
    }
    else if (action == "coarsen") {
        PINTO_LOG(detailed, "Coarsen query for " << id);

        json::Array json_nodes = query_params.getArray("nodes");
        std::vector<UUID> coarsen_nodes;
        BOOST_FOREACH(json::Value& v, json_nodes) {
            // See above note about format
            if (!v.isString()) continue;
            cdata.query->coarsen((ServerID)(UUID(v.getString(),UUID::HumanReadable()).asUInt32()));
        }
    }

    tick();
}

void ManualPintoManager::onDisconnected(Stream* stream) {
    // Clean out any subscriptions we have
    for(ServerSet::const_iterator sub_it = mClients[stream].results.begin(); sub_it != mClients[stream].results.end(); sub_it++)
        mServerSubscribers[*sub_it].erase(stream);
    // Then clean out the query itself
    mClientsByQuery.erase( mClients[stream].query );
    delete mClients[stream].query;
    mClients.erase(stream);

    tick();
}

void ManualPintoManager::tick() {
    mLastTime += mDt;
    mQueryHandler->tick(mLastTime);
}

void ManualPintoManager::queryHasEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    Network::Stream* stream = mClientsByQuery[query];
    ClientData& cdata = mClients[stream];

    QueryEventList evts;
    query->popEvents(evts);

    Sirikata::Protocol::MasterPinto::PintoResponse pinto_response;

    Sirikata::Protocol::Prox::IProximityResults prox_results = pinto_response.mutable_prox_results();
    // We currently have to set this even though we don't have real synchronized
    // timestamps because it's required in the protocol definition
    prox_results.set_t(Time::null());
    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();

        Sirikata::Protocol::Prox::IProximityUpdate event_results = prox_results.add_update();
        uint64 update_seqno = cdata.seqno++;

        // We always want to tag this with the unique query handler index ID
        // so the client can properly group the replicas
        Sirikata::Protocol::Prox::IIndexProperties index_props = event_results.mutable_index_properties();
        index_props.set_id(evt.indexID());
        // Save index IDs so we can include them in loc updates. We only have
        // the one query handler, so just keep a single list we'll use for all
        // updates
        mProxIndices.insert(evt.indexID());

        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            ServerID nodeid = evt.additions()[aidx].id();
            assert (mLocCache->tracking(nodeid));

            // Track forward and inverse so we can both notify on server event
            // and remove on query destruction
            cdata.results.insert(nodeid);
            mServerSubscribers[nodeid].insert(stream);

            PintoManagerLocationServiceCache::Iterator loccacheit = mLocCache->startTracking(nodeid);

            Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
            // Shoe-horn server ID into UUID
            addition.set_object(UUID((uint32)nodeid));

            addition.set_seqno (update_seqno);

            Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
            TimedMotionVector3f loc = mLocCache->location(loccacheit);
            motion.set_t(loc.updateTime());
            motion.set_position(loc.position());
            motion.set_velocity(loc.velocity());

            // FIXME(ewencp) We don't track this since we wouldn't modify
            // any orientations, but it's currently required by the prox
            // message format...
            Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
            msg_orient.set_t(Time::null());
            msg_orient.set_position(Quaternion::identity());
            msg_orient.set_velocity(Quaternion::identity());

            Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
            msg_bounds.set_center_offset( mLocCache->centerOffset(loccacheit) );
            msg_bounds.set_center_bounds_radius( mLocCache->centerBoundsRadius(loccacheit) );
            msg_bounds.set_max_object_size( mLocCache->maxSize(loccacheit) );

            const String& mesh = mLocCache->mesh(loccacheit);
            if (mesh.size() > 0)
                addition.set_mesh(mesh);

            // Either we set a parent, or, if we're adding the root node for
            // the first time (lone addition), we include tree
            // properties. Strictly speaking, these shouldn't be necessary,
            // but including them lets us reuse client code which just
            // checks if some fields are present to know when it has to
            // start tracking a new replicated tree
            ServerID parentid = evt.additions()[aidx].parent();
            if (parentid != NullServerID) {
                // Shoe-horn server ID into UUID
                addition.set_parent(UUID((uint32)parentid));
            }
            else if (/*lone addition*/ aidx == 0 && evt.additions().size() == 1 && evt.removals().size() == 0) {
                // No good value to put here, but NullServerID should never
                // conflict with any space server
                index_props.set_index_id( boost::lexical_cast<String>(NullServerID) );
                // In TL pinto, we're only tracking static objects (aggregates).
                index_props.set_dynamic_classification(Sirikata::Protocol::Prox::IndexProperties::Static);
            }

            addition.set_type(
                (evt.additions()[aidx].type() == QueryEvent::Normal) ?
                Sirikata::Protocol::Prox::ObjectAddition::Object :
                Sirikata::Protocol::Prox::ObjectAddition::Aggregate
            );

            mLocCache->stopTracking(loccacheit);
        }
        for(uint32 pidx = 0; pidx < evt.reparents().size(); pidx++) {
            Sirikata::Protocol::Prox::INodeReparent reparent = event_results.add_reparent();
            reparent.set_object( UUID((uint32)evt.reparents()[pidx].id()) );
            reparent.set_seqno (update_seqno);
            reparent.set_old_parent( UUID((uint32)evt.reparents()[pidx].oldParent()) );
            reparent.set_new_parent( UUID((uint32)evt.reparents()[pidx].newParent()) );
            reparent.set_type(
                (evt.reparents()[pidx].type() == QueryEvent::Normal) ?
                Sirikata::Protocol::Prox::NodeReparent::Object :
                Sirikata::Protocol::Prox::NodeReparent::Aggregate
            );
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            ServerID nodeid = evt.removals()[ridx].id();

            // Track forward and inverse so we can both notify on server event
            // and remove on query destruction
            cdata.results.erase(nodeid);
            mServerSubscribers[nodeid].erase(stream);

            Sirikata::Protocol::Prox::IObjectRemoval removal = event_results.add_removal();
            // Shoe-horn server ID into UUID
            removal.set_object(UUID((uint32)nodeid));
            removal.set_seqno (update_seqno);
            removal.set_type(
                (evt.removals()[ridx].permanent() == QueryEvent::Permanent)
                ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                : Sirikata::Protocol::Prox::ObjectRemoval::Transient
            );
        }
        evts.pop_front();
    }

    // FIXME(ewencp) this send could fail, especially given that we
    // could end up with a lot of results initially. Should be queuing
    // these messages up
    String serialized = serializePBJMessage(pinto_response);
    bool success = stream->send( MemoryReference(serialized), ReliableOrdered );
    assert(success);
}

void ManualPintoManager::aggregateBoundsUpdated(ProxAggregator* handler, const ServerID& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size)
{
    PintoManagerBase::aggregateBoundsUpdated(handler, objid, bnds_center, bnds_center_radius, max_obj_size);
    sendLocUpdate(objid);
}

void ManualPintoManager::sendLocUpdate(ServerID about) {
    // All nodes will get the same update, *except* for the seqno
    Sirikata::Protocol::MasterPinto::PintoResponse msg;
    Sirikata::Protocol::Loc::IBulkLocationUpdate blu = msg.mutable_loc_updates();
    Sirikata::Protocol::Loc::ILocationUpdate update = blu.add_update();

    assert (mLocCache->tracking(about));
    PintoManagerLocationServiceCache::Iterator loccacheit = mLocCache->startTracking(about);

    update.set_object(UUID((uint32)about));

    // Indexes is just a central list since we only have one query handler
    for(ProxIndexIDSet::iterator prox_idx_it = mProxIndices.begin(); prox_idx_it != mProxIndices.end(); prox_idx_it++)
        update.add_index_id((uint32)*prox_idx_it);

    Sirikata::Protocol::ITimedMotionVector motion = update.mutable_location();
    TimedMotionVector3f loc = mLocCache->location(loccacheit);
    motion.set_t(loc.updateTime());
    motion.set_position(loc.position());
    motion.set_velocity(loc.velocity());

    Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = update.mutable_aggregate_bounds();
    msg_bounds.set_center_offset( mLocCache->centerOffset(loccacheit) );
    msg_bounds.set_center_bounds_radius( mLocCache->centerBoundsRadius(loccacheit) );
    msg_bounds.set_max_object_size( mLocCache->maxSize(loccacheit) );

    mLocCache->stopTracking(loccacheit);

    for(ClientStreamSet::iterator cit = mServerSubscribers[about].begin(); cit != mServerSubscribers[about].end(); cit++) {
        Sirikata::Network::Stream* stream = *cit;
        ClientData& cdata = mClients[stream];
        update.set_seqno(cdata.seqno++);
        String serialized = serializePBJMessage(msg);
        bool success = stream->send( MemoryReference(serialized), ReliableOrdered );
        assert(success);
    }
}



// BaseProxCommandable
void ManualPintoManager::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    result.put("name", "manual-pinto-manager");
    result.put("settings.handlers", 1);

    result.put("servers.properties.count", mClients.size());
    result.put("queries.servers.count", mClients.size());

    cmdr->result(cmdid, result);
}

void ManualPintoManager::commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    String key = String("handlers.servers.servers.");
    result.put(key + "name", String("server-queries"));
    result.put(key + "indexid", mQueryHandler->handlerID());
    result.put(key + "queries", mQueryHandler->numQueries());
    result.put(key + "objects", mQueryHandler->numObjects());
    result.put(key + "nodes", mQueryHandler->numNodes());
    cmdr->result(cmdid, result);
}

void ManualPintoManager::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    result.put( String("nodes"), Command::Array());
    Command::Array& nodes_ary = result.getArray("nodes");
    for(ProxQueryHandler::NodeIterator nit = mQueryHandler->nodesBegin(); nit != mQueryHandler->nodesEnd(); nit++) {
        nodes_ary.push_back( Command::Object() );
        nodes_ary.back().put("id", boost::lexical_cast<String>(nit.id()));
        nodes_ary.back().put("parent", boost::lexical_cast<String>(nit.parentId()));
        BoundingSphere3f bounds = nit.bounds(mContext->simTime());
        nodes_ary.back().put("bounds.center.x", bounds.center().x);
        nodes_ary.back().put("bounds.center.y", bounds.center().y);
        nodes_ary.back().put("bounds.center.z", bounds.center().z);
        nodes_ary.back().put("bounds.radius", bounds.radius());
        nodes_ary.back().put("cuts", nit.cuts());
    }

    cmdr->result(cmdid, result);
}

void ManualPintoManager::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put("error", "Rebuilding manual proximity processors isn't supported yet.");
    cmdr->result(cmdid, result);
}

void ManualPintoManager::commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put( String("stats"), Command::EmptyResult());
    cmdr->result(cmdid, result);
}


} // namespace Sirikata
