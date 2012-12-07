// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "MasterPintoServerQuerier.hpp"

#include "Protocol_MasterPinto.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"

#include <json_spirit/json_spirit.h>

#include <sirikata/pintoloc/TimeSynced.hpp>
#include <sirikata/pintoloc/ProtocolLocUpdate.hpp>

using namespace Sirikata::Network;

#define MP_LOG(lvl,msg) SILOG(masterpinto,lvl, msg);

namespace Sirikata {

MasterPintoServerQuerier::MasterPintoServerQuerier(SpaceContext* ctx, const String& params)
 : MasterPintoServerQuerierBase(ctx, params),
   mAggregateQuery(SolidAngle::Max),
   mAggregateQueryMaxResults(0),
   mAggregateQueryDirty(true)
{
}

MasterPintoServerQuerier::~MasterPintoServerQuerier() {
}


void MasterPintoServerQuerier::updateQuery(const String& update) {
    // Need to parse out parameters
    if (update.empty())
        return;
    namespace json = json_spirit;
    json::Value parsed;
    if (!json::read(update, parsed)) {
        MP_LOG(error, "Invalid query update request from Proximity.");
        return;
    }
    SolidAngle min_angle(parsed.getReal("angle", SolidAngle::Max.asFloat()));
    uint32 max_results = parsed.getInt("max_result", 0);

    MP_LOG(debug, "Updating aggregate query angle " << min_angle << " and " << max_results << " maximum results.");
    mAggregateQuery = min_angle;
    mAggregateQueryMaxResults = max_results;
    mAggregateQueryDirty = true;
    mIOStrand->post(
        std::tr1::bind(&MasterPintoServerQuerier::updatePintoQuery, this),
        "MasterPintoServerQuerier::updatePintoQuery"
    );
}

void MasterPintoServerQuerier::onConnected() {
    // For a new connection, make sure we try to send any dirty data
    mAggregateQueryDirty = true;
    mIOStrand->post(
        std::tr1::bind(&MasterPintoServerQuerier::updatePintoQuery, this),
        "MasterPintoServerQuerier::updatePintoQuery"
    );
}

void MasterPintoServerQuerier::updatePintoQuery() {
    if (!mAggregateQueryDirty)
        return;

    mAggregateQueryDirty = false;
    Sirikata::Protocol::MasterPinto::QueryUpdate update;
    update.set_min_angle(mAggregateQuery.asFloat());
    update.set_max_count(mAggregateQueryMaxResults);
    sendQueryUpdate(serializePBJMessage(update), DoNotQueueUpdate);
}

void MasterPintoServerQuerier::onPintoData(const String& data) {
    Sirikata::Protocol::MasterPinto::PintoResponse msg;
    bool parsed = parsePBJMessage(&msg, data);

    if (!parsed) {
        MP_LOG(error, "Couldn't parse response from server.");
        return;
    }

    for(int32 idx = 0; idx < msg.update_size(); idx++) {
        Sirikata::Protocol::MasterPinto::PintoUpdate update = msg.update(idx);
        // Translate to Proximity message
        Sirikata::Protocol::Prox::ProximityUpdate prox_update;
        for(int32 i = 0; i < update.change_size(); i++) {
            Sirikata::Protocol::MasterPinto::PintoResult result = update.change(i);
            MP_LOG(debug, "Event received from master pinto: " << result.server() << (result.addition() ? " added" : " removed"));
            // We need to shoe-horn the server ID into the update's object ID
            UUID server_as_objid((int32)result.server());
            if (result.addition()) {
                Sirikata::Protocol::Prox::IObjectAddition prox_add = prox_update.add_addition();
                prox_add.set_object(server_as_objid);
            }
            else {
                Sirikata::Protocol::Prox::IObjectRemoval prox_rem = prox_update.add_removal();
                prox_rem.set_object(server_as_objid);
            }
        }
        notify(&PintoServerQuerierListener::onPintoServerResult, prox_update);
    }

    if (msg.has_loc_updates()) {
        Sirikata::Protocol::Loc::BulkLocationUpdate bu = msg.loc_updates();
        for(int32 li = 0; li < bu.update_size(); li++) {
	  //llvm-based compilers want a temporary variable to avoid a copy constructor call
            NopTimeSynced nop_ts;
            LocProtocolLocUpdate tmp(bu.update(li), nop_ts);
            // notify wants to only pass through values and tries to copy the
            // LocProtocolLocUpdate by default -- we need to jump through hoops and
            // specify the exact template types explicitly to make this work
            notify<void(PintoServerQuerierListener::*)(const Sirikata::LocUpdate&), const LocProtocolLocUpdate&>(&PintoServerQuerierListener::onPintoServerLocUpdate, tmp);
        }
    }
}

} // namespace Sirikata
