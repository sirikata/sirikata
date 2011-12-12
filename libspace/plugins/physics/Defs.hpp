// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_BULLET_PHYSICS_DEFS_HPP_
#define _SIRIKATA_BULLET_PHYSICS_DEFS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {

#define BULLETLOG(lvl, msg) SILOG(BulletPhysics, lvl, msg)

class SirikataMotionState;
class BulletObject;
class BulletPhysicsService;

//FIXME Enums for manual treatment of objects and bboxes
//IGNORE = Bullet shouldn't know about this object
//STATIC = Bullet thinks this is a static (not moving) object
//DYNAMIC = Bullet thinks this is a dynamic (moving) object
//LINEAR_DYNAMIC = Turn off rotational effects, but make the object
//dynamic. This means you can push it around and gravity affects it, but it
//should never rotate.
//VERTICAL_DYNAMIC = Turn off all but vertical movement. Useful for
//placing objects on the ground.
enum bulletObjTreatment {
	BULLET_OBJECT_TREATMENT_IGNORE,
	BULLET_OBJECT_TREATMENT_STATIC,
	BULLET_OBJECT_TREATMENT_DYNAMIC,
	BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC,
	BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC
};

enum bulletObjCollisionMaskGroup {
	BULLET_OBJECT_COLLISION_GROUP_STATIC = 1,
	BULLET_OBJECT_COLLISION_GROUP_DYNAMIC = 1 << 1,
	BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED = 1 << 2
};

//ENTIRE_OBJECT = Bullet creates an AABB encompassing the entire object
//PER_TRIANGLE = Bullet creates a series of AABBs for each triangle in the object
//		This option is useful for polygon soups - terrain, for example
//SPHERE = Bullet creates a bounding sphere based on the bounds.radius
enum bulletObjBBox {
	BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT,
	BULLET_OBJECT_BOUNDS_PER_TRIANGLE,
	BULLET_OBJECT_BOUNDS_SPHERE
};


struct LocationInfo {
    LocationInfo()
     : location(),
       orientation(),
       bounds(),
       mesh(),
       physics(),
       local(),
       aggregate(),
       simObject(NULL)
    {}

    // Regular location info that we need to maintain for all objects
    TimedMotionVector3f location;
    TimedMotionQuaternion orientation;
    BoundingSphere3f bounds;
    String mesh;
    String physics;
    bool local;
    bool aggregate;

    BulletObject* simObject;
};

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_DEFS_HPP_
