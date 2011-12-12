// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_
#define _SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_

#include "Defs.hpp"

namespace Sirikata {

/** Base class for simulated objects in bullet. This provides a pretty generic
 *  interface since different types of objects are handled very differently,
 *  e.g. regular rigid bodies vs. characters.
 */
class BulletObject {
public:
    // Gets the collision mask for a given type of object
    static void getCollisionMask(bulletObjTreatment treatment, bulletObjCollisionMaskGroup* mygroup, bulletObjCollisionMaskGroup* collide_with);

    BulletObject(BulletPhysicsService* parent)
     : mParent(parent)
    {}
    virtual ~BulletObject() {}

    virtual bool fixed() = 0;
    virtual bulletObjTreatment treatment() = 0;
    virtual bulletObjBBox bbox() = 0;
    virtual float32 mass() = 0;

    /** After the mesh has been downloaded and parsed (or immediately if no mesh
     *  is required), this loads the object into the simulation. This should
     *  setup any Bullet state and start the physical simulation on the object.
     */
    virtual void load(Mesh::MeshdataPtr mesh) = 0;

    /** Unload the object from the simulation.
     */
    virtual void unload() = 0;

    /** Make modifications during an internal tick. Useful for capping speed and
     *  rotational speed.
     */
    virtual void internalTick() = 0;

    /** Check this object for deactivation. Implementations should call
     *  BulletPhysicsService::updateObjectFromDeactivation if the object has
     *  been deactivated.
     */
    virtual void deactivationTick() = 0;

protected:
    BulletPhysicsService* mParent;
}; // class BulletObject

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_
