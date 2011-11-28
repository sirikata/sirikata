// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualObjectQueryProcessor.hpp"
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Frame.pbj.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define QPLOG(lvl, msg) SILOG(manual-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Manual {

using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;
using std::tr1::placeholders::_3;


ManualObjectQueryProcessor* ManualObjectQueryProcessor::create(ObjectHostContext* ctx, const String& args) {
    return new ManualObjectQueryProcessor(ctx);
}


ManualObjectQueryProcessor::ManualObjectQueryProcessor(ObjectHostContext* ctx)
 : mContext(ctx)
{
}

ManualObjectQueryProcessor::~ManualObjectQueryProcessor() {
}

void ManualObjectQueryProcessor::start() {
    mContext->objectHost->SpaceNodeSessionManager::addListener(static_cast<SpaceNodeSessionListener*>(this));
    mContext->objectHost->ObjectNodeSessionProvider::addListener(static_cast<ObjectNodeSessionListener*>(this));
}

void ManualObjectQueryProcessor::stop() {
    mContext->objectHost->ObjectNodeSessionProvider::removeListener(static_cast<ObjectNodeSessionListener*>(this));
    mContext->objectHost->SpaceNodeSessionManager::removeListener(static_cast<SpaceNodeSessionListener*>(this));

    mObjectState.clear();
    mServerQueries.clear();
}

void ManualObjectQueryProcessor::onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {
    QPLOG(detailed, "New space node session " << id);
    assert(mServerQueries.find(id) == mServerQueries.end());
    mServerQueries.insert(
        ServerQueryMap::value_type(
            id,
            ServerQueryStatePtr(new ServerQueryState(mContext, sn_stream))
        )
    );

    sn_stream->listenSubstream(OBJECT_PORT_LOCATION,
        std::tr1::bind(&ManualObjectQueryProcessor::handleLocationSubstream, this,
            id, _1, _2
        )
    );
}

void ManualObjectQueryProcessor::onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) {
    mServerQueries.erase(id);
}

void ManualObjectQueryProcessor::onObjectNodeSession(const SpaceID& space, const ObjectReference& oref, const OHDP::NodeID& id) {
    QPLOG(detailed, "New object-space node session: " << oref << " connected to " << space << "-" << id);

    OHDP::SpaceNodeID snid(space, id);
    SpaceObjectReference sporef(space, oref);

    ObjectStateMap::iterator obj_it = mObjectState.find(sporef);

    // First, clear out old state
    if (obj_it != mObjectState.end() && obj_it->second.node != OHDP::NodeID::null()) {
        OHDP::SpaceNodeID old_snid(obj_it->first.space(), obj_it->second.node);
        ServerQueryMap::iterator old_serv_it = mServerQueries.find(old_snid);
        assert(old_serv_it != mServerQueries.end());
        decrementServerQuery(old_serv_it);
    }

    // Update state
    // Object no longer has a session, it's no longer relevant
    if (id == OHDP::NodeID::null()) {
        mObjectState.erase(obj_it);
        return;
    }
    // Otherwise it's new/migrating
    if (obj_it == mObjectState.end())
        obj_it = mObjectState.insert( ObjectStateMap::value_type(sporef, ObjectState()) ).first;
    obj_it->second.node = id;
    // Figure out if we need to do anything on the
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    assert(serv_it != mServerQueries.end());
    incrementServerQuery(serv_it);
}

void ManualObjectQueryProcessor::incrementServerQuery(ServerQueryMap::iterator serv_it) {
    bool is_new = (serv_it->second->nconnected == 0);
    serv_it->second->nconnected++;
    updateServerQuery(serv_it, is_new);
}

void ManualObjectQueryProcessor::updateServerQuery(ServerQueryMap::iterator serv_it, bool is_new) {
    if (is_new) {
        QPLOG(detailed, "Initializing server query to " << serv_it->first);

        Protocol::Prox::QueryRequest request;
        using namespace boost::property_tree;
        try {
            ptree pt;
            pt.put("action", "init");
            std::stringstream data_json;
            write_json(data_json, pt);
            request.set_query_parameters(data_json.str());
        }
        catch(json_parser::json_parser_error exc) {
            QPLOG(detailed, "Failed to encode 'init' request.");
            return;
        }
        std::string init_msg_str = serializePBJMessage(request);

        String framed = Network::Frame::write(init_msg_str);
        sendProxMessage(serv_it, framed);

    }
}

void ManualObjectQueryProcessor::decrementServerQuery(ServerQueryMap::iterator serv_it) {
    serv_it->second->nconnected--;
    if (!serv_it->second->canRemove()) return;

    // FIXME send message
    QPLOG(detailed, "Destroying server query to " << serv_it->first);
    if (serv_it->second->prox_stream) {
        Protocol::Prox::QueryRequest request;

        using namespace boost::property_tree;
        try {
            ptree pt;
            pt.put("action", "destroy");
            std::stringstream data_json;
            write_json(data_json, pt);
            request.set_query_parameters(data_json.str());
        }
        catch(json_parser::json_parser_error exc) {
            QPLOG(detailed, "Failed to encode 'destroy' request.");
            return;
        }
        std::string destroy_msg_str = serializePBJMessage(request);
        String framed = Network::Frame::write(destroy_msg_str);
        serv_it->second->prox_stream->write((const uint8*)framed.c_str(), framed.size());
    }
}

void ManualObjectQueryProcessor::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) {
    ObjectStateMap::iterator it = mObjectState.find(sporef);

    if (new_query.empty()) { // Cancellation
        // Remove from server query
        // FIXME
        // Clear object state if possible
        if (it->second.canRemove())
            mObjectState.erase(it);
    }
    else { // Init or update
        // Track state
        it->second.query = new_query;
        // Update server query
        // FIXME
    }
}


// Proximity

void ManualObjectQueryProcessor::sendProxMessage(ServerQueryMap::iterator serv_it, const String& msg) {
    serv_it->second->outstanding.push(msg);

    // Possibly trigger writing

    // Already in progress
    if (serv_it->second->writing) return;

    // Without a stream, we need to first create the stream
    if (!serv_it->second->prox_stream) {
        serv_it->second->writing = true;

        serv_it->second->base_stream->createChildStream(
            std::tr1::bind(&ManualObjectQueryProcessor::handleCreatedProxSubstream, this, serv_it->first, _1, _2),
            NULL, 0,
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
        );
        return;
    }

    // Otherwise, we're clear for starting writing
    writeSomeProxData(serv_it->second);
}

void ManualObjectQueryProcessor::writeSomeProxData(ServerQueryStatePtr data) {
    assert(data->prox_stream);
    data->writing = true;

    while(!data->outstanding.empty()) {
        String& msg = data->outstanding.front();
        int bytes_written = data->prox_stream->write((const uint8*)msg.data(), msg.size());
        if (bytes_written < 0) {
            // FIXME
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
            std::tr1::bind(&ManualObjectQueryProcessor::writeSomeProxData, this, data)
        );
    }
}

void ManualObjectQueryProcessor::handleCreatedProxSubstream(const OHDP::SpaceNodeID& snid, int success, OHDPSST::Stream::Ptr prox_stream) {
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
        std::tr1::bind(&ManualObjectQueryProcessor::handleProximitySubstreamRead, this,
            snid, prox_stream, prevdata, _1, _2
        )
    );

    // We created the stream to start writing, so start that now
    assert(serv_it->second->writing == true);
    writeSomeProxData(serv_it->second);
}

void ManualObjectQueryProcessor::handleProximitySubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr prox_stream, String* prevdata, uint8* buffer, int length) {
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

void ManualObjectQueryProcessor::handleProximityMessage(const OHDP::SpaceNodeID& snid, const String& payload) {
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

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
            ProxProtocolLocUpdate add(addition);

            SpaceObjectReference observed(snid.space(), ObjectReference(addition.object()));

            // Store the data
            assert(query_state->objects.find(observed) == query_state->objects.end());
            SequencedPresenceProperties& props = query_state->objects[observed];
            props.setLocation(add.locationWithLocalTime(mContext->objectHost, snid.space()), add.location_seqno());
            props.setOrientation(add.orientationWithLocalTime(mContext->objectHost, snid.space()), add.orientation_seqno());
            props.setBounds(add.bounds(), add.bounds_seqno());
            props.setMesh(Transfer::URI(add.meshOrDefault()), add.mesh_seqno());
            props.setPhysics(add.physicsOrDefault(), add.physics_seqno());

            // Replay orphans
            query_state->orphans.invokeOrphanUpdates(snid, observed, this);
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);
            SpaceObjectReference observed(snid.space(), ObjectReference(removal.object()));
            assert(query_state->objects.find(observed) != query_state->objects.end());
            // Backup data for orphans and then destroy
            query_state->orphans.addUpdateFromExisting(observed, query_state->objects[observed]);
            query_state->objects.erase(observed);
        }
    }

}


// Location


void ManualObjectQueryProcessor::handleLocationSubstream(const OHDP::SpaceNodeID& snid, int err, OHDPSST::Stream::Ptr s) {
    s->registerReadCallback(
        std::tr1::bind(
            &ManualObjectQueryProcessor::handleLocationSubstreamRead, this,
            snid, s, new std::stringstream(), _1, _2
        )
    );
}

void ManualObjectQueryProcessor::handleLocationSubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr s, std::stringstream* prevdata, uint8* buffer, int length) {
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
void applyLocUpdate(SequencedPresenceProperties& props, const LocUpdate& lu, ObjectHost* oh, const SpaceID& space) {
    if (lu.has_location())
        props.setLocation(lu.locationWithLocalTime(oh, space), lu.location_seqno());
    if (lu.has_orientation())
        props.setOrientation(lu.orientationWithLocalTime(oh, space), lu.orientation_seqno());
    if (lu.has_bounds())
        props.setBounds(lu.bounds(), lu.bounds_seqno());
    if (lu.has_mesh())
        props.setMesh(Transfer::URI(lu.meshOrDefault()), lu.mesh_seqno());
    if (lu.has_physics())
        props.setPhysics(lu.physicsOrDefault(), lu.physics_seqno());
}
}

bool ManualObjectQueryProcessor::handleLocationMessage(const OHDP::SpaceNodeID& snid, const std::string& payload) {
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
        SpaceObjectReference observed(snid.space(), ObjectReference(update.object()));

        ServerQueryState::ObjectPropertiesMap::iterator props_it = query_state->objects.find(observed);

        // Because of prox/loc ordering, we may or may not have a record of the
        // object yet.
        if (props_it == query_state->objects.end()) {
            query_state->orphans.addOrphanUpdate(observed, update);
        }
        else {
            LocProtocolLocUpdate llu(update);
            applyLocUpdate(props_it->second, llu, mContext->objectHost, snid.space());
        }
    }

    return true;
}

void ManualObjectQueryProcessor::onOrphanLocUpdate(const OHDP::SpaceNodeID& observer, const LocUpdate& lu) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(observer);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received proximity message without query. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    SpaceObjectReference observed(observer.space(), lu.object());
    assert(query_state->objects.find(observed) != query_state->objects.end());
    SequencedPresenceProperties& props = query_state->objects[observed];
    applyLocUpdate(props, lu, mContext->objectHost, observer.space());
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
