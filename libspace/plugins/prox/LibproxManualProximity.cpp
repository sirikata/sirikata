// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include "LibproxManualProximity.hpp"

namespace Sirikata {

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

} // namespace Sirikata
