/*  Sirikata
 *  BulletPhysicsService.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava, Rahul Sheth
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIRIKATA_BULLET_PHYSICS_SERVICE_HPP_
#define _SIRIKATA_BULLET_PHYSICS_SERVICE_HPP_

#include <sirikata/space/LocationService.hpp>
#include "btBulletDynamicsCommon.h"

#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

namespace Sirikata {

class SirikataMotionState;

//FIXME Enums for manual treatment of objects and bboxes
//IGNORE = Bullet shouldn't know about this object
//STATIC = Bullet thinks this is a static (not moving) object
//DYNAMIC = Bullet thinks this is a dynamic (moving) object
enum bulletObjTreatment {
	BULLET_OBJECT_TREATMENT_IGNORE,
	BULLET_OBJECT_TREATMENT_STATIC,
	BULLET_OBJECT_TREATMENT_DYNAMIC
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

//this struct helps us delete objects and their physics data
struct BulletPhysicsPointerData {
	BulletPhysicsPointerData() {
		objShape = NULL;
		objMotionState = NULL;
		objRigidBody = NULL;

		//FIXME: get rid of these when the physics details don't need to be hardcoded
		objTreatment = BULLET_OBJECT_TREATMENT_IGNORE;
		objBBox = BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT;
	}
	btCollisionShape * objShape;
	SirikataMotionState* objMotionState;
	btRigidBody* objRigidBody;
	bulletObjTreatment objTreatment;
	bulletObjBBox objBBox;
}; // struct BulletPhysicsPointerData

using namespace Mesh;
/** Standard location service, which functions entirely based on location
 *  updates from objects and other spaces servers.
 */
class BulletPhysicsService : public LocationService {
	friend class SirikataMotionState;
public:
    BulletPhysicsService(SpaceContext* ctx, LocationUpdatePolicy* update_policy);
    // FIXME add constructor which can add all the objects being simulated to mLocations

	virtual ~BulletPhysicsService();

    virtual bool contains(const UUID& uuid) const;
    virtual TrackingType type(const UUID& uuid) const;

    virtual void service();

    virtual TimedMotionVector3f location(const UUID& uuid);
    virtual Vector3f currentPosition(const UUID& uuid);
    virtual TimedMotionQuaternion orientation(const UUID& uuid);
    virtual Quaternion currentOrientation(const UUID& uuid);
    virtual BoundingSphere3f bounds(const UUID& uuid);
    virtual const String& mesh(const UUID& uuid);
    virtual const String& physics(const UUID& uuid);

    virtual void addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void removeLocalObject(const UUID& uuid);

    virtual void addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void removeLocalAggregateObject(const UUID& uuid);
    virtual void updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void updateLocalAggregateMesh(const UUID& uuid, const String& newval);
    virtual void updateLocalAggregatePhysics(const UUID& uuid, const String& newval);

    virtual void addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void removeReplicaObject(const Time& t, const UUID& uuid);

    virtual void receiveMessage(Message* msg);

    virtual void locationUpdate(UUID source, void* buffer, uint32 length);

    void getMesh(const std::string, const UUID);

	void getMetadata(const std::string, const UUID);

	void metadataFinished(const UUID, std::string, std::tr1::shared_ptr<Transfer::MetadataRequest>, std::tr1::shared_ptr<Transfer::RemoteFileMetadata>);

	void chunkFinished(const UUID, std::string, std::tr1::shared_ptr<Transfer::ChunkRequest>, std::tr1::shared_ptr<const Transfer::DenseData>);

protected:
    struct LocationInfo {
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        BoundingSphere3f bounds;
        String mesh;
        String physics;
        bool local;
        bool aggregate;
        bool isFixed;
    };
    typedef std::tr1::unordered_map<UUID, LocationInfo, UUID::Hasher> LocationMap;

	LocationMap mLocations;

	std::vector<UUID> physicsUpdates;

private:

    //Bullet Dynamics World Vars
    btBroadphaseInterface* broadphase;

	btDefaultCollisionConfiguration* collisionConfiguration;

	btCollisionDispatcher* dispatcher;

	btSequentialImpulseConstraintSolver* solver;

	btDiscreteDynamicsWorld* dynamicsWorld;

	btRigidBody*  fallRigidBody;

	Timer mTimer;

	//load meshes to create appropriate bounding volumes
	ModelsSystem* mModelsSystem;

	Transfer::TransferMediator *mTransferMediator;

	std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;

	//at some point, replace with something better (semaphores, polling, etc.)
	bool meshLoaded;
	MeshdataPtr retrievedMesh;

	//can probably combine with LocationInfo and LocationMap
	typedef std::tr1::unordered_map<UUID, BulletPhysicsPointerData, UUID::Hasher> PhysicsPointerMap;

	PhysicsPointerMap BulletPhysicsPointers;

	bool firstCube;

	bool printDebugInfo;

}; // class BulletPhysicsService

class SirikataMotionState : public btMotionState {
public:
    SirikataMotionState(const btTransform &initialpos, BulletPhysicsService::LocationInfo * locinfo, UUID uuid, BulletPhysicsService* service) {
        mObjLocationInfo = locinfo;
        mPosition = initialpos;
        mUUID = uuid;
        ptrToService = service;
    }

    virtual ~SirikataMotionState() {
    }

    void setLocationInfo(BulletPhysicsService::LocationInfo * locinfo) {
        mObjLocationInfo = locinfo;
    }

    virtual void getWorldTransform(btTransform &worldTrans) const {
        worldTrans = mPosition;
    }

    virtual void setWorldTransform(const btTransform &worldTrans) {
        if(NULL == mObjLocationInfo) return;
        btQuaternion rot = worldTrans.getRotation();

        //FIXME this should work, but somewhere in the pipeline to Ogre w becomes NaN and we get an exception
        //printf("Old Orientation: %f, %f, %f, %f\n", mObjLocationInfo->orientation.position().x, mObjLocationInfo->orientation.position().y, mObjLocationInfo->orientation.position().z, mObjLocationInfo->orientation.position().w);
        TimedMotionQuaternion newOrientation(mObjLocationInfo->orientation.updateTime(), MotionQuaternion(Quaternion(rot.x(), rot.y(), rot.z(), rot.w()), mObjLocationInfo->orientation.velocity()));
        //printf("New Orientation: %f, %f, %f, %f\n", newOrientation.position().x, newOrientation.position().y, newOrientation.position().z, newOrientation.position().w);
        if(!(mObjLocationInfo->isFixed)) {
			//mObjLocationInfo->orientation = newOrientation;
		}

        btVector3 pos = worldTrans.getOrigin();
        //printf("Old Position: %f, %f, %f\n", mObjLocationInfo->location.position().x, mObjLocationInfo->location.position().y, mObjLocationInfo->location.position().z);
        TimedMotionVector3f newLocation(mObjLocationInfo->location.updateTime(), MotionVector3f(Vector3f(pos.x(), pos.y(), pos.z()), mObjLocationInfo->location.velocity()));
        if(!(mObjLocationInfo->isFixed)) {
			mObjLocationInfo->location = newLocation;
		}
        //printf("New Position: %f, %f, %f\n", pos.x(), pos.y(), pos.z());

        ptrToService->physicsUpdates.push_back(mUUID);
    }

protected:
    BulletPhysicsService::LocationInfo * mObjLocationInfo;
    btTransform mPosition;
    UUID mUUID;
    BulletPhysicsService * ptrToService;
}; // class SirikataMotionState

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_SERVICE_HPP_
