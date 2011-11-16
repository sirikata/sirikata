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
#include <sirikata/oh/LocUpdate.hpp>
#include <sirikata/oh/ProtocolLocUpdate.hpp>

namespace Sirikata {

namespace {
/** Implementation of LocUpdate which collects its information from
 *  stored OrphanedProxData.
 */
class ProxDataLocUpdate : public LocUpdate {
public:
    ProxDataLocUpdate(const OrphanLocUpdateManager::OrphanedProxData& lu)
     : mUpdate(lu)
    {}
    virtual ~ProxDataLocUpdate() {}

    virtual ObjectReference object() const { return mUpdate.object; }

    // Location
    virtual bool has_location() const { return true; }
    virtual TimedMotionVector3f location() const { return mUpdate.timedMotionVector; }
    virtual uint64 location_seqno() const { return mUpdate.tmvSeqNo; }

    // Orientation
    virtual bool has_orientation() const { return true; }
    virtual TimedMotionQuaternion orientation() const { return mUpdate.timedMotionQuat; }
    virtual uint64 orientation_seqno() const { return mUpdate.tmqSeqNo; }

    // Bounds
    virtual bool has_bounds() const { return true; }
    virtual BoundingSphere3f bounds() const { return mUpdate.bounds; }
    virtual uint64 bounds_seqno() const { return mUpdate.bndsSeqNo; }

    // Mesh
    virtual bool has_mesh() const { return true; }
    virtual String mesh() const { return mUpdate.mesh.toString(); }
    virtual uint64 mesh_seqno() const { return mUpdate.meshSeqNo; }

    // Physics
    virtual bool has_physics() const { return true; }
    virtual String physics() const { return mUpdate.physics; }
    virtual uint64 physics_seqno() const { return mUpdate.physSeqNo; }
private:
    ProxDataLocUpdate();

    const OrphanLocUpdateManager::OrphanedProxData& mUpdate;
};
} // namespace

OrphanLocUpdateManager::UpdateInfo::~UpdateInfo() {
    if (value != NULL)
        delete value;
    if (opd != NULL)
        delete opd;
}

LocUpdate* OrphanLocUpdateManager::UpdateInfo::getLocUpdate() const {
    if (value != NULL)
        return new ProtocolLocUpdate(*value);
    else
        return new ProxDataLocUpdate(*opd);
}

OrphanLocUpdateManager::OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout)
 : PollingService(strand, timeout, ctx, "OrphanLocUpdateManager"),
   mContext(ctx),
   mTimeout(timeout)
{

}

void OrphanLocUpdateManager::addOrphanUpdate(const SpaceObjectReference& obj, const Sirikata::Protocol::Loc::LocationUpdate& update) {
    UpdateInfoList& info_list = mUpdates[obj];
    info_list.push_back(
        UpdateInfoPtr(new UpdateInfo(new Sirikata::Protocol::Loc::LocationUpdate(update), mContext->simTime() + mTimeout))
    );
}

void OrphanLocUpdateManager::addUpdateFromExisting(
    const SpaceObjectReference& obj,
    const TimedMotionVector3f& loc,
    uint64 loc_seqno,
    const TimedMotionQuaternion& orient,
    uint64 orient_seqno,
    const BoundingSphere3f& bounds,
    uint64 bounds_seqno,
    const Transfer::URI& mesh,
    uint64 mesh_seqno,
    const String& physics,
    uint64 physics_seqno
) {
    UpdateInfoList& info_list = mUpdates[obj];

    OrphanedProxData* opd = new OrphanedProxData(
        obj.object(),
        loc, loc_seqno,
        orient, orient_seqno,
        mesh, mesh_seqno,
        bounds, bounds_seqno,
        physics, physics_seqno
    );
    info_list.push_back( UpdateInfoPtr(new UpdateInfo(opd, mContext->simTime() + mTimeout)) );
}

void OrphanLocUpdateManager::addUpdateFromExisting(const SpaceObjectReference&obj, ProxyObjectPtr proxyPtr) {
    addUpdateFromExisting(
        proxyPtr->getObjectReference(),
        proxyPtr->getTimedMotionVector(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_POS_PART),
        proxyPtr->getTimedMotionQuaternion(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_ORIENT_PART),
        proxyPtr->getBounds(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_BOUNDS_PART),
        proxyPtr->getMesh(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_MESH_PART),
        proxyPtr->getPhysics(),
        proxyPtr->getUpdateSeqNo(ProxyObject::LOC_PHYSICS_PART)
    );
}



OrphanLocUpdateManager::UpdateInfoList OrphanLocUpdateManager::getOrphanUpdates(const SpaceObjectReference& obj) {
    UpdateInfoList results;

    ObjectUpdateMap::iterator it = mUpdates.find(obj);
    if (it == mUpdates.end()) return results;

    UpdateInfoList& info_list = it->second;
    results.swap(info_list);

    mUpdates.erase(it);
    return results;
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
