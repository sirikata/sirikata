// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ServerQueryHandler.hpp"
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Frame.pbj.hpp"

#include "ManualObjectQueryProcessor.hpp"
#include <sirikata/oh/OHSpaceTimeSynced.hpp>

#define QPLOG(lvl, msg) SILOG(manual-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Manual {

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

    ServerQueryStatePtr sqs(new ServerQueryState(this, id, mContext, mStrand, sn_stream));
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
    if (is_new) {
        QPLOG(detailed, "Initializing server query to " << serv_it->first);
        serv_it->second->client.initQuery();
    }
}

void ServerQueryHandler::decrementServerQuery(OHDP::SpaceNodeID node) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(node);
    assert(serv_it != mServerQueries.end());

    serv_it->second->nconnected--;
    if (!serv_it->second->canRemove()) return;

    QPLOG(detailed, "Destroying server query to " << serv_it->first);
    serv_it->second->client.destroyQuery();
}


void ServerQueryHandler::onCreatedReplicatedIndex(Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>* client, const OHDP::SpaceNodeID& snid, ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects) {
    mParent->createdReplicatedIndex(snid, proxid, loccache, sid, dynamic_objects);
}

void ServerQueryHandler::onDestroyedReplicatedIndex(Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>* client, const OHDP::SpaceNodeID& snid, ProxIndexID proxid) {
    mParent->removedReplicatedIndex(snid, proxid);
}

void ServerQueryHandler::sendReplicatedClientProxMessage(Pinto::Manual::ReplicatedClientWithID<OHDP::SpaceNodeID>* client, const OHDP::SpaceNodeID& snid, const String& msg) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    // This is directly from the Manual ReplicatedClient, so it should
    // definitely still exist
    assert(serv_it != mServerQueries.end());

    sendProxMessage(serv_it, msg);
}



// Proximity

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

    serv_it->second->client.proxUpdate(contents);
}



// Proximity tracking updates in local queries to trigger adjustments to server query
void ServerQueryHandler::queriersAreObserving(const OHDP::SpaceNodeID& snid, const ObjectReference& objid) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received update about object queries on server query that doesn't exist. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;
    query_state->client.queriersAreObserving(objid);
}

void ServerQueryHandler::queriersStoppedObserving(const OHDP::SpaceNodeID& snid, const ObjectReference& objid) {
    ServerQueryMap::iterator serv_it = mServerQueries.find(snid);
    if (serv_it == mServerQueries.end()) {
        QPLOG(debug, "Received update about object queries on server query that doesn't exist. Query may have recently been destroyed.");
        return;
    }
    ServerQueryStatePtr& query_state = serv_it->second;
    query_state->client.queriersStoppedObserving(objid);
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
        query_state->client.locUpdate(update);
    }

    return true;
}



} // namespace Manual
} // namespace OH
} // namespace Sirikata
