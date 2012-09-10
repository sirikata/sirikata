// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_AGGREGATE_BOUNDING_INFO_HPP_
#define _SIRIKATA_LIBCORE_AGGREGATE_BOUNDING_INFO_HPP_

#include <sirikata/core/util/Vector3.hpp>
#include <sirikata/core/util/BoundingSphere.hpp>

namespace Sirikata {

/** AggregateBoundingInfo represents the bounds information for a collection of
 *  objects in it's own coordinate frame. It is used to represent bounds for
 *  both collections of objects and individual objects: individual objects are
 *  just a degenerate case.
 *
 *  Three values are included in this bounds information: offset, center bounds
 *  radius, and maximum object size. The offset adjusts the center of mass of
 *  the object or collection of objects -- it's not the position of the object,
 *  which is stored separately. The center bounds radius is the radius which,
 *  when combined with the offset position, encompasses the center points of all
 *  individual objects in this aggregate. (Note that this is *not* the radius
 *  which bounds all the objects, it only bounds their center points.) Finally,
 *  the maximum object size is the radius of the largest object in this
 *  aggregate.
 *
 *  Because this covers the needs of both individual objects and aggregates, it
 *  can have some degenerate values. In particular, for a single object the
 *  center bounds radius will be 0 since there is only 1 center point to form
 *  bounds around, and for aggregates the offset should generally be 0 since
 *  there's nothing meaningful that can be accomplished by setting it to some
 *  other value.
 *
 */
class AggregateBoundingInfo {
public:
    Vector3f centerOffset;
    float32 centerBoundsRadius;
    float32 maxObjectRadius;

    /// Default constructor, initializes to an invalid value, where the bounds
    /// information is negative.
    AggregateBoundingInfo()
     : centerOffset(0, 0, 0),
       centerBoundsRadius(-1),
       maxObjectRadius(0)
    {}

    /// Constructor for single object data
    AggregateBoundingInfo(Vector3f off, float32 obj_size)
     : centerOffset(off),
       centerBoundsRadius(0),
       maxObjectRadius(obj_size)
    {}

    /// Constructor for single object data
    explicit AggregateBoundingInfo(const BoundingSphere3f& obj_bounds)
     : centerOffset(obj_bounds.center()),
       centerBoundsRadius(0),
       maxObjectRadius(obj_bounds.radius())
    {}

    /// Constructor for aggregate
    AggregateBoundingInfo(Vector3f off, float32 center_bounds, float32 max_obj_size)
     : centerOffset(off),
       centerBoundsRadius(center_bounds),
       maxObjectRadius(max_obj_size)
    {}

    /// Get the bounding sphere containing the centers of all objects
    BoundingSphere3f centerBounds() const {
        return BoundingSphere3f(centerOffset, centerBoundsRadius);
    }

    /// Get the radius of the full bounding sphere
    float32 fullRadius() const {
        return centerBoundsRadius + maxObjectRadius;
    }

    /// Get the bounding sphere containing all objects
    BoundingSphere3f fullBounds() const {
        return BoundingSphere3f(centerOffset, fullRadius());
    }

    /// Returns true if this is a single object, i.e. if the center bounds
    /// radius is 0.
    bool singleObject() const {
        return (centerBoundsRadius == 0);
    }

    bool invalid() const {
        return (centerBoundsRadius < 0);
    }

    AggregateBoundingInfo& mergeIn(const AggregateBoundingInfo& rhs) {
        *this = merge(rhs);
        return *this;
    }

    AggregateBoundingInfo merge(const AggregateBoundingInfo& rhs) {
        BoundingSphere3f new_cbnds = centerBounds().merge(rhs.centerBounds());
        float32 new_max_rad = std::max(maxObjectRadius, rhs.maxObjectRadius);
        return AggregateBoundingInfo(new_cbnds.center(), new_cbnds.radius(), new_max_rad);
    }

    bool operator==(const AggregateBoundingInfo& rhs) {
        return (centerOffset == rhs.centerOffset && centerBoundsRadius == rhs.centerBoundsRadius && maxObjectRadius == rhs.maxObjectRadius);
    }
    bool operator!=(const AggregateBoundingInfo& rhs) {
        return (centerOffset != rhs.centerOffset || centerBoundsRadius != rhs.centerBoundsRadius || maxObjectRadius != rhs.maxObjectRadius);
    }

private:
};

inline std::ostream& operator <<(std::ostream& os, const AggregateBoundingInfo &rhs) {
    os << "< offset: " << rhs.centerOffset << ", center bounds radius: " << rhs.centerBoundsRadius << ", max object size: " << rhs.maxObjectRadius << ">";
    return os;
}

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_AGGREGATE_BOUNDING_INFO_HPP_
