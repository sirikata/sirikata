/*  Sirikata
 *  OrphanLocUpdateManager.hpp
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

#ifndef _SIRIKATA_ORPHAN_LOC_UPDATE_MANAGER_HPP_
#define _SIRIKATA_ORPHAN_LOC_UPDATE_MANAGER_HPP_

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/Defs.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/BoundingSphere.hpp>


namespace Sirikata {

class LocUpdate;
namespace Protocol {
namespace Loc {
class LocationUpdate;
}
}

/** OrphanLocUpdateManager tracks location updates/information for objects,
 *  making sure that location information does not get lost due to reordering of
 *  proximity and location messages from the space server. It currently handles
 *  two types of errors.
 *
 *  First, if a location message is received before the corresponding proximity
 *  addition event, it is stored for awhile in case the addition is received
 *  soon. (This is the reason for the name of the class -- the location update
 *  is 'orphaned' since it doesn't have a parent proximity addition.)
 *
 *  The second type of issue is when the messages that should be received are
 *  (remove object X, add object X, update location of X), but it is received as
 *  (update location of X, remove object X, add object X). In this case the
 *  update would have been applied and then lost. For this case, we save the
 *  state of objects for a short while after they are removed, including the
 *  sequence numbers. Then, we can reapply them after objects are re-added, just
 *  as we would with an orphaned update.
 *
 *  Loc updates are saved for short time and, if they aren't needed, are
 *  discarded. In all cases, sequence numbers are still used so possibly trying
 *  to apply old updates isn't an issue.
 */
class SIRIKATA_PROXYOBJECT_EXPORT OrphanLocUpdateManager : public PollingService {
public:
    OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout);

    /** Add an orphan update to the queue and set a timeout for it to be cleared
     *  out.
     */
    void addOrphanUpdate(const SpaceObjectReference& obj, const Sirikata::Protocol::Loc::LocationUpdate& update);
    /**
       Take all fields in proxyPtr, and create an OrphanedProxData struct from
       them.
     */
    void addUpdateFromExisting(const SpaceObjectReference&obj, ProxyObjectPtr proxyPtr);
    /** Take all parameters from an object to backup for a short time.
     */
    void addUpdateFromExisting(
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
    );

    struct UpdateInfo;
    typedef std::tr1::shared_ptr<UpdateInfo> UpdateInfoPtr;
    typedef std::vector<UpdateInfoPtr> UpdateInfoList;


    /** Gets all orphan updates for a given object. */
    UpdateInfoList getOrphanUpdates(const SpaceObjectReference& obj);

    void setFinalCallback(const std::tr1::function<void()>&);

    //When we get a prox removal, we take all the data that was stored in the
    //corresponding proxy object and put it into an OrphanedProxData
    struct OrphanedProxData
    {
        OrphanedProxData(
            const ObjectReference& oref,
            const TimedMotionVector3f& tmv, uint64 tmv_seq_no, const TimedMotionQuaternion& tmq, uint64 tmq_seq_no, const Transfer::URI& uri_mesh, uint64 mesh_seq_no, const BoundingSphere3f& bounding_sphere, uint64 bnds_seq_no, const String& phys, uint64 phys_seq_no)
         : object(oref),
           timedMotionVector(tmv),
           tmvSeqNo(tmv_seq_no),
           timedMotionQuat(tmq),
           tmqSeqNo(tmq_seq_no),
           mesh(uri_mesh),
           meshSeqNo(mesh_seq_no),
           bounds(bounding_sphere),
           bndsSeqNo(bnds_seq_no),
           physics(phys),
           physSeqNo(phys_seq_no)
        {}

        ~OrphanedProxData()
        {}


        ObjectReference object;

        TimedMotionVector3f timedMotionVector;
        uint64 tmvSeqNo;

        TimedMotionQuaternion timedMotionQuat;
        uint64 tmqSeqNo;

        Transfer::URI mesh;
        uint64 meshSeqNo;

        BoundingSphere3f bounds;
        uint64 bndsSeqNo;

        String physics;
        uint64 physSeqNo;
    };

    struct UpdateInfo {
        UpdateInfo(Sirikata::Protocol::Loc::LocationUpdate* _v, const Time& t)
         : value(_v), opd(NULL), expiresAt(t)
        {}
        UpdateInfo(OrphanedProxData* _v, const Time& t)
         : value(NULL), opd(_v), expiresAt(t)
        {}
        ~UpdateInfo();

        //Either value or opd will be non-null.  Never both.
        Sirikata::Protocol::Loc::LocationUpdate* value;
        OrphanedProxData* opd;
        Time expiresAt;

        LocUpdate* getLocUpdate() const;
    private:
        UpdateInfo();
    };


private:
    virtual void poll();


    typedef std::tr1::unordered_map<SpaceObjectReference, UpdateInfoList, SpaceObjectReference::Hasher> ObjectUpdateMap;

    Context* mContext;
    Duration mTimeout;
    ObjectUpdateMap mUpdates;
}; // class OrphanLocUpdateManager


} // namespace Sirikata

#endif
