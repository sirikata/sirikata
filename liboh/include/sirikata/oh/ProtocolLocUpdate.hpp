// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_PROTOCOL_LOC_UPDATE_HPP_
#define _SIRIKATA_OH_PROTOCOL_LOC_UPDATE_HPP_

#include <sirikata/oh/LocUpdate.hpp>
#include "Protocol_Loc.pbj.hpp"

namespace Sirikata {

/** Implementation of LocUpdate which collects its information from a PBJ
 *  LocationUpdate object.
 */
class ProtocolLocUpdate : public LocUpdate {
public:
    ProtocolLocUpdate(const Sirikata::Protocol::Loc::LocationUpdate& lu)
     : mUpdate(lu)
    {}
    virtual ~ProtocolLocUpdate() {}

    virtual ObjectReference object() const { return ObjectReference(mUpdate.object()); }

    // Location
    virtual bool has_location() const { return mUpdate.has_location(); }
    virtual TimedMotionVector3f location() const {
        Sirikata::Protocol::TimedMotionVector update_loc = mUpdate.location();
        return TimedMotionVector3f(update_loc.t(), MotionVector3f(update_loc.position(), update_loc.velocity()));
    }
    virtual uint64 location_seqno() const { return seqno(); }

    // Orientation
    virtual bool has_orientation() const { return mUpdate.has_orientation(); }
    virtual TimedMotionQuaternion orientation() const {
        Sirikata::Protocol::TimedMotionQuaternion update_orient = mUpdate.orientation();
        return TimedMotionQuaternion(update_orient.t(), MotionQuaternion(update_orient.position(), update_orient.velocity()));
    }
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
    ProtocolLocUpdate();

    uint64 seqno() const { return (mUpdate.has_seqno() ? mUpdate.seqno() : 0); }

    const Sirikata::Protocol::Loc::LocationUpdate& mUpdate;
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_PROTOCOL_LOC_UPDATE_HPP_
