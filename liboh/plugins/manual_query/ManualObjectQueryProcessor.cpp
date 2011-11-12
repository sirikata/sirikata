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
}

void ManualObjectQueryProcessor::onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {
    QPLOG(detailed, "New space node session " << id);
    assert(mServerQueries.find(id) == mServerQueries.end());
    mServerQueries.insert( ServerQueryMap::value_type(id, ServerQueryState(sn_stream)) );

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
    bool is_new = (serv_it->second.nconnected == 0);
    serv_it->second.nconnected++;
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

        serv_it->second.base_stream->createChildStream(
            std::tr1::bind(&ManualObjectQueryProcessor::handleCreatedProxSubstream, this, serv_it->first, _1, _2),
            (void*)framed.c_str(), framed.size(),
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY
        );
    }
}

void ManualObjectQueryProcessor::decrementServerQuery(ServerQueryMap::iterator serv_it) {
    serv_it->second.nconnected--;
    if (!serv_it->second.canRemove()) return;

    // FIXME send message
    QPLOG(detailed, "Destroying server query to " << serv_it->first);
    if (serv_it->second.prox_stream) {
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
        serv_it->second.prox_stream->write((const uint8*)framed.c_str(), framed.size());
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

void ManualObjectQueryProcessor::handleCreatedProxSubstream(const OHDP::SpaceNodeID& snid, int success, OHDPSST::Stream::Ptr prox_stream) {
    if (success != SST_IMPL_SUCCESS)
        QPLOG(error, "Failed to create proximity substream to " << snid);

    // Save the stream for additional writing
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    // We may have lost the session since we requested the connection
    if (serv_it == mServerQueries.end()) return;
    serv_it->second.prox_stream = prox_stream;

    // Register to get data
    String* prevdata = new String();
    prox_stream->registerReadCallback(
        std::tr1::bind(&ManualObjectQueryProcessor::handleProximitySubstreamRead, this,
            snid, prox_stream, prevdata, _1, _2
        )
    );
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
        QPLOG(detailed, "Got proximity message.");
        // FIXME handleProximityMessage(snid, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //prox_stream->registerReadCallback(0);
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

bool ManualObjectQueryProcessor::handleLocationMessage(const OHDP::SpaceNodeID& snid, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    QPLOG(detailed, "Got location message.");
    // FIXME figure out who to forward to, or store for later use

    return true;
}



} // namespace Manual
} // namespace OH
} // namespace Sirikata
