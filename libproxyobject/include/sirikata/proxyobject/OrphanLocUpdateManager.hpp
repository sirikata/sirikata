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

namespace Protocol {
namespace Loc {
class LocationUpdate;
}
}

/** OrphanLocUpdateManager keeps track of loc updates that arrived before the
 *  corresponding prox update so they can be delivered when the prox update does
 *  arrive. Without an OrphanLocUpdateManager, a loc update could be lost due to
 *  incorrect ordering, resulting in the correct information never being updated
 *  to its correct value.  Loc updates are saved for short time and then
 *  discarded.
 */
class SIRIKATA_PROXYOBJECT_EXPORT OrphanLocUpdateManager : public PollingService {
public:
    typedef Sirikata::Protocol::Loc::LocationUpdate LocUpdate;
    
    OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout);
    ~OrphanLocUpdateManager();

    /** Add an orphan update to the queue and set a timeout for it to be cleared
     *  out.
     */
    void addOrphanUpdate(const SpaceObjectReference& obj, const LocUpdate& update);
    /**
       Take all fields in proxyPtr, and create an OrphanedProxData struct from
       them.  
     */
    void addUpdateFromExisting(const SpaceObjectReference&obj, ProxyObjectPtr proxyPtr);

    struct UpdateInfo;
    typedef std::vector<UpdateInfo> UpdateInfoList;

    
    /** Gets all orphan updates for a given object. */
    UpdateInfoList getOrphanUpdates(const SpaceObjectReference& obj);

    void setFinalCallback(const std::tr1::function<void()>&);
    
    //When we get a prox removal, we take all the data that was stored in the
    //corresponding proxy object and put it into an OrphanedProxData
    struct OrphanedProxData
    {
        OrphanedProxData(const TimedMotionVector3f& tmv, uint64 tmv_seq_no, const TimedMotionQuaternion& tmq, uint64 tmq_seq_no, const Transfer::URI& uri_mesh, uint64 mesh_seq_no, const BoundingSphere3f& bounding_sphere, uint64 bnds_seq_no, const String& phys, uint64 phys_seq_no)
         : timedMotionVector(tmv),
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
        //Either value or opd will be non-null.  Never both.
        LocUpdate* value;
        OrphanedProxData* opd;
        Time expiresAt;
    };

     
private:
    virtual void poll();
    

    typedef std::tr1::unordered_map<SpaceObjectReference, UpdateInfoList, SpaceObjectReference::Hasher> ObjectUpdateMap;

    Context* mContext;
    Duration mTimeout;
    ObjectUpdateMap mUpdates;
    /** This function gets called and cleared each time the poller is called */
    std::tr1::function<void()> *mFinalCallback;
}; // class OrphanLocUpdateManager


} // namespace Sirikata

#endif
