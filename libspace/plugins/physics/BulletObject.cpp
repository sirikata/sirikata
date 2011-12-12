// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "BulletObject.hpp"

namespace Sirikata {

// Kinda sucks that we need this to be outside the implementations of
// BulletObject, but we essentially need to specify a matrix of all possible
// interactions
void BulletObject::getCollisionMask(bulletObjTreatment treatment, bulletObjCollisionMaskGroup* mygroup, bulletObjCollisionMaskGroup* collide_with) {
    switch(treatment) {
      case BULLET_OBJECT_TREATMENT_IGNORE:
        // We shouldn't be trying to add this to a sim
        assert(treatment != BULLET_OBJECT_TREATMENT_IGNORE);
        break;
      case BULLET_OBJECT_TREATMENT_STATIC:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_STATIC;
        // Static collides with everything
        *collide_with = (bulletObjCollisionMaskGroup)(BULLET_OBJECT_COLLISION_GROUP_STATIC | BULLET_OBJECT_COLLISION_GROUP_DYNAMIC | BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED);
        break;
      case BULLET_OBJECT_TREATMENT_DYNAMIC:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_DYNAMIC;
        *collide_with = (bulletObjCollisionMaskGroup)(BULLET_OBJECT_COLLISION_GROUP_STATIC | BULLET_OBJECT_COLLISION_GROUP_DYNAMIC | BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED);
        break;
      case BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC:
      case BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED;
        // Only collide with static objects because the constrained
        // movement can become problematic otherwise, e.g. the
        // vertical only movement can result in interpenetrating
        // objects which can't resolve their collision normally and
        // the energy ends up in the vertically moving object,
        // throwing it up in the air
        *collide_with = BULLET_OBJECT_COLLISION_GROUP_STATIC;
        break;

    };
}

} // namespace Sirikata
