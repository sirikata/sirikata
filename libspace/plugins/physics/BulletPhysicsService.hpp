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
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {

class SirikataMotionState;

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


using namespace Mesh;
/** Standard location service, which functions entirely based on location
 *  updates from objects and other spaces servers.
 */
class BulletPhysicsService : public LocationService {
	friend class SirikataMotionState;

    // Gets the collision mask for a given type of object
    static void getCollisionMask(bulletObjTreatment treatment, bulletObjCollisionMaskGroup* mygroup, bulletObjCollisionMaskGroup* collide_with);
public:
    BulletPhysicsService(SpaceContext* ctx, LocationUpdatePolicy* update_policy);
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
    // Added for Bullet implementation
    bool isFixed(const UUID& uuid);
    void setLocation(const UUID& uuid, const TimedMotionVector3f& newloc);
    void setOrientation(const UUID& uuid, const TimedMotionQuaternion& neworient);


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

    typedef std::tr1::function<void(MeshdataPtr)> MeshdataParsedCallback;
    void getMesh(const std::string meshURI, const UUID uuid, MeshdataParsedCallback cb);
    // The last two get set in this callback, indicating that the
    // transfer finished (whether or not it was successful) and the
    // resulting data.
    void getMeshCallback(Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr response, MeshdataParsedCallback cb);


    void updateBulletFromObject(const UUID& uuid, btTransform& worldTrans);
    void updateObjectFromBullet(const UUID& uuid, const btTransform& worldTrans);

    // Callback invoked each time bullet performs an internal tick
    // (may be finer granularity than we request).
    void internalTickCallback();
protected:
    struct LocationInfo {
	LocationInfo();

        // Regular location info that we need to maintain for all objects
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        BoundingSphere3f bounds;
        String mesh;
        String physics;
        bool local;
        bool aggregate;

        // Bullet specific data. First some basic properties:
        bool isFixed;
	bulletObjTreatment objTreatment;
	bulletObjBBox objBBox;
        float32 mass;
        // And then some implementation data:
	btCollisionShape * objShape;
	SirikataMotionState* objMotionState;
	btRigidBody* objRigidBody;
    };

    typedef std::tr1::unordered_map<UUID, LocationInfo, UUID::Hasher> LocationMap;
    LocationMap mLocations;

    typedef std::tr1::unordered_set<UUID, UUID::Hasher> UUIDSet;
    // Which objects have dynamic physical simulation and need to be
    // sanity checked at each tick.
    UUIDSet mDynamicPhysicsObjects;
    // Objects which have outstanding updates to location information
    // from the physics engine.
    UUIDSet physicsUpdates;

    typedef std::tr1::unordered_map<UUID, Transfer::ResourceDownloadTaskPtr, UUID::Hasher> MeshDownloadMap;
    MeshDownloadMap mMeshDownloads;

private:

    void updatePhysicsWorld(const UUID& uuid);
    // This continues the work of updatePhysicsWorld once the mesh has
    // been retrieved.
    void updatePhysicsWorldWithMesh(const UUID& uuid, MeshdataPtr retrievedMesh);

    // This does the work of actually adding the rigid body to the
    // bullet simulation.
    void addRigidBody(const UUID& uuid, LocationInfo& locinfo);
    // This clears a rigid body from the bullet simulation, and clears
    // the associated state in the LocationInfo.
    void removeRigidBody(const UUID& uuid, LocationInfo& locinfo);

    //Bullet Dynamics World Vars
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

    Time mLastTime;

    //load meshes to create appropriate bounding volumes
    ModelsSystem* mModelsSystem;
    Mesh::Filter* mModelFilter;

    Transfer::TransferMediator *mTransferMediator;
    Transfer::TransferPoolPtr mTransferPool;
}; // class BulletPhysicsService

class SirikataMotionState : public btMotionState {
public:
    SirikataMotionState(BulletPhysicsService* service, UUID uuid)
     : ptrToService(service),
       mUUID(uuid)
    {
    }

    virtual ~SirikataMotionState() {
    }

    virtual void getWorldTransform(btTransform &worldTrans) const {
        ptrToService->updateBulletFromObject(mUUID, worldTrans);
    }

    virtual void setWorldTransform(const btTransform &worldTrans) {
        ptrToService->updateObjectFromBullet(mUUID, worldTrans);
    }

protected:
    BulletPhysicsService* ptrToService;
    UUID mUUID;
}; // class SirikataMotionState

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_SERVICE_HPP_
