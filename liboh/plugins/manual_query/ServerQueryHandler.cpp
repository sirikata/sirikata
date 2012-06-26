// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ServerQueryHandler.hpp"
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Frame.pbj.hpp"

#include <json_spirit/json_spirit.h>

#include "ManualObjectQueryProcessor.hpp"

#define QPLOG(lvl, msg) SILOG(manual-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Manual {



namespace {

// Helpers for encoding requests
String initRequest() {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "init");
    return json::write(req);
}

String destroyRequest() {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "destroy");
    return json::write(req);
}

String refineRequest(const std::vector<ObjectReference>& aggs) {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "refine");
    req.put("nodes", json::Array());
    json::Array& nodes = req.getArray("nodes");
    for(uint32 i = 0; i < aggs.size(); i++)
        nodes.push_back( json::Value(aggs[i].toString()) );
    return json::write(req);
}

String coarsenRequest(const std::vector<ObjectReference>& aggs) {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "coarsen");
    req.put("nodes", json::Array());
    json::Array& nodes = req.getArray("nodes");
    for(uint32 i = 0; i < aggs.size(); i++)
        nodes.push_back( json::Value(aggs[i].toString()) );
    return json::write(req);
}

} // namespace


using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;
using std::tr1::placeholders::_3;


ServerQueryHandler::ServerQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, Network::IOStrandPtr strand)
 : mContext(ctx),
   mParent(parent),
   mStrand(strand),
   mCleanupOrphansPoller(
       strand.get(),
       std::tr1::bind(&ServerQueryHandler::cleanupOrphans, this),
       "ServerQueryHandler::cleanupOrphans",
       Duration::minutes(1)
   )
{
}

void ServerQueryHandler::start() {
    mContext->objectHost->SpaceNodeSessionManager::addListener(static_cast<SpaceNodeSessionListener*>(this));
    mCleanupOrphansPoller.start();
}

void ServerQueryHandler::stop() {
    mContext->objectHost->SpaceNodeSessionManager::removeListener(static_cast<SpaceNodeSessionListener*>(this));
    mCleanupOrphansPoller.stop();
    mServerQueries.clear();
}


void ServerQueryHandler::onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {
    QPLOG(detailed, "New space node session " << id);
    assert(mServerQueries.find(id) == mServerQueries.end());

    Network::IOTimerPtr unobserved_timer(
        Network::IOTimer::create(
            mStrand.get(),
            std::tr1::bind(&ServerQueryHandler::processExpiredNodes, this, id)
        )
    );
    ServerQueryStatePtr sqs(new ServerQueryState(mContext, mStrand, sn_stream, unobserved_timer));
    mServerQueries.insert(
        ServerQueryMap::value_type(id, sqs)
    );

    sn_stream->listenSubstream(OBJECT_PORT_LOCATION,
        std::tr1::bind(&ServerQueryHandler::handleLocationSubstream, this,
            id, _1, _2
        )
    );

    mParent->createdServerQuery(id);
}

void ServerQueryHandler::onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) {
    mServerQueries.erase(id);
    mParent->removedServerQuery(id);
}


void ServerQueryHandler::incrementServerQuery(OHDP::SpaceNodeID node) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(node);
    assert(serv_it != mServerQueries.end());

    bool is_new = (serv_it->second->nconnected == 0);
    serv_it->second->nconnected++;
    updateServerQuery(serv_it, is_new);
}

void ServerQueryHandler::updateServerQuery(ServerQueryMap::iterator serv_it, bool is_new) {
    if (is_new) {
        QPLOG(detailed, "Initializing server query to " << serv_it->first);
        sendInitRequest(serv_it);
    }
}

void ServerQueryHandler::decrementServerQuery(OHDP::SpaceNodeID node) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(node);
    assert(serv_it != mServerQueries.end());

    serv_it->second->nconnected--;
    if (!serv_it->second->canRemove()) return;

    QPLOG(detailed, "Destroying server query to " << serv_it->first);
    sendDestroyRequest(serv_it);
}


// Proximity

void ServerQueryHandler::sendInitRequest(ServerQueryMap::iterator serv_it) {
    Protocol::Prox::QueryRequest request;
    request.set_query_parameters(initRequest());
    if (request.query_parameters().empty()) return;
    std::string init_msg_str = serializePBJMessage(request);

    String framed = Network::Frame::write(init_msg_str);
    sendProxMessage(serv_it, framed);
}

void ServerQueryHandler::sendDestroyRequest(ServerQueryMap::iterator serv_it) {
    if (!serv_it->second->prox_stream) return;

    Protocol::Prox::QueryRequest request;
    request.set_query_parameters(destroyRequest());
    if (request.query_parameters().empty()) return;
    std::string destroy_msg_str = serializePBJMessage(request);
    String framed = Network::Frame::write(destroy_msg_str);
    sendProxMessage(serv_it, framed);
}

void ServerQueryHandler::sendRefineRequest(ServerQueryMap::iterator serv_it, const ObjectReference& agg) {
    std::vector<ObjectReference> aggs;
    aggs.push_back(agg);
    sendRefineRequest(serv_it, aggs);
}

void ServerQueryHandler::sendRefineRequest(ServerQueryMap::iterator serv_it, const std::vector<ObjectReference>& aggs) {
    Protocol::Prox::QueryRequest request;
    request.set_query_parameters(refineRequest(aggs));
    if (request.query_parameters().empty()) return;
    std::string msg_str = serializePBJMessage(request);
    String framed = Network::Frame::write(msg_str);
    sendProxMessage(serv_it, framed);
}

void ServerQueryHandler::sendCoarsenRequest(ServerQueryMap::iterator serv_it, const ObjectReference& agg) {
    std::vector<ObjectReference> aggs;
    aggs.push_back(agg);
    sendCoarsenRequest(serv_it, aggs);
}

void ServerQueryHandler::sendCoarsenRequest(ServerQueryMap::iterator serv_it, const std::vector<ObjectReference>& aggs) {
    Protocol::Prox::QueryRequest request;
    request.set_query_parameters(coarsenRequest(aggs));
    if (request.query_parameters().empty()) return;
    std::string msg_str = serializePBJMessage(request);
    String framed = Network::Frame::write(msg_str);
    sendProxMessage(serv_it, framed);
}


void ServerQueryHandler::sendProxMessage(ServerQueryMap::iterator serv_it, const String& msg) {
    serv_it->second->outstanding.push(msg);

    // Possibly trigger writing

    // Already in progress
    if (serv_it->second->writing) return;

    // Without a stream, we need to first create the stream
    if (!serv_it->second->prox_stream) {
        serv_it->second->writing = true;

        serv_it->second->base_stream->createChildStream(
            std::tr1::bind(&ServerQueryHandler::handleCreatedProxSubstream, this, serv_it->first, _1, _2),
            NULL, 0,
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
        );
        return;
    }

    // Otherwise, we're clear for starting writing
    writeSomeProxData(serv_it->second);
}

void ServerQueryHandler::writeSomeProxData(ServerQueryStatePtr data) {
    assert(data->prox_stream);
    data->writing = true;

    while(!data->outstanding.empty()) {
        String& msg = data->outstanding.front();
        int bytes_written = data->prox_stream->write((const uint8*)msg.data(), msg.size());
        if (bytes_written < 0) {
            break;
        }
        else if (bytes_written < (int)msg.size()) {
            msg = msg.substr(bytes_written);
            break;
        }
        else {
            data->outstanding.pop();
        }
    }

    if (data->outstanding.empty()) {
        data->writing = false;
    }
    else {
        if (mContext->stopped()) return;

        static Duration retry_rate = Duration::milliseconds((int64)1);
        mContext->mainStrand->post(
            retry_rate,
            std::tr1::bind(&ServerQueryHandler::writeSomeProxData, this, data),
            "ServerQueryHandler::writeSomeProxData"
        );
    }
}

void ServerQueryHandler::handleCreatedProxSubstream(const OHDP::SpaceNodeID& snid, int success, OHDPSST::Stream::Ptr prox_stream) {
    if (success != SST_IMPL_SUCCESS)
        QPLOG(error, "Failed to create proximity substream to " << snid);

    // Save the stream for additional writing
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    // We may have lost the session since we requested the connection
    if (serv_it == mServerQueries.end()) return;
    serv_it->second->prox_stream = prox_stream;

    // Register to get data
    String* prevdata = new String();
    prox_stream->registerReadCallback(
        std::tr1::bind(&ServerQueryHandler::handleProximitySubstreamRead, this,
            snid, prox_stream, prevdata, _1, _2
        )
    );

    // We created the stream to start writing, so start that now
    assert(serv_it->second->writing == true);
    writeSomeProxData(serv_it->second);
}

void ServerQueryHandler::handleProximitySubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr prox_stream, String* prevdata, uint8* buffer, int length) {
    if (mContext->stopped()) {
        QPLOG(detailed, "Ignoring proximity update after system stop requested.");
        return;
    }

    prevdata->append((const char*)buffer, length);

    while(true) {
        std::string msg = Network::Frame::parse(*prevdata);

        // If we don't have a full message, just wait for more
        if (msg.empty()) return;

        // Otherwise, try to handle it
        handleProximityMessage(snid, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //prox_stream->registerReadCallback(0);
}

void ServerQueryHandler::handleProximityMessage(const OHDP::SpaceNodeID& snid, const String& payload) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received proximity message without query. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success) {
        QPLOG(error, "Failed to decode proximity message");
        return;
    }

    // Proximity messages are handled just by updating our state
    // tracking the objects, adding or removing the objects or
    // updating their properties.
    QPLOG(detailed, "Received proximity message with " << contents.update_size() << " updates");
    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        // We may have to trigger creation of a new replicated tree if this is a
        // new root. We should always extract the ID of the tree this update
        // applies to
        assert(update.has_index_properties());
        Sirikata::Protocol::Prox::IndexProperties index_props = update.index_properties();
        assert(index_props.has_id());
        uint32 index_unique_id = index_props.id();
        QPLOG(detailed, "-Update for tree index ID: " << index_unique_id);
        if (index_props.has_index_id() || index_props.has_dynamic_classification()) {
            QPLOG(detailed, "New replicated tree:");
            QPLOG(detailed, " ID: " << index_props.id());
            ServerID index_from_server = NullServerID;
            if (index_props.has_index_id()) {
                QPLOG(detailed, " Tree ID: " << index_props.index_id());
                index_from_server = boost::lexical_cast<ServerID>(index_props.index_id());
            }
            bool dynamic_objects = true;
            if (index_props.has_dynamic_classification()) {
                dynamic_objects = !(index_props.dynamic_classification() == Sirikata::Protocol::Prox::IndexProperties::Static);
                QPLOG(detailed, " Class: " << (dynamic_objects ? "dynamic" : "static"));
            }

            // Create a new loccache for this new tree. We could be getting this
            query_state->createLocCache(index_unique_id);
            mParent->createdReplicatedIndex(snid, index_unique_id, query_state->getLocCache(index_unique_id), index_from_server, dynamic_objects);
        }

        OHLocationServiceCachePtr loccache = query_state->getLocCache(index_unique_id);
        OrphanLocUpdateManagerPtr orphan_manager = query_state->getOrphanLocUpdateManager(index_unique_id);
        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
            // Convert to local time
            addition.location().set_t(
                mContext->objectHost->localTime(snid.space(), addition.location().t())
            );

            ProxProtocolLocUpdate add(addition);

            ObjectReference observed_oref(addition.object());
            SpaceObjectReference observed(snid.space(), observed_oref);

            bool is_agg = (addition.has_type() && addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate);

            ObjectReference parent_oref(addition.parent());

            // Store the data
            loccache->objectAdded(
                observed_oref, is_agg,
                parent_oref,
                add.location(), add.location_seqno(),
                add.orientation(), add.orientation_seqno(),
                add.bounds(), add.bounds_seqno(),
                Transfer::URI(add.meshOrDefault()), add.mesh_seqno(),
                add.physicsOrDefault(), add.physics_seqno()
            );

            // Replay orphans
            orphan_manager->invokeOrphanUpdatesWithExtra(mContext->objectHost, snid, observed, this, index_unique_id);
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);
            ObjectReference observed_oref(removal.object());
            SpaceObjectReference observed(snid.space(), observed_oref);
            bool temporary_removal = (!removal.has_type() || (removal.type() == Sirikata::Protocol::Prox::ObjectRemoval::Transient));
            // Backup data for orphans and then destroy
            // TODO(ewencp) Seems like we shouldn't actually need this
            // since we shouldn't get a removal with having had an
            // addition first...
            if (loccache->startSimpleTracking(observed_oref)) {
                orphan_manager->addUpdateFromExisting(observed, loccache->properties(observed_oref));
                loccache->stopSimpleTracking(observed_oref);
            }
            loccache->objectRemoved(
                observed_oref, temporary_removal
            );
        }

        // We may have removed everything from the specified tree. We need to
        // check if we've hit that condition and clean out any associated
        // state. If objects from that server reappear, this state will be
        // created from scratch.
        if (loccache->empty()) {
            mParent->removedReplicatedIndex(snid, index_unique_id);
            query_state->createLocCache(index_unique_id);
        }
    }

}



// Proximity tracking updates in local queries to trigger adjustments to server query
void ServerQueryHandler::queriersAreObserving(const OHDP::SpaceNodeID& snid, const ObjectReference& objid) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received update about object queries on server query that doesn't exist. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    // Someone is observing this node, we should try to refine it
    sendRefineRequest(serv_it, objid);

    // And make sure we don't have it lined up for coarsening
    UnobservedNodesByID& by_id = query_state->unobservedTimeouts.get<objid_tag>();
    UnobservedNodesByID::iterator it = by_id.find(objid);
    if (it == by_id.end()) return;
    by_id.erase(it);
}

void ServerQueryHandler::queriersStoppedObserving(const OHDP::SpaceNodeID& snid, const ObjectReference& objid) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received update about object queries on server query that doesn't exist. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    // Nobody is observing this node, we should try to coarsen it after awhile,
    // but only if it doesn't have children.
    query_state->unobservedTimeouts.insert(UnobservedNodeTimeout(objid, mContext->recentSimTime() + Duration::seconds(15)));
    // And restart the timeout for the most recent
    Duration next_timeout = query_state->unobservedTimeouts.get<expires_tag>().begin()->expires - mContext->recentSimTime();
    query_state->unobservedTimer->cancel();
    query_state->unobservedTimer->wait(next_timeout);
}

void ServerQueryHandler::processExpiredNodes(const OHDP::SpaceNodeID& snid) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) return;
    ServerQueryStatePtr& query_state = serv_it->second;

    Time curt = mContext->recentSimTime();
    UnobservedNodesByExpiration& by_expires = query_state->unobservedTimeouts.get<expires_tag>();
    while(!by_expires.empty() &&
        by_expires.begin()->expires < curt)
    {
        sendCoarsenRequest(serv_it, by_expires.begin()->objid);
        by_expires.erase(by_expires.begin());
    }

    if (!by_expires.empty()) {
        Duration next_timeout = query_state->unobservedTimeouts.get<expires_tag>().begin()->expires - mContext->recentSimTime();
        query_state->unobservedTimer->wait(next_timeout);
    }
}

// Location


void ServerQueryHandler::handleLocationSubstream(const OHDP::SpaceNodeID& snid, int err, OHDPSST::Stream::Ptr s) {
    s->registerReadCallback(
        std::tr1::bind(
            &ServerQueryHandler::handleLocationSubstreamRead, this,
            snid, s, new std::stringstream(), _1, _2
        )
    );
}

void ServerQueryHandler::handleLocationSubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    if (handleLocationMessage(snid, prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s, and close the stream
        s->registerReadCallback(0);
        s->close(false);
    }
}

namespace {
// Helper for applying an update to
void applyLocUpdate(const ObjectReference& objid, OHLocationServiceCachePtr loccache, const LocUpdate& lu) {
    // We can bail immediately if none of the updates are relevant since the
    // object isn't even being tracked
    if (!loccache->tracking(objid)) return;

    if (lu.has_epoch())
        loccache->epochUpdated(objid, lu.epoch());
    if (lu.has_parent())
        loccache->parentUpdated(objid, lu.parent(), lu.parent_seqno());
    if (lu.has_location())
        loccache->locationUpdated(objid, lu.location(), lu.location_seqno());
    if (lu.has_orientation())
        loccache->orientationUpdated(objid, lu.orientation(), lu.orientation_seqno());
    if (lu.has_bounds())
        loccache->boundsUpdated(objid, lu.bounds(), lu.bounds_seqno());
    if (lu.has_mesh())
        loccache->meshUpdated(objid, Transfer::URI(lu.meshOrDefault()), lu.mesh_seqno());
    if (lu.has_physics())
        loccache->physicsUpdated(objid, lu.physicsOrDefault(), lu.physics_seqno());
}
}

bool ServerQueryHandler::handleLocationMessage(const OHDP::SpaceNodeID& snid, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received location message without query. Query may have recently been destroyed.");
        return true; // Was successfully parsed, but not handled
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);
        ObjectReference observed_oref(update.object());
        SpaceObjectReference observed(snid.space(), observed_oref);

        // Because of prox/loc ordering, we may or may not have a record of the
        // object yet. We need to send to either OrphanLocUpdateManager or, if
        // we do already have the object, we can apply it now.

        // However, in the case of replicated trees, we need to make sure we
        // apply to the right trees. We need the server to have told us which
        // ones to apply it to
        if (update.index_id_size() == 0) {
            QPLOG(error, "LocationUpdate didn't include index_id to indicate which replicated trees need it, this update will be ignored!");
            continue;
        }

        for(int32 index_id_idx = 0; index_id_idx < update.index_id_size(); index_id_idx++) {
            ProxIndexID index_id = update.index_id(index_id_idx);
            ServerQueryState::IndexObjectCacheMap::iterator index_it = query_state->objects.find(index_id);
            // If we can't find an index, we may just be getting the update
            // earlier than we're getting the prox update, i.e. just what
            // OrphanLocUpdateManager handles. But we haven't necessarily
            // allocated one yet. We'll allocate the state, but *not* notify the
            // parent that we createdReplicatedIndex yet since we haven't really
            // yet and we need the info from the prox update's IndexProperties
            // to do that. See handleProximityMessage for how this is finally
            // resolved.
            if (index_it == query_state->objects.end()) {
                query_state->createLocCache(index_id);
                // Add it to a list so we can clean it out eventually
                // if we never get prox data for it
                mCachesForOrphans.push_back(std::make_pair(snid, index_id));
            }
            // Then we just continue as normal, sticking it in as an orphan if
            // necessary
            if (!query_state->getLocCache(index_id)->tracking(observed_oref)) {
                query_state->getOrphanLocUpdateManager(index_id)->addOrphanUpdate(observed, update);
            }
            else { // or actually applying it
                LocProtocolLocUpdate llu(update, mContext->objectHost, snid.space());
                applyLocUpdate(observed_oref, query_state->getLocCache(index_id), llu);
            }
        }
    }

    return true;
}


void ServerQueryHandler::onOrphanLocUpdate(const OHDP::SpaceNodeID& observer, const LocUpdate& lu, ProxIndexID iid) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(observer);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received proximity message without query. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    SpaceObjectReference observed(observer.space(), lu.object());
    assert(query_state->getLocCache(iid)->tracking(lu.object()));
    applyLocUpdate(lu.object(), query_state->getLocCache(iid), lu);
}

void ServerQueryHandler::cleanupOrphans() {
    // Backwards so we can erase as we go
    for(int i = mCachesForOrphans.size()-1; i >= 0; i--) {
        // If we don't even have the data anymore, delete and move on
        if (mServerQueries.find(mCachesForOrphans[i].first) == mServerQueries.end()) {
            mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
            continue;
        }
        if (mServerQueries[mCachesForOrphans[i].first]->objects.find(mCachesForOrphans[i].second) == mServerQueries[mCachesForOrphans[i].first]->objects.end()) {
            mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
            continue;
        }
        // If the orphan manager still has entries, we need to wait longer
        if (!mServerQueries[mCachesForOrphans[i].first]->getOrphanLocUpdateManager(mCachesForOrphans[i].second)->empty())
            continue;
        // If the loccache has data, then we've gotten something back from it,
        // so we can just remove this entry from our list. It should get cleaned
        // up normally
        if (!mServerQueries[mCachesForOrphans[i].first]->getLocCache(mCachesForOrphans[i].second)->fullyEmpty()) {
            mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
            continue;
        }
        // And in any other case, we need to clean up.
        mServerQueries[mCachesForOrphans[i].first]->removeLocCache(mCachesForOrphans[i].second);
        mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
    }
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
