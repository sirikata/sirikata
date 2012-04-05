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
#include <sirikata/oh/LocUpdate.hpp>
#include <sirikata/oh/ProtocolLocUpdate.hpp>
#include <sirikata/core/util/PresenceProperties.hpp>

namespace Sirikata {

namespace Protocol {
namespace Loc {
class LocationUpdate;
}
}
class PresencePropertiesLocUpdate;

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
    template<typename QuerierIDType>
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void onOrphanLocUpdate(const QuerierIDType& observer, const LocUpdate& lu) = 0;
    };

    OrphanLocUpdateManager(Context* ctx, Network::IOStrand* strand, const Duration& timeout);

    /** Add an orphan update to the queue and set a timeout for it to be cleared
     *  out.
     */
    void addOrphanUpdate(const SpaceObjectReference& observed, const Sirikata::Protocol::Loc::LocationUpdate& update);
    /**
       Take all fields in proxyPtr, and create an struct from
       them.
     */
    void addUpdateFromExisting(ProxyObjectPtr proxyPtr);
    /** Take all parameters from an object to backup for a short time.
     */
    void addUpdateFromExisting(
        const SpaceObjectReference& observed,
        const SequencedPresenceProperties& props
    );

    /** Gets all orphan updates for a given object. */
    template<typename QuerierIDType>
    void invokeOrphanUpdates(const QuerierIDType& observer, const SpaceObjectReference& proximateID, Listener<QuerierIDType>* listener) {
        ObjectUpdateMap::iterator it = mUpdates.find(proximateID);
        if (it == mUpdates.end()) return;

        const UpdateInfoList& info_list = it->second;
        for(UpdateInfoList::const_iterator info_it = info_list.begin(); info_it != info_list.end(); info_it++) {
            if ((*info_it)->value != NULL) {
                LocProtocolLocUpdate llu( *((*info_it)->value) );
                listener->onOrphanLocUpdate( observer, llu );
            }
            else if ((*info_it)->opd != NULL) {
                PresencePropertiesLocUpdate plu( (*info_it)->object.object(), *((*info_it)->opd) );
                listener->onOrphanLocUpdate( observer, plu );
            }
        }

        // Once we've notified of these we can get rid of them -- if they
        // need the info again they should re-register it with
        // addUpdateFromExisting before cleaning up the object.
        mUpdates.erase(it);
    }

private:
    virtual void poll();

    struct UpdateInfo {
        UpdateInfo(const SpaceObjectReference& obj, Sirikata::Protocol::Loc::LocationUpdate* _v, const Time& t)
         : object(obj), value(_v), opd(NULL), expiresAt(t)
        {}
        UpdateInfo(const SpaceObjectReference& obj, SequencedPresenceProperties* _v, const Time& t)
         : object(obj), value(NULL), opd(_v), expiresAt(t)
        {}
        ~UpdateInfo();

        SpaceObjectReference object;
        //Either value or opd will be non-null.  Never both.
        Sirikata::Protocol::Loc::LocationUpdate* value;
        SequencedPresenceProperties* opd;

        Time expiresAt;
    private:
        UpdateInfo();
    };
    typedef std::tr1::shared_ptr<UpdateInfo> UpdateInfoPtr;
    typedef std::vector<UpdateInfoPtr> UpdateInfoList;

    typedef std::tr1::unordered_map<SpaceObjectReference, UpdateInfoList, SpaceObjectReference::Hasher> ObjectUpdateMap;

    Context* mContext;
    Duration mTimeout;
    ObjectUpdateMap mUpdates;
}; // class OrphanLocUpdateManager



/** Implementation of LocUpdate which collects its information from
 *  stored SequencedPresenceProperties.
 */
class PresencePropertiesLocUpdate : public LocUpdate {
public:
    PresencePropertiesLocUpdate(const ObjectReference& o, const SequencedPresenceProperties& lu)
     : mObject(o),
       mUpdate(lu)
    {}
    virtual ~PresencePropertiesLocUpdate() {}

    virtual ObjectReference object() const { return mObject; }

    // Request epoch
    // Since these are only used for orphan updates, we should never have an
    // epoch we would care about -- presences should never have orphans since
    // their proxies are created as soon as they get connected.
    virtual bool has_epoch() const { return false; }
    virtual uint64 epoch() const { return 0; }

    // Parent aggregate
    virtual bool has_parent() const { return (parent() != ObjectReference::null()); }
    virtual ObjectReference parent() const { return ObjectReference(mUpdate.parent()); }
    virtual uint64 parent_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_PARENT_PART); }

    // Location
    virtual bool has_location() const { return true; }
    virtual TimedMotionVector3f location() const { return mUpdate.location(); }
    virtual uint64 location_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_POS_PART); }

    // Overridden because PresenceProperties work with local time
    virtual TimedMotionVector3f locationWithLocalTime(ObjectHost* oh, const SpaceID& from_space) const { return location(); }

    // Orientation
    virtual bool has_orientation() const { return true; }
    virtual TimedMotionQuaternion orientation() const { return mUpdate.orientation(); }
    virtual uint64 orientation_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_ORIENT_PART); }

    // Overridden because PresenceProperties work with local time
    virtual TimedMotionQuaternion orientationWithLocalTime(ObjectHost* oh, const SpaceID& from_space) const { return orientation(); }

    // Bounds
    virtual bool has_bounds() const { return true; }
    virtual BoundingSphere3f bounds() const { return mUpdate.bounds(); }
    virtual uint64 bounds_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_BOUNDS_PART); }

    // Mesh
    virtual bool has_mesh() const { return true; }
    virtual String mesh() const { return mUpdate.mesh().toString(); }
    virtual uint64 mesh_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_MESH_PART); }

    // Physics
    virtual bool has_physics() const { return true; }
    virtual String physics() const { return mUpdate.physics(); }
    virtual uint64 physics_seqno() const { return mUpdate.getUpdateSeqNo(SequencedPresenceProperties::LOC_PHYSICS_PART); }
private:
    PresencePropertiesLocUpdate();
    PresencePropertiesLocUpdate(const PresencePropertiesLocUpdate&);

    const ObjectReference& mObject;
    const SequencedPresenceProperties& mUpdate;
};


} // namespace Sirikata

#endif
