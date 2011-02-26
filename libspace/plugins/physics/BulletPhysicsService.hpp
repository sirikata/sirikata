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

    virtual void addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void removeLocalObject(const UUID& uuid);

    virtual void addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void removeLocalAggregateObject(const UUID& uuid);
    virtual void updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void updateLocalAggregateMesh(const UUID& uuid, const String& newval);

    virtual void addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void removeReplicaObject(const Time& t, const UUID& uuid);

    virtual void receiveMessage(Message* msg);

    virtual void locationUpdate(UUID source, void* buffer, uint32 length);
    
    void getMesh(const string, const UUID);
    
	void getMetadata(const string, const UUID);

	void metadataFinished(const UUID, std::string, std::tr1::shared_ptr<Transfer::MetadataRequest>, std::tr1::shared_ptr<Transfer::RemoteFileMetadata>);

	void chunkFinished(const UUID, std::string, std::tr1::shared_ptr<Transfer::ChunkRequest>, std::tr1::shared_ptr<const Transfer::DenseData>);    

protected:
    struct LocationInfo {
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        BoundingSphere3f bounds;
        String mesh;
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
	
}; // class BulletPhysicsService

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_SERVICE_HPP_
