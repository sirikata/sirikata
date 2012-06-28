// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualPintoManager.hpp"

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_Prox.pbj.hpp"
#include <json_spirit/json_spirit.h>

#include <prox/manual/RTreeManualQueryHandler.hpp>
#include <boost/foreach.hpp>

using namespace Sirikata::Network;

#define PINTO_LOG(lvl, msg) SILOG(pinto,lvl,msg)

namespace Sirikata {

ManualPintoManager::ManualPintoManager(PintoContext* ctx)
 : PintoManagerBase(ctx),
   mLastTime(Time::null()),
   mDt(Duration::milliseconds((int64)1))
{
    mQueryHandler = new Prox::RTreeManualQueryHandler<ServerProxSimulationTraits>(10);
    bool static_objects = false;
    mQueryHandler->initialize(
        mLocCache, mLocCache,
        static_objects, false /* not replicated */
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
}

void ManualPintoManager::onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->region(bounds);
}

void ManualPintoManager::onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->maxSize(ms);
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

    if (action.empty() || (action != "refine" && action != "coarsen") ||
        !query_params.contains("nodes") || !query_params.get("nodes").isArray())
    {
        PINTO_LOG(detailed, "Invalid refine request " << id);
        return;
    }

    if (action == "refine") {
        PINTO_LOG(detailed, "Refine query for " << id);

        json::Array json_nodes = query_params.getArray("nodes");
        BOOST_FOREACH(json::Value& v, json_nodes) {
            if (!v.isInt()) continue;
            cdata.query->refine((ServerID)v.getInt());
        }
    }
    else if (action == "coarsen") {
        PINTO_LOG(detailed, "Coarsen query for " << id);

        json::Array json_nodes = query_params.getArray("nodes");
        std::vector<UUID> coarsen_nodes;
        BOOST_FOREACH(json::Value& v, json_nodes) {
            if (!v.isInt()) continue;
            cdata.query->coarsen((ServerID)v.getInt());
        }
    }
}

void ManualPintoManager::onDisconnected(Stream* stream) {
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

    Sirikata::Protocol::Prox::ProximityResults prox_results;
    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();

        Sirikata::Protocol::Prox::IProximityUpdate event_results = prox_results.add_update();
        uint64 update_seqno = cdata.seqno++;

        // We always want to tag this with the unique query handler index ID
        // so the client can properly group the replicas
        Sirikata::Protocol::Prox::IIndexProperties index_props = event_results.mutable_index_properties();
        index_props.set_id(evt.indexID());

        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            ServerID nodeid = evt.additions()[aidx].id();
            if (mLocCache->tracking(nodeid)) { // If the cache already lost it, we can't do anything
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

                ServerID parentid = evt.additions()[aidx].parent();
                if (parentid != NullServerID) {
                    // Shoe-horn server ID into UUID
                    addition.set_parent(UUID((uint32)parentid));
                }
                addition.set_type(
                    (evt.additions()[aidx].type() == QueryEvent::Normal) ?
                    Sirikata::Protocol::Prox::ObjectAddition::Object :
                    Sirikata::Protocol::Prox::ObjectAddition::Aggregate
                );

                mLocCache->stopTracking(loccacheit);
            }
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            ServerID nodeid = evt.removals()[ridx].id();

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
    String serialized = serializePBJMessage(prox_results);
    stream->send( MemoryReference(serialized), ReliableOrdered );
}

} // namespace Sirikata
