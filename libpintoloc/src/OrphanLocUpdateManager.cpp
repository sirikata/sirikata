// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/pintoloc/OrphanLocUpdateManager.hpp>
#include <sirikata/core/service/Context.hpp>
#include "Protocol_Loc.pbj.hpp"
#include <sirikata/proxyobject/ProxyObject.hpp>

namespace Sirikata {

OrphanLocUpdateManager::UpdateInfo::~UpdateInfo() {
    if (value != NULL)
        delete value;
    if (opd != NULL)
        delete opd;
}

OrphanLocUpdateManager::OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout)
 : PollingService(strand, "OrphanLocUpdateManager Poll", timeout, ctx, "OrphanLocUpdateManager"),
   mContext(ctx),
   mTimeout(timeout)
{

}

void OrphanLocUpdateManager::addOrphanUpdate(const SpaceObjectReference& observed, const Sirikata::Protocol::Loc::LocationUpdate& update) {
    assert( ObjectReference(update.object()) == observed.object() );
    UpdateInfoList& info_list = mUpdates[observed];
    info_list.push_back(
        UpdateInfoPtr(new UpdateInfo(observed, new Sirikata::Protocol::Loc::LocationUpdate(update), mContext->simTime() + mTimeout))
    );
}

void OrphanLocUpdateManager::addUpdateFromExisting(
    const SpaceObjectReference& observed,
    const SequencedPresenceProperties& props
) {
    UpdateInfoList& info_list = mUpdates[observed];

    SequencedPresenceProperties* opd = new SequencedPresenceProperties(props);
    info_list.push_back( UpdateInfoPtr(new UpdateInfo(observed, opd, mContext->simTime() + mTimeout)) );
}

void OrphanLocUpdateManager::addUpdateFromExisting(ProxyObjectPtr proxyPtr) {
    addUpdateFromExisting(
        proxyPtr->getObjectReference(),
        *(dynamic_cast<SequencedPresenceProperties*>(proxyPtr.get()))
    );
}

void OrphanLocUpdateManager::poll() {
    Time now = mContext->simTime();
    // Scan through all updates looking for outdated ones
    for(ObjectUpdateMap::iterator it = mUpdates.begin(); it != mUpdates.end(); ) {
        UpdateInfoList& info_list = it->second;
        while(!info_list.empty() && (*info_list.begin())->expiresAt < now) {
            info_list.erase(info_list.begin());
        }

        ObjectUpdateMap::iterator next_it = it;
        next_it++;
        if (info_list.empty())
            mUpdates.erase(it);
        it = next_it;
    }
}

} // namespace Sirikata
