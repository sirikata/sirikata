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
#include <sirikata/mesh/Meshdata.hpp>

#include "Defs.hpp"

namespace Sirikata {

using namespace Mesh;
/** Standard location service, which functions entirely based on location
 *  updates from objects and other spaces servers.
 */
class BulletPhysicsService : public LocationService {
public:
    BulletPhysicsService(SpaceContext* ctx, LocationUpdatePolicy* update_policy);
    virtual ~BulletPhysicsService();

    virtual bool contains(const UUID& uuid) const;
    virtual TrackingType type(const UUID& uuid) const;

    virtual void service();

    virtual uint64 epoch(const UUID& uuid);
    virtual TimedMotionVector3f location(const UUID& uuid);
    virtual Vector3f currentPosition(const UUID& uuid);
    virtual TimedMotionQuaternion orientation(const UUID& uuid);
    virtual Quaternion currentOrientation(const UUID& uuid);
    virtual BoundingSphere3f bounds(const UUID& uuid);
    virtual const String& mesh(const UUID& uuid);
    virtual const String& physics(const UUID& uuid);
    // Added for Bullet implementation
    bool isFixed(const UUID& uuid);
    // Returns true if the current settings for the object allow
    // motion requests (location, orientation) to be directly applied.
    // Even when false, other requests, like changing bounds, mesh,
    // and physics settings, should still go through.
    bool directMotionRequestsEnabled(const UUID& uuid);
    void setLocation(const UUID& uuid, const TimedMotionVector3f& newloc);
    void setOrientation(const UUID& uuid, const TimedMotionQuaternion& neworient);


  virtual void addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics, const String& zernike);
    virtual void removeLocalObject(const UUID& uuid);

    virtual void addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void removeLocalAggregateObject(const UUID& uuid);
    virtual void updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void updateLocalAggregateMesh(const UUID& uuid, const String& newval);
    virtual void updateLocalAggregatePhysics(const UUID& uuid, const String& newval);

  virtual void addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics, const String& zernike);
    virtual void removeReplicaObject(const Time& t, const UUID& uuid);

    virtual void receiveMessage(Message* msg);

    virtual bool locationUpdate(UUID source, void* buffer, uint32 length);

    typedef std::tr1::function<void(MeshdataPtr)> MeshdataParsedCallback;
    void getMesh(const Transfer::URI meshURI, const UUID uuid, MeshdataParsedCallback cb);
    // The last two get set in this callback, indicating that the
    // transfer finished (whether or not it was successful) and the
    // resulting data.
    void getMeshCallback(Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response, MeshdataParsedCallback cb);

    LocationInfo& info(const UUID& uuid);
    const LocationInfo& info(const UUID& uuid) const;

    btDiscreteDynamicsWorld* dynamicsWorld() { return mDynamicsWorld; }
    btBroadphaseInterface* broadphase() { return mBroadphase; }

    // Objects that want callbacks for each tick, e.g. for grabbing updates that
    // aren't emitted automatically or updating velocity
    void addTickObject(const UUID& uuid);
    void removeTickObject(const UUID& uuid);
    // Objects that want callbacks for each internal tick, e.g. for capping
    // velocity
    void addInternalTickObject(const UUID& uuid);
    void removeInternalTickObject(const UUID& uuid);
    // Objects that want deactivation check callbacks
    void addDeactivateableObject(const UUID& uuid);
    void removeDeactivateableObject(const UUID& uuid);

    // Add an update for this object, i.e. it was detected that it moved
    void addUpdate(const UUID& uuid);

    void updateObjectFromDeactivation(const UUID& uuid);

    // Callback invoked each time bullet performs an internal tick
    // (may be finer granularity than we request).
    void internalTickCallback();
protected:
    typedef std::tr1::unordered_map<UUID, LocationInfo, UUID::Hasher> LocationMap;
    LocationMap mLocations;

    typedef std::tr1::unordered_set<UUID, UUID::Hasher> UUIDSet;
    // Which objects have dynamic physical simulation and need to be
    // sanity checked at each tick.
    UUIDSet mTickObjects;
    // Which objects have dynamic physical simulation and need to be
    // sanity checked at each tick.
    UUIDSet mInternalTickObjects;
    // Objects that need to be checked for deactivation
    UUIDSet mDeactivateableObjects;
    // Objects which have outstanding updates to location information
    // from the physics engine.
    UUIDSet physicsUpdates;
    // TODO(ewencp) This is kind of a hack. If we generate updates too quickly
    // we can overwhelm the client and the networking, making it hard for more
    // recent updates to get out. This is common for bullet since it is
    // constantly updating positions. To avoid this, we trigger the update
    // policy less frequently, possibly at the cost of higher average latency
    // for updates to reach the OH.
    uint32 mUpdateIteration;

    typedef std::tr1::unordered_map<UUID, Transfer::ResourceDownloadTaskPtr, UUID::Hasher> MeshDownloadMap;
    MeshDownloadMap mMeshDownloads;

private:

    void updatePhysicsWorld(const UUID& uuid);
    // This continues the work of updatePhysicsWorld once the mesh has
    // been retrieved.
    void updatePhysicsWorldWithMesh(const UUID& uuid, MeshdataPtr retrievedMesh);

    // Helper for cleaning up a LocationInfo before removing it
    void cleanupLocationInfo(LocationInfo& locinfo);


    //Bullet Dynamics World Vars
    btBroadphaseInterface* mBroadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* mDynamicsWorld;

    Time mLastTime;
    // Track last time we checked deactivation state
    Time mLastDeactivationTime;

    //load meshes to create appropriate bounding volumes
    ModelsSystem* mModelsSystem;
    Mesh::Filter* mModelFilter;

    Transfer::TransferMediator *mTransferMediator;
    Transfer::TransferPoolPtr mTransferPool;
    Network::IOStrand* mParsingStrand;
}; // class BulletPhysicsService

} // namespace Sirikata

#endif //_SIRIKATA_BULLET_PHYSICS_SERVICE_HPP_
