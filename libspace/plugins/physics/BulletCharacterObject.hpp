// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_BULLET_PHYSICS_CHARACTER_OBJECT_HPP_
#define _SIRIKATA_BULLET_PHYSICS_CHARACTER_OBJECT_HPP_

#include "BulletObject.hpp"

class btPairCachingGhostObject;

namespace Sirikata {

class BulletCharacterController;

/** Implementation of BulletObject for characters. Characters behave
 *  differently than other physical objects -- they try to stay on the
 *  ground and fall like normal objects, but we don't want them to
 *  rotate, want them to slide against the ground and walls, etc.
 */
class BulletCharacterObject : public BulletObject {
public:
    BulletCharacterObject(BulletPhysicsService* parent, const UUID& uuid, bulletObjBBox bb = BULLET_OBJECT_BOUNDS_SPHERE);
    virtual ~BulletCharacterObject();

    virtual bool fixed();
    virtual bulletObjTreatment treatment();
    virtual bulletObjBBox bbox();
    virtual float32 mass();

    virtual void load(Mesh::MeshdataPtr mesh);
    virtual void unload();
    virtual void preTick(const Time& t);
    virtual void postTick(const Time& t);
    virtual void deactivationTick(const Time& t);


    virtual bool applyRequestedLocation(const TimedMotionVector3f& loc);
    virtual bool applyRequestedOrientation(const TimedMotionQuaternion& orient);
    virtual void applyForcedLocation(const TimedMotionVector3f& loc);
    virtual void applyForcedOrientation(const TimedMotionQuaternion& orient);

private:
    UUID mID;

    bulletObjBBox mBBox;

    btPairCachingGhostObject* mGhostObject;
    BulletCharacterController* mCharacter;
    btCollisionShape* mCollisionShape;
}; // class BulletCharacterObject

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_CHARACTER_OBJECT_HPP_
