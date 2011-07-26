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
#include <sirikata/proxyobject/ProxyObject.hpp>


namespace Sirikata {

OrphanLocUpdateManager::OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout)
 : PollingService(strand, timeout, ctx, "OrphanLocUpdateManager"),
   mContext(ctx),
   mTimeout(timeout)
{
}

OrphanLocUpdateManager::~OrphanLocUpdateManager() {
    for(ObjectUpdateMap::iterator it = mUpdates.begin(); it != mUpdates.end(); it++) {
        UpdateInfoList& info_list = it->second;
        for(UpdateInfoList::iterator up_it = info_list.begin(); up_it != info_list.end(); up_it++) {
            if (up_it->value != NULL) delete up_it->value;
            if (up_it->opd != NULL) delete up_it->opd;
        }
    }
    mUpdates.clear();
}

void OrphanLocUpdateManager::addOrphanUpdate(const SpaceObjectReference& obj, const LocUpdate& update) {
    UpdateInfoList& info_list = mUpdates[obj];

    UpdateInfo ui;
    ui.value = new LocUpdate(update);
    ui.opd = NULL;
    ui.expiresAt = mContext->simTime() + mTimeout;
    info_list.push_back(ui);
}

void OrphanLocUpdateManager::addUpdateFromExisting(const SpaceObjectReference&obj, ProxyObjectPtr proxyPtr)
{
    UpdateInfoList& info_list = mUpdates[obj];

    UpdateInfo ui;
    ui.value = NULL;

    ui.opd = new OrphanedProxData(proxyPtr->getTimedMotionVector(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_POS_PART),
        proxyPtr->getTimedMotionQuaternion(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_ORIENT_PART),
        proxyPtr->getMesh(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_MESH_PART),
        proxyPtr->getBounds(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_BOUNDS_PART),
        proxyPtr->getPhysics(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_PHYSICS_PART));

    ui.expiresAt = mContext->simTime() + mTimeout;
    info_list.push_back(ui);
}



OrphanLocUpdateManager::UpdateInfoList OrphanLocUpdateManager::getOrphanUpdates(const SpaceObjectReference& obj) {
    UpdateInfoList results;

    ObjectUpdateMap::iterator it = mUpdates.find(obj);
    if (it == mUpdates.end()) return results;

    const UpdateInfoList& info_list = it->second;

    for(UpdateInfoList::const_iterator up_it = info_list.begin(); up_it != info_list.end(); up_it++)
    {
        UpdateInfo upInfo;
        if (up_it->value == NULL)
        {
            upInfo.value = NULL;
            OrphanedProxData* opd = up_it->opd;
            assert(opd != NULL);
            upInfo.opd   = new OrphanedProxData(*opd);
            delete opd;
        }
        else
        {
            assert (up_it->opd == NULL);
            LocUpdate* lu = up_it->value;
            upInfo.value = new LocUpdate(*lu);
            delete lu;
        }
        results.push_back(upInfo);
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
            if (info_list.begin()->value != NULL)
                delete info_list.begin()->value;
            if (info_list.begin()->opd != NULL)
                delete info_list.begin()->opd;

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
