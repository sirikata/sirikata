// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "BulletCharacterObject.hpp"
#include "BulletCharacterController.hpp"
#include "BulletPhysicsService.hpp"

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace Sirikata {

BulletCharacterObject::BulletCharacterObject(BulletPhysicsService* parent, const UUID& uuid, bulletObjBBox bb)
 : BulletObject(parent),
   mID(uuid),
   mBBox(bb),
   mGhostObject(NULL),
   mCharacter(NULL),
   mCollisionShape(NULL)
{
    if (mBBox != BULLET_OBJECT_BOUNDS_SPHERE &&
        mBBox != BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT)
    {
        BULLETLOG(error, "Tried to set character collision bounds to unsupported type. Only box and sphere are supported for characters. Defaulting to box.");
        mBBox = BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT;
    }
}

BulletCharacterObject::~BulletCharacterObject() {
}

bool BulletCharacterObject::fixed() {
    return false;
}

bulletObjTreatment BulletCharacterObject::treatment() {
    return BULLET_OBJECT_TREATMENT_CHARACTER;
}

bulletObjBBox BulletCharacterObject::bbox() {
    return mBBox;
}

float32 BulletCharacterObject::mass() {
    return 0.f;
}

void BulletCharacterObject::load(Mesh::MeshdataPtr mesh) {
    LocationInfo& locinfo = mParent->info(mID);

    Vector3f objPosition = mParent->currentPosition(mID);
    Quaternion objOrient = mParent->currentOrientation(mID);
    btTransform startTransform = btTransform(
        btQuaternion(objOrient.x,objOrient.y,objOrient.z,objOrient.w),
        btVector3(objPosition.x,objPosition.y,objPosition.z)
    );

    mGhostObject = new btPairCachingGhostObject();
    mGhostObject->setWorldTransform(startTransform);
    mParent->broadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

    // Currently only support spheres, TODO(ewencp) we might want to support
    // capsules instead.
    mCollisionShape = computeCollisionShape(mID, mBBox, Mesh::MeshdataPtr());
    mGhostObject->setCollisionShape(mCollisionShape);
    mGhostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);

    // TODO(ewencp) make this configurable?
    btScalar stepHeight = btScalar(0.5);
    // FIXME(ewencp) we know we're only allocating spheres/capsules for this, so
    // it's currently safe to do this cast, but technically
    // computeCollisionShape can compute concave shapes as well.
    mCharacter = new BulletCharacterController (mGhostObject, static_cast<btConvexShape*>(mCollisionShape), stepHeight);


    // Get mask information
    bulletObjCollisionMaskGroup mygroup, collide_with;
    BulletObject::getCollisionMask(BULLET_OBJECT_TREATMENT_CHARACTER, &mygroup, &collide_with);
    mParent->dynamicsWorld()->addCollisionObject(mGhostObject, (short)mygroup, (short)collide_with);
    mParent->dynamicsWorld()->addAction(mCharacter);

    mParent->addTickObject(mID);
    mParent->addDeactivateableObject(mID);
}

void BulletCharacterObject::unload() {
    if (mCharacter) {
        mParent->removeTickObject(mID);
        mParent->removeDeactivateableObject(mID);

        mParent->dynamicsWorld()->removeAction(mCharacter);
        mParent->dynamicsWorld()->removeCollisionObject(mGhostObject);

        delete mCharacter;
        mCharacter = NULL;

        delete mGhostObject;
        mGhostObject = NULL;

        delete mCollisionShape;
        mCollisionShape = NULL;
    }
}

void BulletCharacterObject::preTick(const Time& t) {
    LocationInfo& locinfo = mParent->info(mID);
    Vector3f char_vel = locinfo.location.velocity();
    btVector3 bt_char_vel(char_vel.x, char_vel.y, char_vel.z);
    mCharacter->setWalkDirection(bt_char_vel);
    // Character controller doesn't have any rotation support. Instead, we set
    // the rotation on the ghost object directly based on the requested
    // rotation. FIXME(ewencp) this needs some sort of handling similar to
    // walking direction where we know to stop the rotation at some point.
    btTransform xform = mGhostObject->getWorldTransform();
    btVector3 objPosition = xform.getOrigin();
    Quaternion objOrient = mParent->currentOrientation(mID);
    mGhostObject->setWorldTransform(
        btTransform(
            btQuaternion(objOrient.x, objOrient.y, objOrient.z, objOrient.w),
            objPosition
        )
    );
}

void BulletCharacterObject::postTick(const Time& t) {
    LocationInfo& locinfo = mParent->info(mID);

    btTransform worldTrans = mGhostObject->getWorldTransform();
    btVector3 pos = worldTrans.getOrigin();
    TimedMotionVector3f newLocation(t, MotionVector3f(Vector3f(pos.x(), pos.y(), pos.z()), locinfo.location.velocity()));
    mParent->setLocation(mID, newLocation);
    btQuaternion rot = worldTrans.getRotation();
    TimedMotionQuaternion newOrientation(
        t,
        MotionQuaternion(
            Quaternion(rot.x(), rot.y(), rot.z(), rot.w()),
            locinfo.orientation.velocity()
        )
    );
    mParent->setOrientation(mID, newOrientation);

    mParent->addUpdate(mID);
}

void BulletCharacterObject::deactivationTick(const Time& t) {
    if (mGhostObject != NULL && !mGhostObject->isActive())
        mParent->updateObjectFromDeactivation(mID);
}

} // namespace Sirikata
