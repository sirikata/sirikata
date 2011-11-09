// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include "LibproxManualProximity.hpp"

namespace Sirikata {


#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;


LibproxManualProximity::LibproxManualProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : LibproxProximityBase(ctx, locservice, cseg, net, aggmgr)
{
}

LibproxManualProximity::~LibproxManualProximity() {
}

void LibproxManualProximity::start() {
    Proximity::start();
}

void LibproxManualProximity::poll() {
}

void LibproxManualProximity::addQuery(UUID obj, SolidAngle sa, uint32 max_results) {
    // Ignored, this query handler only deals with ObjectHost queries
}

void LibproxManualProximity::addQuery(UUID obj, const String& params) {
    // Ignored, this query handler only deals with ObjectHost queries
}

void LibproxManualProximity::removeQuery(UUID obj) {
    // Ignored, this query handler only deals with ObjectHost queries
}


// Migration management

std::string LibproxManualProximity::migrationClientTag() {
    return "prox";
}

std::string LibproxManualProximity::generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) {
    // There shouldn't be any object data to move since we only manage
    // ObjectHost queries
    return "";
}

void LibproxManualProximity::receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) {
    // We should never be receiving data for migrations since we only
    // handle object host queries
    assert(data.empty());
}

// Object host session and message management

void LibproxManualProximity::onObjectHostSession(const OHDP::NodeID& id, OHDPSST::Stream::Ptr oh_stream) {
    // Setup listener for requests from object hosts. We should only
    // have one active substream at a time. Proximity sessions are
    // always initiated by the object host -- upon receiving a
    // connection we register the query and use the same substream to
    // transmit results.
    oh_stream->listenSubstream(
        OBJECT_PORT_PROXIMITY,
        std::tr1::bind(
            &LibproxManualProximity::handleObjectHostSubstream, this,
            _1, _2
        )
    );
}

void LibproxManualProximity::handleObjectHostSubstream(int success, OHDPSST::Stream::Ptr substream) {
    if (success != SST_IMPL_SUCCESS) return;

    PROXLOG(detailed, "New object host proximity session from " << substream->remoteEndPoint().endPoint);
    // Store this for sending data back
    addObjectHostProxStreamInfo(substream);
    // And register to read requests
    readFramesFromObjectHostStream(
        substream->remoteEndPoint().endPoint.node(),
        std::tr1::bind(&LibproxManualProximity::handleObjectHostProxMessage, this, substream->remoteEndPoint().endPoint.node(), _1)
    );
}

void LibproxManualProximity::handleObjectHostProxMessage(const OHDP::NodeID& id, String& data) {
}

void LibproxManualProximity::onObjectHostSessionEnded(const OHDP::NodeID& id) {
}


} // namespace Sirikata
