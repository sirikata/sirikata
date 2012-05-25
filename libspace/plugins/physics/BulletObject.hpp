// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_
#define _SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_

#include "Defs.hpp"

class btCollisionShape;

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

    /** Make modifications before a full tick.
     */
    virtual void preTick(const Time& t) {}

    /** Make modifications before a full tick.
     */
    virtual void postTick(const Time& t) {}

    /** Make modifications during an internal tick. Useful for capping speed and
     *  rotational speed.
     */
    virtual void internalTick(const Time& t) {}

    /** Check this object for deactivation. Implementations should call
     *  BulletPhysicsService::updateObjectFromDeactivation if the object has
     *  been deactivated.
     */
    virtual void deactivationTick(const Time& t) {}



    /** Try to apply the requested position to this object, updating
     *  the physics simulation if necessary. Some types of objects
     *  may always fail to apply updates. Returns true if the update
     *  was applied and an update will need to be sent.
     */
    virtual bool applyRequestedLocation(const TimedMotionVector3f& loc, uint64 epoch) = 0;
    /** Try to apply the requested orientation to this object, updating
     *  the physics simulation if necessary. Some types of objects
     *  may always fail to apply updates. Returns true if the update
     *  was applied and an update will need to be sent.
     */
    virtual bool applyRequestedOrientation(const TimedMotionQuaternion& orient, uint64 epoch) = 0;

    /** Apply the given position to this object. This takes whatever
     *  steps are necessary to apply the change, even completely
     *  reloading the object in Bullet. This should only be used when
     *  absolutely necessary, e.g. for updates about remote,
     *  physically simulated objects.
     */
    virtual void applyForcedLocation(const TimedMotionVector3f& loc, uint64 epoch) = 0;
    /** Apply the given orientation to this object. This takes whatever
     *  steps are necessary to apply the change, even completely
     *  reloading the object in Bullet. This should only be used when
     *  absolutely necessary, e.g. for updates about remote,
     *  physically simulated objects.
     */
    virtual void applyForcedOrientation(const TimedMotionQuaternion& orient, uint64 epoch) = 0;

protected:

    // Helper for computing the collision
    btCollisionShape* computeCollisionShape(const UUID& id, bulletObjBBox shape_type, bulletObjTreatment treatment, Mesh::MeshdataPtr retrievedMesh);

    BulletPhysicsService* mParent;
}; // class BulletObject

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_
