// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "MasterPintoManualServerQuerier.hpp"
#include "Protocol_Prox.pbj.hpp"
#include <json_spirit/json_spirit.h>

using namespace Sirikata::Network;

#define MP_LOG(lvl,msg) SILOG(masterpinto,lvl, msg);

namespace Sirikata {

MasterPintoManualServerQuerier::MasterPintoManualServerQuerier(SpaceContext* ctx, const String& params)
 : MasterPintoServerQuerierBase(ctx, params)
{
}

MasterPintoManualServerQuerier::~MasterPintoManualServerQuerier() {
}


void MasterPintoManualServerQuerier::updateQuery(const String& update) {
    // The client of this class will manage a replicated tree. It'll generate
    // update requests that are commands for refining/coarsening the tree. In
    // this case we can just forward the requests directly since it should
    // already be properly json encoded.

    if (update.empty())
        return;

    mIOStrand->post(
        std::tr1::bind(&MasterPintoManualServerQuerier::sendQueryUpdate, this, update, QueueUpdate),
        "MasterPintoManualServerQuerier::updatePintoQuery"
    );
}

void MasterPintoManualServerQuerier::onPintoData(const String& data) {
    Sirikata::Protocol::Prox::ProximityResults msg;
    bool parsed = parsePBJMessage(&msg, data);

    if (!parsed) {
        MP_LOG(error, "Couldn't parse response from server.");
        return;
    }

    // The manual master pinto sends proximity updates directly, we can just
    // pass them along
    for(int32 idx = 0; idx < msg.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate prox_update = msg.update(idx);
        notify(&PintoServerQuerierListener::onPintoServerResult, prox_update);
    }
}

} // namespace Sirikata
