// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_LOC_UPDATE_HPP_
#define _SIRIKATA_OH_LOC_UPDATE_HPP_

#include <sirikata/core/util/Noncopyable.hpp>

namespace Sirikata {

/** LocUpdate is an abstract representation of location updates. This is
 *  provided since some location updates may be used directly from the wire
 *  protocol whereas others may be delayed, 'reconstituted', or just created on
 *  the fly in-memory (e.g. due to local queries). This class acts as an adaptor
 *  to get at all the properties from the underlying representation.
 */
class LocUpdate : Noncopyable {
public:
    virtual ~LocUpdate() {}

    // Object
    virtual ObjectReference object() const = 0;

    // Location
    virtual bool has_location() const = 0;
    virtual TimedMotionVector3f location() const = 0;
    virtual uint64 location_seqno() const = 0;

    // Orientation
    virtual bool has_orientation() const = 0;
    virtual TimedMotionQuaternion orientation() const = 0;
    virtual uint64 orientation_seqno() const = 0;

    // Bounds
    virtual bool has_bounds() const = 0;
    virtual BoundingSphere3f bounds() const = 0;
    virtual uint64 bounds_seqno() const = 0;

    // Mesh
    virtual bool has_mesh() const = 0;
    virtual String mesh() const = 0;
    virtual uint64 mesh_seqno() const = 0;

    // Physics
    virtual bool has_physics() const = 0;
    virtual String physics() const = 0;
    virtual uint64 physics_seqno() const = 0;
};

} // namespace Sirikata

#endif //_SIRIKATA_OH_LOC_UPDATE_HPP_
