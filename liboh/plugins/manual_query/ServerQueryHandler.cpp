// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ServerQueryHandler.hpp"
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Frame.pbj.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "ManualObjectQueryProcessor.hpp"

#define QPLOG(lvl, msg) SILOG(manual-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Manual {



namespace {

// Helpers for encoding requests
String initRequest() {
    using namespace boost::property_tree;
    try {
        ptree pt;
        pt.put("action", "init");
        std::stringstream data_json;
        write_json(data_json, pt);
        return data_json.str();
    }
    catch(json_parser::json_parser_error exc) {
        QPLOG(detailed, "Failed to encode 'init' request.");
        return "";
    }
}

String destroyRequest() {
    using namespace boost::property_tree;
    try {
        ptree pt;
        pt.put("action", "destroy");
        std::stringstream data_json;
        write_json(data_json, pt);
        return data_json.str();
    }
    catch(json_parser::json_parser_error exc) {
        QPLOG(detailed, "Failed to encode 'destroy' request.");
        return "";
    }
}

String refineRequest(const std::vector<ObjectReference>& aggs) {
    using namespace boost::property_tree;
    try {
        ptree nodes;
        for(uint32 i = 0; i < aggs.size(); i++) {
            nodes.put( aggs[i].toString(), aggs[i].toString() );
        }

        ptree pt;
        pt.put("action", "refine");
        pt.put_child("nodes", nodes);
        std::stringstream data_json;
        write_json(data_json, pt);
        return data_json.str();
    }
    catch(json_parser::json_parser_error exc) {
        QPLOG(detailed, "Failed to encode 'refine' request.");
        return "";
    }
}

String coarsenRequest(const std::vector<ObjectReference>& aggs) {
    using namespace boost::property_tree;
    try {
        ptree nodes;
        for(uint32 i = 0; i < aggs.size(); i++) {
            nodes.put( aggs[i].toString(), aggs[i].toString() );
        }

        ptree pt;
        pt.put("action", "coarsen");
        pt.put_child("nodes", nodes);
        std::stringstream data_json;
        write_json(data_json, pt);
        return data_json.str();
    }
    catch(json_parser::json_parser_error exc) {
        QPLOG(detailed, "Failed to encode 'coarsen' request.");
        return "";
    }
}

} // namespace


using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;
using std::tr1::placeholders::_3;


ServerQueryHandler::ServerQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, Network::IOStrandPtr strand)
 : mContext(ctx),
   mParent(parent),
   mStrand(strand)
{
}

void ServerQueryHandler::start() {
    mContext->objectHost->SpaceNodeSessionManager::addListener(static_cast<SpaceNodeSessionListener*>(this));
}

void ServerQueryHandler::stop() {
    mContext->objectHost->SpaceNodeSessionManager::removeListener(static_cast<SpaceNodeSessionListener*>(this));
    mServerQueries.clear();
}


void ServerQueryHandler::onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {
    QPLOG(detailed, "New space node session " << id);
    assert(mServerQueries.find(id) == mServerQueries.end());

    ServerQueryStatePtr sqs(new ServerQueryState(mContext, mStrand, sn_stream));
    mServerQueries.insert(
        ServerQueryMap::value_type(id, sqs)
    );

    sn_stream->listenSubstream(OBJECT_PORT_LOCATION,
        std::tr1::bind(&ServerQueryHandler::handleLocationSubstream, this,
            id, _1, _2
        )
    );

    mParent->createdServerQuery(id, sqs->objects);
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
            std::tr1::bind(&ServerQueryHandler::writeSomeProxData, this, data)
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

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
            ProxProtocolLocUpdate add(addition);

            ObjectReference observed_oref(addition.object());
            SpaceObjectReference observed(snid.space(), observed_oref);

            bool is_agg = (addition.has_type() && addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate);

            // Store the data
            query_state->objects->objectAdded(
                observed_oref, is_agg,
                add.locationWithLocalTime(mContext->objectHost, snid.space()), add.location_seqno(),
                add.orientationWithLocalTime(mContext->objectHost, snid.space()), add.orientation_seqno(),
                add.bounds(), add.bounds_seqno(),
                Transfer::URI(add.meshOrDefault()), add.mesh_seqno(),
                add.physicsOrDefault(), add.physics_seqno()
            );

            // Replay orphans
            query_state->orphans.invokeOrphanUpdates(snid, observed, this);

            // TODO(ewencp) this is a temporary way to trigger refinement -- we
            // just always refine whenver we see. Obviously this is inefficient
            // and won't scale, but it's useful to get some tests running.
            if (addition.has_type() && addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate)
                sendRefineRequest(serv_it, observed_oref);
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);
            ObjectReference observed_oref(removal.object());
            SpaceObjectReference observed(snid.space(), observed_oref);
            // Backup data for orphans and then destroy
            query_state->orphans.addUpdateFromExisting(observed, query_state->objects->properties(observed_oref));
            query_state->objects->objectRemoved(
                observed_oref
            );

        }

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
void applyLocUpdate(const ObjectReference& objid, OHLocationServiceCachePtr loccache, const LocUpdate& lu, ObjectHost* oh, const SpaceID& space) {
    if (lu.has_location())
        loccache->locationUpdated(objid, lu.locationWithLocalTime(oh, space), lu.location_seqno());
    if (lu.has_orientation())
        loccache->orientationUpdated(objid, lu.orientationWithLocalTime(oh, space), lu.orientation_seqno());
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
        // object yet.
        if (!query_state->objects->tracking(observed_oref)) {
            query_state->orphans.addOrphanUpdate(observed, update);
        }
        else {
            LocProtocolLocUpdate llu(update);
            applyLocUpdate(observed_oref, query_state->objects, llu, mContext->objectHost, snid.space());
        }
    }

    return true;
}


void ServerQueryHandler::onOrphanLocUpdate(const OHDP::SpaceNodeID& observer, const LocUpdate& lu) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(observer);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received proximity message without query. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;

    SpaceObjectReference observed(observer.space(), lu.object());
    assert(query_state->objects->tracking(lu.object()));
    applyLocUpdate(lu.object(), query_state->objects, lu, mContext->objectHost, observer.space());
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
