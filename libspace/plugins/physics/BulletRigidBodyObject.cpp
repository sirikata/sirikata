// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "BulletRigidBodyObject.hpp"
#include "BulletPhysicsService.hpp"

namespace Sirikata {

class SirikataMotionState : public btMotionState {
public:
    SirikataMotionState(BulletRigidBodyObject* parent)
     : mParent(parent)
    {
    }

    virtual ~SirikataMotionState() {
    }

    virtual void getWorldTransform(btTransform &worldTrans) const {
        mParent->updateBulletFromObject(worldTrans);
    }

    virtual void setWorldTransform(const btTransform &worldTrans) {
        mParent->updateObjectFromBullet(worldTrans);
    }

protected:
    BulletRigidBodyObject* mParent;
}; // class SirikataMotionState


BulletRigidBodyObject::BulletRigidBodyObject(BulletPhysicsService* parent, const UUID& id, bulletObjTreatment treatment, bulletObjBBox bb, float32 mass)
 : BulletObject(parent),
   mID(id),
   mFixed(true),
   mTreatment(treatment),
   mBBox(bb),
   mMass(mass),
   mObjShape(NULL),
   mObjMotionState(NULL),
   mObjRigidBody(NULL)
{
    switch(treatment) {
      case BULLET_OBJECT_TREATMENT_STATIC:
        BULLETLOG(detailed, "This mesh will not move: " << id);
        //this is a variable in loc structure that sets the item to be static
        mFixed = true;
        //if the mass is 0, Bullet treats the object as static
        mMass = 0;
        break;
      case BULLET_OBJECT_TREATMENT_DYNAMIC:
        BULLETLOG(detailed, "This mesh will move: " << id);
        mFixed = false;
        break;
      case BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC:
        BULLETLOG(detailed, "This mesh will move linearly: " << id);
        mFixed = false;
        break;
      case BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC:
        BULLETLOG(detailed, "This mesh will move vertically: " << id);
        mFixed = false;
        break;

      case BULLET_OBJECT_TREATMENT_IGNORE:
      case BULLET_OBJECT_TREATMENT_CHARACTER:
        assert(false && "You shouldn't get into BulletRigidBodyObject for this treatment type.");
        break;
    }
}

BulletRigidBodyObject::~BulletRigidBodyObject() {
    removeRigidBody();
}

void BulletRigidBodyObject::load(MeshdataPtr retrievedMesh) {
    mObjShape = computeCollisionShape(mID, mBBox, retrievedMesh);
    assert(mObjShape != NULL);
    addRigidBody();
}

void BulletRigidBodyObject::addRigidBody() {
    LocationInfo& locinfo = mParent->info(mID);

    //register the motion state (callbacks) for Bullet
    mObjMotionState = new SirikataMotionState(this);

    //set a placeholder for the inertial vector
    btVector3 objInertia(0,0,0);
    //calculate the inertia
    mObjShape->calculateLocalInertia(mMass, objInertia);
    //make a constructionInfo object
    btRigidBody::btRigidBodyConstructionInfo objRigidBodyCI(mMass, mObjMotionState, mObjShape, objInertia);

    //CREATE: make the rigid body
    mObjRigidBody = new btRigidBody(objRigidBodyCI);
    //mObjRigidBody->setRestitution(0.5);
    //set initial velocity
    Vector3f objVelocity = locinfo.props.location().velocity();
    mObjRigidBody->setLinearVelocity(btVector3(objVelocity.x, objVelocity.y, objVelocity.z));
    Quaternion objAngVelocity = locinfo.props.orientation().velocity();
    Vector3f angvel_axis;
    float32 angvel_angle;
    objAngVelocity.toAngleAxis(angvel_angle, angvel_axis);
    Vector3f angvel = angvel_axis.normal() * angvel_angle;
    mObjRigidBody->setAngularVelocity(btVector3(angvel.x, angvel.y, angvel.z));
    // With different types of dynamic objects we need to set . Eventually, we
    // might just want to store values for this in locinfo, currently we just
    // decide based on the treatment.  Everything is linear: <1, 1, 1>, angular
    // <1, 1, 1> by default.
    if (mTreatment == BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC) {
        mObjRigidBody->setAngularFactor(btVector3(0, 0, 0));
    }
    else if (mTreatment == BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC) {
        mObjRigidBody->setAngularFactor(btVector3(0, 0, 0));
        mObjRigidBody->setLinearFactor(btVector3(0, 1, 0));
    }

    // Get mask information
    bulletObjCollisionMaskGroup mygroup, collide_with;
    BulletObject::getCollisionMask(mTreatment, &mygroup, &collide_with);
    //add to the dynamics world
    mParent->dynamicsWorld()->addRigidBody(mObjRigidBody, (short)mygroup, (short)collide_with);
    // And if its dynamic, make sure its in our list of objects to
    // track for sanity checking
    mParent->addInternalTickObject(mID);
    mParent->addDeactivateableObject(mID);
}

void BulletRigidBodyObject::unload() {
    removeRigidBody();
}

void BulletRigidBodyObject::removeRigidBody() {
    if (mObjRigidBody) {
        mParent->dynamicsWorld()->removeRigidBody(mObjRigidBody);

        delete mObjShape;
        mObjShape = NULL;
        delete mObjMotionState;
        mObjMotionState = NULL;
        delete mObjRigidBody;
        mObjRigidBody = NULL;

        mParent->removeInternalTickObject(mID);
        mParent->removeDeactivateableObject(mID);
    }
}

void BulletRigidBodyObject::updateBulletFromObject(btTransform& worldTrans) {
    Vector3f objPosition = mParent->currentPosition(mID);
    Quaternion objOrient = mParent->currentOrientation(mID);
    worldTrans = btTransform(
        btQuaternion(objOrient.x,objOrient.y,objOrient.z,objOrient.w),
        btVector3(objPosition.x,objPosition.y,objPosition.z)
    );
}

void BulletRigidBodyObject::updateObjectFromBullet(const btTransform& worldTrans) {
    assert(mFixed == false);

    LocationInfo& locinfo = mParent->info(mID);

    btVector3 pos = worldTrans.getOrigin();
    btVector3 vel = mObjRigidBody->getLinearVelocity();
    TimedMotionVector3f newLocation(mParent->context()->simTime(), MotionVector3f(Vector3f(pos.x(), pos.y(), pos.z()), Vector3f(vel.x(), vel.y(), vel.z())));
    mParent->setLocation(mID, newLocation);
    BULLETLOG(insane, "Updating " << mID << " to velocity " << vel.x() << " " << vel.y() << " " << vel.z());
    btQuaternion rot = worldTrans.getRotation();
    btVector3 angvel = mObjRigidBody->getAngularVelocity();
    Vector3f angvel_siri(angvel.x(), angvel.y(), angvel.z());
    float angvel_angle = angvel_siri.normalizeThis();
    TimedMotionQuaternion newOrientation(
        mParent->context()->simTime(),
        MotionQuaternion(
            Quaternion(rot.x(), rot.y(), rot.z(), rot.w()),
            Quaternion(angvel_siri, angvel_angle)
        )
    );
    mParent->setOrientation(mID, newOrientation);

    mParent->addUpdate(mID);
}


namespace {

void capLinearVelocity(btRigidBody* rb, float32 max_speed) {
    btVector3 vel = rb->getLinearVelocity();
    float32 speed = vel.length();
    if (speed > max_speed)
        rb->setLinearVelocity( vel * (max_speed / speed) );
}

void capAngularVelocity(btRigidBody* rb, float32 max_speed) {
    btVector3 vel = rb->getAngularVelocity();
    float32 speed = vel.length();
    if (speed > max_speed)
        rb->setAngularVelocity( vel * (max_speed / speed) );
}

}

void BulletRigidBodyObject::internalTick(const Time& t) {
    assert(mObjRigidBody != NULL);
    capLinearVelocity(mObjRigidBody, 100);
    capAngularVelocity(mObjRigidBody, 4*3.14159);
}

void BulletRigidBodyObject::deactivationTick(const Time& t) {
    if (mObjRigidBody != NULL && !mObjRigidBody->isActive())
        mParent->updateObjectFromDeactivation(mID);
}


bool BulletRigidBodyObject::applyRequestedLocation(const TimedMotionVector3f& loc, uint64 epoch) {
    // We can move any dynamic objects, but we'll require that static objects
    // have physics turned off, move, then turn physics back on.
    if (mTreatment == BULLET_OBJECT_TREATMENT_STATIC)
        return false;

    applyForcedLocation(loc, epoch);
    return true;
}

bool BulletRigidBodyObject::applyRequestedOrientation(const TimedMotionQuaternion& orient, uint64 epoch) {
    // We can move any dynamic objects, but we'll require that static objects
    // have physics turned off, move, then turn physics back on.
    if (mTreatment == BULLET_OBJECT_TREATMENT_STATIC)
        return false;

    applyForcedOrientation(orient, epoch);
    return true;
}

void BulletRigidBodyObject::applyForcedLocation(const TimedMotionVector3f& loc, uint64 epoch) {
    // Update recorded info
    LocationInfo& locinfo = mParent->info(mID);
    locinfo.props.setLocation(loc, epoch);

    //mesh for mObjRigidBody may not have been loaded yet before
    //request to update that occurs
    //in addRigidBody will overwrite current position anyways, so don't lose
    //any information from discarding move here.
    if (mObjRigidBody == NULL)
      return;
    
    // Setting the motion state triggers a sync, even if its the same one that
    // was already being used.
    mObjRigidBody->setMotionState(mObjMotionState);
    // Activate the object in case it's gone to sleep from being still
    mObjRigidBody->activate();
}

void BulletRigidBodyObject::applyForcedOrientation(const TimedMotionQuaternion& orient, uint64 epoch) {
    // Update recorded info
    LocationInfo& locinfo = mParent->info(mID);
    locinfo.props.setOrientation(orient, epoch);

    //mesh for mObjRigidBody may not have been loaded yet before
    //request to update that occurs
    //in addRigidBody will overwrite current position anyways, so don't lose
    //any information from discarding move here.
    if (mObjRigidBody == NULL)
      return;


    // Setting the motion state triggers a sync, even if its the same one that
    // was already being used.
    mObjRigidBody->setMotionState(mObjMotionState);
    // Activate the object in case it's gone to sleep from being still
    mObjRigidBody->activate();
}

} // namespace Sirikata
