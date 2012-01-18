// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_BULLET_PHYSICS_RIGID_BODY_OBJECT_HPP_
#define _SIRIKATA_BULLET_PHYSICS_RIGID_BODY_OBJECT_HPP_

#include "BulletObject.hpp"
#include "btBulletDynamicsCommon.h"

namespace Sirikata {

class SirikataMotionState;

/** Implementation of BulletObject covering most types of rigid bodies --
 *  static, dynamic, and linearly/vertically constrained.
 */
class BulletRigidBodyObject : public BulletObject {
public:
    BulletRigidBodyObject(BulletPhysicsService* parent, const UUID& uuid, bulletObjTreatment treatment = BULLET_OBJECT_TREATMENT_IGNORE, bulletObjBBox bb = BULLET_OBJECT_BOUNDS_SPHERE, float32 mass = 1.f);
    virtual ~BulletRigidBodyObject();

    virtual bool fixed() { return mFixed; }
    virtual bulletObjTreatment treatment() { return mTreatment; }
    virtual bulletObjBBox bbox() { return mBBox; }
    virtual float32 mass() { return mMass; }

    virtual void load(Mesh::MeshdataPtr mesh);
    virtual void unload();
    virtual void internalTick(const Time& t);
    virtual void deactivationTick(const Time& t);


    virtual bool applyRequestedLocation(const TimedMotionVector3f& loc);
    virtual bool applyRequestedOrientation(const TimedMotionQuaternion& orient);
    virtual void applyForcedLocation(const TimedMotionVector3f& loc);
    virtual void applyForcedOrientation(const TimedMotionQuaternion& orient);


    // Updates from SirikataMotionState
    void updateBulletFromObject(btTransform& worldTrans);
    void updateObjectFromBullet(const btTransform& worldTrans);

private:
    void addRigidBody();
    void removeRigidBody();

    UUID mID;
    // Bullet specific data. First some basic properties:
    bool mFixed;
    bulletObjTreatment mTreatment;
    bulletObjBBox mBBox;
    float32 mMass;
    // And then some implementation data:
    btCollisionShape* mObjShape;
    SirikataMotionState* mObjMotionState;
    btRigidBody* mObjRigidBody;

}; // class BulletRigidBodyObject

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_OBJECT_HPP_
