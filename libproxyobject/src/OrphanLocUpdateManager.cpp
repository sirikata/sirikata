/*  Sirikata
 *  OrphanLocUpdateManager.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/proxyobject/OrphanLocUpdateManager.hpp>
#include <sirikata/core/service/Context.hpp>
#include "Protocol_Loc.pbj.hpp"

namespace Sirikata {

OrphanLocUpdateManager::OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout)
 : PollingService(strand, timeout, ctx, "OrphanLocUpdateManager"),
   mContext(ctx),
   mTimeout(timeout)
{
}

OrphanLocUpdateManager::~OrphanLocUpdateManager() {
    for(ObjectUpdateMap::iterator it = mUpdates.begin(); it != mUpdates.end(); ) {
        UpdateInfoList& info_list = it->second;
        for(UpdateInfoList::iterator up_it = info_list.begin(); up_it != info_list.end(); up_it++)
            delete up_it->value;
    }
    mUpdates.clear();
}

void OrphanLocUpdateManager::addOrphanUpdate(const SpaceObjectReference& obj, const LocUpdate& update) {
    UpdateInfoList& info_list = mUpdates[obj];

    UpdateInfo ui;
    ui.value = new LocUpdate(update);
    ui.expiresAt = mContext->simTime() + mTimeout;
    info_list.push_back(ui);
}

OrphanLocUpdateManager::UpdateList OrphanLocUpdateManager::getOrphanUpdates(const SpaceObjectReference& obj) {
    UpdateList results;

    ObjectUpdateMap::iterator it = mUpdates.find(obj);
    if (it == mUpdates.end()) return results;
    const UpdateInfoList& info_list = it->second;

    for(UpdateInfoList::const_iterator up_it = info_list.begin(); up_it != info_list.end(); up_it++) {
        LocUpdate* lu = up_it->value;
        results.push_back(*lu);
        delete lu;
    }

    mUpdates.erase(it);
    return results;
}

void OrphanLocUpdateManager::poll() {
    Time now = mContext->simTime();
    // Scan through all updates looking for outdated ones
    for(ObjectUpdateMap::iterator it = mUpdates.begin(); it != mUpdates.end(); ) {
        UpdateInfoList& info_list = it->second;
        while(!info_list.empty() && info_list.begin()->expiresAt < now) {
            delete info_list.begin()->value;
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
