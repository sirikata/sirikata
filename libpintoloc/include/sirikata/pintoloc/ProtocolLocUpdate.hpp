// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTOLOC_PROTOCOL_LOC_UPDATE_HPP_
#define _SIRIKATA_LIBPINTOLOC_PROTOCOL_LOC_UPDATE_HPP_

#include <sirikata/pintoloc/LocUpdate.hpp>
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"
#include <sirikata/pintoloc/TimeSynced.hpp>

namespace Sirikata {

/** Implementation of LocUpdate which collects its information from a PBJ
 *  LocationUpdate object.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT LocProtocolLocUpdate : public LocUpdate {
public:
    /** Construct a LocProtocolLocUpdate using the given raw LocationUpdate and
     *  the OH and space to adjust times to the local timeframe.
     *
     *  \param lu the raw LocationUpdate
     *  \param sync TimeSync to use for time conversion
     *
     *  \note the references passed in here must remain valid for the lifetime
     *  of this object.
     */
    LocProtocolLocUpdate(const Sirikata::Protocol::Loc::LocationUpdate& lu, const TimeSynced& sync)
     : mUpdate(lu),
        mSync(sync)
    {}
    virtual ~LocProtocolLocUpdate() {}

    virtual ObjectReference object() const { return ObjectReference(mUpdate.object()); }

    // Request epoch
    virtual bool has_epoch() const { return mUpdate.has_epoch(); }
    virtual uint64 epoch() const { return mUpdate.epoch(); }

    // Parent aggregate
    virtual bool has_parent() const { return mUpdate.has_parent(); }
    virtual ObjectReference parent() const { return ObjectReference(mUpdate.parent()); }
    virtual uint64 parent_seqno() const { return seqno(); }

    // Location
    virtual bool has_location() const { return mUpdate.has_location(); }
    virtual TimedMotionVector3f location() const;
    virtual uint64 location_seqno() const { return seqno(); }

    // Orientation
    virtual bool has_orientation() const { return mUpdate.has_orientation(); }
    virtual TimedMotionQuaternion orientation() const;
    virtual uint64 orientation_seqno() const { return seqno(); }

    // Bounds
    virtual bool has_bounds() const { return mUpdate.has_aggregate_bounds(); }
    virtual AggregateBoundingInfo bounds() const {
        Vector3f center = mUpdate.aggregate_bounds().has_center_offset() ? mUpdate.aggregate_bounds().center_offset() : Vector3f(0,0,0);
        float32 center_rad = mUpdate.aggregate_bounds().has_center_bounds_radius() ? mUpdate.aggregate_bounds().center_bounds_radius() : 0.f;
        float32 max_object_size = mUpdate.aggregate_bounds().has_max_object_size() ? mUpdate.aggregate_bounds().max_object_size() : 0.f;
        return AggregateBoundingInfo(center, center_rad, max_object_size);
    }
    virtual uint64 bounds_seqno() const { return seqno(); }

    // Mesh
    virtual bool has_mesh() const { return mUpdate.has_mesh(); }
    virtual String mesh() const { return mUpdate.mesh(); }
    virtual uint64 mesh_seqno() const { return seqno(); }

    // Physics
    virtual bool has_physics() const { return mUpdate.has_physics(); }
    virtual String physics() const { return mUpdate.physics(); }
    virtual uint64 physics_seqno() const { return seqno(); }

    virtual uint32 index_id_size() const { return mUpdate.index_id_size(); }
    virtual ProxIndexID index_id(int32 idx) const { return mUpdate.index_id(idx); }
    virtual uint64 index_id_seqno() const { return seqno(); }

private:
    LocProtocolLocUpdate();
    LocProtocolLocUpdate(const LocProtocolLocUpdate&);

    uint64 seqno() const { return (mUpdate.has_seqno() ? mUpdate.seqno() : 0); }

    const Sirikata::Protocol::Loc::LocationUpdate& mUpdate;
    const TimeSynced& mSync;
};

/** Implementation of LocUpdate which collects its information from a PBJ
 *  ProximityUpdate object.
 *
 *  Note that this assumes times have been converted to local time
 *  already because this is used in places where we wouldn't be able
 *  to tell whether the time had been converted yet.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT ProxProtocolLocUpdate : public LocUpdate {
public:
    /** Construct a ProxProtocolLocUpdate using the given raw ProximityAddition
     *  and the OH and space to adjust times to the local timeframe.
     *
     *  \param lu the raw LocationUpdate
     *
     *  \note the references passed in here must remain valid for the lifetime
     *  of this object.
     */
    ProxProtocolLocUpdate(const Sirikata::Protocol::Prox::ObjectAddition& lu)
     : mUpdate(lu)
    {}
    virtual ~ProxProtocolLocUpdate() {}

    virtual ObjectReference object() const { return ObjectReference(mUpdate.object()); }

    // Request epoch
    virtual bool has_epoch() const { return true; }
    virtual uint64 epoch() const { return 0; }

    // Parent aggregate
    virtual bool has_parent() const { return mUpdate.has_parent(); }
    virtual ObjectReference parent() const { return ObjectReference(mUpdate.parent()); }
    virtual uint64 parent_seqno() const { return seqno(); }

    // Location
    virtual bool has_location() const { return true; }
    TimedMotionVector3f location() const {
        Sirikata::Protocol::TimedMotionVector update_loc = mUpdate.location();
        return TimedMotionVector3f(
            update_loc.t(),
            MotionVector3f(update_loc.position(), update_loc.velocity())
        );
    }
    virtual uint64 location_seqno() const { return seqno(); }

    // Orientation
    virtual bool has_orientation() const { return true; }
    TimedMotionQuaternion orientation() const {
        Sirikata::Protocol::TimedMotionQuaternion update_orient = mUpdate.orientation();
        return TimedMotionQuaternion(
            update_orient.t(),
            MotionQuaternion(update_orient.position(), update_orient.velocity())
        );
    }
    virtual uint64 orientation_seqno() const { return seqno(); }

    // Bounds
    virtual bool has_bounds() const { return mUpdate.has_aggregate_bounds(); }
    virtual AggregateBoundingInfo bounds() const {
        Vector3f center = mUpdate.aggregate_bounds().has_center_offset() ? mUpdate.aggregate_bounds().center_offset() : Vector3f(0,0,0);
        float32 center_rad = mUpdate.aggregate_bounds().has_center_bounds_radius() ? mUpdate.aggregate_bounds().center_bounds_radius() : 0.f;
        float32 max_object_size = mUpdate.aggregate_bounds().has_max_object_size() ? mUpdate.aggregate_bounds().max_object_size() : 0.f;
        return AggregateBoundingInfo(center, center_rad, max_object_size);
    }
    virtual uint64 bounds_seqno() const { return seqno(); }

    // Mesh
    virtual bool has_mesh() const { return mUpdate.has_mesh(); }
    virtual String mesh() const { return mUpdate.mesh(); }
    virtual uint64 mesh_seqno() const { return seqno(); }

    // Physics
    virtual bool has_physics() const { return mUpdate.has_physics(); }
    virtual String physics() const { return mUpdate.physics(); }
    virtual uint64 physics_seqno() const { return seqno(); }

    virtual uint32 index_id_size() const { return 0; }
    virtual ProxIndexID index_id(int32 idx) const { assert(false && "Out of bounds index id"); return -1; }
    virtual uint64 index_id_seqno() const { return seqno(); }
private:
    ProxProtocolLocUpdate();
    ProxProtocolLocUpdate(const ProxProtocolLocUpdate&);

    uint64 seqno() const { return (mUpdate.has_seqno() ? mUpdate.seqno() : 0); }

    const Sirikata::Protocol::Prox::ObjectAddition& mUpdate;
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBPINTOLOC_PROTOCOL_LOC_UPDATE_HPP_
