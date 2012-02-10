// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_PROTOCOL_LOC_UPDATE_HPP_
#define _SIRIKATA_OH_PROTOCOL_LOC_UPDATE_HPP_

#include <sirikata/oh/LocUpdate.hpp>
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"

namespace Sirikata {

/** Implementation of LocUpdate which collects its information from a PBJ
 *  LocationUpdate object.
 */
class SIRIKATA_OH_EXPORT LocProtocolLocUpdate : public LocUpdate {
public:
    /** Construct a LocProtocolLocUpdate using the given raw LocationUpdate and
     *  the OH and space to adjust times to the local timeframe.
     *
     *  \param lu the raw LocationUpdate
     *  \param oh the OH this update originated from
     *  \param space the space the update originated from
     *
     *  \note the references passed in here must remain valid for the lifetime
     *  of this object.
     */
    LocProtocolLocUpdate(const Sirikata::Protocol::Loc::LocationUpdate& lu, const ObjectHost* oh, const SpaceID& space)
     : mUpdate(lu),
       mOH(oh),
       mSpace(space)
    {}
    virtual ~LocProtocolLocUpdate() {}

    virtual ObjectReference object() const { return ObjectReference(mUpdate.object()); }

    // Request epoch
    virtual bool has_epoch() const { return mUpdate.has_epoch(); }
    virtual uint64 epoch() const { return mUpdate.epoch(); }

    // Location
    virtual bool has_location() const { return mUpdate.has_location(); }
    virtual TimedMotionVector3f location() const;
    virtual uint64 location_seqno() const { return seqno(); }

    // Orientation
    virtual bool has_orientation() const { return mUpdate.has_orientation(); }
    virtual TimedMotionQuaternion orientation() const;
    virtual uint64 orientation_seqno() const { return seqno(); }

    // Bounds
    virtual bool has_bounds() const { return mUpdate.has_bounds(); }
    virtual BoundingSphere3f bounds() const { return mUpdate.bounds(); }
    virtual uint64 bounds_seqno() const { return seqno(); }

    // Mesh
    virtual bool has_mesh() const { return mUpdate.has_mesh(); }
    virtual String mesh() const { return mUpdate.mesh(); }
    virtual uint64 mesh_seqno() const { return seqno(); }

    // Physics
    virtual bool has_physics() const { return mUpdate.has_physics(); }
    virtual String physics() const { return mUpdate.physics(); }
    virtual uint64 physics_seqno() const { return seqno(); }
private:
    LocProtocolLocUpdate();
    LocProtocolLocUpdate(const LocProtocolLocUpdate&);

    uint64 seqno() const { return (mUpdate.has_seqno() ? mUpdate.seqno() : 0); }

    const Sirikata::Protocol::Loc::LocationUpdate& mUpdate;
    const ObjectHost* mOH;
    const SpaceID& mSpace;
};

/** Implementation of LocUpdate which collects its information from a PBJ
 *  ProximityUpdate object.
 */
class SIRIKATA_OH_EXPORT ProxProtocolLocUpdate : public LocUpdate {
public:
    /** Construct a ProxProtocolLocUpdate using the given raw ProximityAddition
     *  and the OH and space to adjust times to the local timeframe.
     *
     *  \param lu the raw LocationUpdate
     *  \param oh the OH this update originated from
     *  \param space the space the update originated from
     *
     *  \note the references passed in here must remain valid for the lifetime
     *  of this object.
     */
    ProxProtocolLocUpdate(const Sirikata::Protocol::Prox::ObjectAddition& lu, const ObjectHost* oh, const SpaceID& space)
     : mUpdate(lu),
       mOH(oh),
       mSpace(space)
    {}
    virtual ~ProxProtocolLocUpdate() {}

    virtual ObjectReference object() const { return ObjectReference(mUpdate.object()); }

    // Request epoch
    virtual bool has_epoch() const { return true; }
    virtual uint64 epoch() const { return 0; }

    // Location
    virtual bool has_location() const { return true; }
    virtual TimedMotionVector3f location() const;
    virtual uint64 location_seqno() const { return seqno(); }

    // Orientation
    virtual bool has_orientation() const { return true; }
    virtual TimedMotionQuaternion orientation() const;
    virtual uint64 orientation_seqno() const { return seqno(); }

    // Bounds
    virtual bool has_bounds() const { return true; }
    virtual BoundingSphere3f bounds() const { return mUpdate.bounds(); }
    virtual uint64 bounds_seqno() const { return seqno(); }

    // Mesh
    virtual bool has_mesh() const { return mUpdate.has_mesh(); }
    virtual String mesh() const { return mUpdate.mesh(); }
    virtual uint64 mesh_seqno() const { return seqno(); }

    // Physics
    virtual bool has_physics() const { return mUpdate.has_physics(); }
    virtual String physics() const { return mUpdate.physics(); }
    virtual uint64 physics_seqno() const { return seqno(); }
private:
    ProxProtocolLocUpdate();
    ProxProtocolLocUpdate(const ProxProtocolLocUpdate&);

    uint64 seqno() const { return (mUpdate.has_seqno() ? mUpdate.seqno() : 0); }

    const Sirikata::Protocol::Prox::ObjectAddition& mUpdate;
    const ObjectHost* mOH;
    const SpaceID& mSpace;
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_PROTOCOL_LOC_UPDATE_HPP_
