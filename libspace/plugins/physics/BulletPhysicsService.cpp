/*  Sirikata
 *  BulletPhysicsService.cpp
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

#include "BulletPhysicsService.hpp"
#include <sirikata/core/trace/Trace.hpp>

#include "Protocol_Loc.pbj.hpp"

namespace Sirikata {
#ifdef _WIN32
	//FIXME: is this the right thing? DRH
typedef float float_t;
#endif

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
        
        //printf("Old Orientation: %f, %f, %f\n", mObjLocationInfo->orientation.position().x, mObjLocationInfo->orientation.position().y, mObjLocationInfo->orientation.position().z);
        TimedMotionQuaternion newOrientation(mObjLocationInfo->orientation.updateTime(), MotionQuaternion(Quaternion(rot.x(), rot.y(), rot.z(), rot.w()), mObjLocationInfo->orientation.velocity()));
        //printf("New Orientation: %f, %f, %f\n", newOrientation.position().x, newOrientation.position().y, newOrientation.position().z);
        if(!(mObjLocationInfo->isFixed)) {
			mObjLocationInfo->orientation = newOrientation;
		}
        
        btVector3 pos = worldTrans.getOrigin();
        //printf("Old Position: %f, %f, %f\n", mObjLocationInfo->location.position().x, mObjLocationInfo->location.position().y, mObjLocationInfo->location.position().z);
        TimedMotionVector3f newLocation(mObjLocationInfo->location.updateTime(), MotionVector3f(Vector3<float_t>(pos.x(), pos.y(), pos.z()), mObjLocationInfo->location.velocity()));
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
};

BulletPhysicsService::BulletPhysicsService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : LocationService(ctx, update_policy)
{
	broadphase = new btDbvtBroadphase();
	
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	
	solver = new btSequentialImpulseConstraintSolver;
	
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	
	//TESTING: hello world bullet program
	dynamicsWorld->setGravity(btVector3(0,-1,0));
	
	/*btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);
	//btCollisionShape* fallShape = new btSphereShape(1);
		
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0,-50,0)));
	
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0,0,0));
	btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
	
	dynamicsWorld->addRigidBody(groundRigidBody);*/
	//btDefaultMotionState* fallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,50,0)));
	
	//btScalar mass = 1;
	//btVector3 fallInertia(0,0,0);
	//fallShape->calculateLocalInertia(mass,fallInertia);
	//btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallShape, fallInertia);
	//fallRigidBody = new btRigidBody(fallRigidBodyCI);
	//dynamicsWorld->addRigidBody(fallRigidBody);
	//END TESTING
	
	mTimer.start();
	
	mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());
    
    static char x = '1';
    mTransferPool = mTransferMediator->registerClient("BulletPhysics_"+x);
    x++;
	
	printf("BulletPhysicsService loaded - constructor\n");
}

BulletPhysicsService::~BulletPhysicsService(){
	delete dynamicsWorld;
	delete solver;
	delete dispatcher;
	delete collisionConfiguration;
	delete broadphase;
	delete mModelsSystem;
	
	printf("BulletPhysicsService unloaded - destructor\n");
}

bool BulletPhysicsService::contains(const UUID& uuid) const {
    return (mLocations.find(uuid) != mLocations.end());
}

LocationService::TrackingType BulletPhysicsService::type(const UUID& uuid) const {
    LocationMap::const_iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return NotTracking;
    if (it->second.aggregate)
        return Aggregate;
    if (it->second.local)
        return Local;
    return Replica;
}


void BulletPhysicsService::service() {
	Duration delTime = mTimer.elapsed();
	float simForwardTime = delTime.toMilliseconds() / 1000.0f;
	
    dynamicsWorld->stepSimulation(simForwardTime, 10);
	mTimer.start();
	    
    for(unsigned int i = 0; i < physicsUpdates.size(); i++) {
		LocationMap::iterator it = mLocations.find(physicsUpdates[i]);
		notifyLocalLocationUpdated( physicsUpdates[i], it->second.aggregate, it->second.location );		
	}
    
    mUpdatePolicy->service();
}

TimedMotionVector3f BulletPhysicsService::location(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());


    LocationInfo locinfo = it->second;
    return locinfo.location;
}

Vector3f BulletPhysicsService::currentPosition(const UUID& uuid) {
    TimedMotionVector3f loc = location(uuid);
    Vector3f returner = loc.extrapolate(mContext->simTime()).position();
    return returner;
}

TimedMotionQuaternion BulletPhysicsService::orientation(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.orientation;
}

Quaternion BulletPhysicsService::currentOrientation(const UUID& uuid) {
    TimedMotionQuaternion orient = orientation(uuid);
    return orient.extrapolate(mContext->simTime()).position();
}

BoundingSphere3f BulletPhysicsService::bounds(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo locinfo = it->second;
    return locinfo.bounds;
}

const String& BulletPhysicsService::mesh(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    const LocationInfo& locinfo = it->second;
    return locinfo.mesh;
}

void BulletPhysicsService::getMesh(const std::string meshURI, const UUID uuid) {
	
	meshLoaded = false;
	getMetadata(meshURI, uuid);
	//TEMPORARY: Please fix me! Busy waiting is bad.
	while(!meshLoaded) {};
	
    std::string t = retrievedMesh->uri;
	printf("Mesh was retrieved!: %s\n", &t[0]);
}

void BulletPhysicsService::getMetadata(const std::string meshURI, const UUID uuid) {
	Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshURI), 1.0, std::tr1::bind(
                                       &BulletPhysicsService::metadataFinished, this, uuid, meshURI,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);
}

void BulletPhysicsService::metadataFinished(const UUID uuid, std::string meshName,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response) {
	if (response != NULL) {
    const Transfer::RemoteFileMetadata metadata = *response;

    Transfer::TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata,
                                               response->getChunkList().front(), 1.0,
                                             std::tr1::bind(&BulletPhysicsService::chunkFinished, this, uuid, meshName,
                                                              std::tr1::placeholders::_1,
                                                              std::tr1::placeholders::_2) ) );

    mTransferPool->addRequest(req);
  }
  else {
    Transfer::TransferRequestPtr req(new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &BulletPhysicsService::metadataFinished, this, uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);
  }
}

void BulletPhysicsService::chunkFinished(const UUID uuid, std::string meshName,
                                       std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                       std::tr1::shared_ptr<const Transfer::DenseData> response)
{
	if (response != NULL) {
        retrievedMesh = mModelsSystem->load(request->getURI(), request->getMetadata().getFingerprint(), response);
        meshLoaded = true;
    }
    else {
     Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &BulletPhysicsService::metadataFinished, this, uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

     mTransferPool->addRequest(req);
	}
}
void BulletPhysicsService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh) {
    LocationMap::iterator it = mLocations.find(uuid);

    // Add or update the information to the cache
    if (it == mLocations.end()) {
        mLocations[uuid] = LocationInfo();
        it = mLocations.find(uuid);
    } else {
        // It was already in there as a replica, notify its removal
        assert(it->second.local == false);
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME remote server ID
        notifyReplicaObjectRemoved(uuid);
    }

    LocationInfo& locinfo = it->second;
    locinfo.location = loc;
    locinfo.orientation = orient;
    locinfo.bounds = bnds;
    locinfo.mesh = msh;
    locinfo.local = true;
    locinfo.aggregate = false;
    
    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, true, loc);
    notifyLocalObjectAdded(uuid, false, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid));
    
    /*BEGIN: Bullet Physics Code*/
    btCollisionShape* objShape;
    if(strcmp(&msh[0], "meerkat:///danielrh/ground.DAE") == 0 || strcmp(&msh[0], "meerkat:///danielrh/Poisonbird.dae") == 0) {
		printf("Mesh is fixed: %s\n", &msh[0]);    
		locinfo.isFixed =  true;
		
		//getting the mesh, creating the bounding volume
		getMesh(msh, uuid);
		Meshdata::GeometryInstanceIterator geoIter = retrievedMesh->getGeometryInstanceIterator();
		
		uint32 indexInstance; Matrix4x4f transformInstance;
		
		while(geoIter.next(&indexInstance, &transformInstance)) {
			GeometryInstance* geoInst = &(retrievedMesh->instances[indexInstance]);
			unsigned int geoIndx = geoInst->geometryIndex;
			SubMeshGeometry* subGeom = &(retrievedMesh->geometry[geoIndx]);
			unsigned int numOfPrimitives = subGeom->primitives.size();
			for(unsigned int i = 0; i < numOfPrimitives; i++) {
				//create bullet triangle array from our data structure
				std::vector<int> gIndices;
				btVector3 *gVertices= new btVector3[subGeom->positions.size()];
				for(unsigned int j=0; j < subGeom->primitives[i].indices.size(); j++) {
					gIndices.push_back((int)(subGeom->primitives[i].indices[j]));
				}
				for(unsigned int j=0; j < subGeom->positions.size(); j++) {
					btVector3 tmpVec = btVector3(subGeom->positions[j].x, subGeom->positions[j].y, subGeom->positions[j].z); 
					gVertices[j]=tmpVec;
				}
				//TODO: check memleak, check divisible by 3
				printf("btTriangleIndexVertexArray:\n");
				printf("argument 1: %d\n", (int) (subGeom->primitives[i].indices.size())/3);
				printf("argument 3: %d\n", (int) 3*sizeof(unsigned short));
				printf("argument 4: %d\n", subGeom->positions.size());
				printf("argument 6: %d\n", (int) sizeof(Vector3f));
				btTriangleIndexVertexArray* indexVertexArrays = new btTriangleIndexVertexArray((int) gIndices.size()/3, &gIndices[0], (int) 3*sizeof(int), subGeom->positions.size(), (btScalar *) &gVertices[0].x(), (int) sizeof(btVector3));
				
				btVector3 aabbMin(-1000,-1000,-1000),aabbMax(1000,1000,1000);
				objShape  = new btBvhTriangleMeshShape(indexVertexArrays,true,aabbMin,aabbMax);
				delete [] gVertices;
			}
		}
	}
    //CREATE: bounding sphere
    printf("bounds radius: %f\n", bnds.radius());
    
    //if(!(locinfo.isFixed))
		objShape = new btSphereShape(10);
    
    //CREATE: motion state and link to locationinfo
    Vector3<float_t> objPosition = loc.position();
    //printf("loc position: %f, %f, %f\n", objPosition.x, objPosition.y, objPosition.z);
    Quaternion objOrient = orient.position();
    SirikataMotionState* objMotionState = new SirikataMotionState(btTransform(btQuaternion(objOrient.x,objOrient.y,objOrient.z,objOrient.w),btVector3(objPosition.x,objPosition.y,objPosition.z)), &(it->second), uuid, this);
    
    //TEMPORARY: dont have mass, set to default value here
    //set mass
    btScalar mass = 1;
    //set a placeholder for the inertial vector
	btVector3 objInertia(0,0,0);
	//calculate the inertia
	objShape->calculateLocalInertia(mass,objInertia);
	//make a constructionInfo object
	btRigidBody::btRigidBodyConstructionInfo objRigidBodyCI(mass, objMotionState, objShape, objInertia);
    //CREATE: make the rigid body
    btRigidBody* objRigidBody = new btRigidBody(objRigidBodyCI);
    //set initial velocity
    Vector3<float_t> objVelocity = loc.velocity();
    objRigidBody->setLinearVelocity(btVector3(objVelocity.x, objVelocity.y, objVelocity.z));
    //add to the dynamics world
    dynamicsWorld->addRigidBody(objRigidBody);
        
    /*END: Bullet Physics Code*/
}

void BulletPhysicsService::removeLocalObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == false );
    mLocations.erase(uuid);

    // Remove from the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, false, TimedMotionVector3f());
    notifyLocalObjectRemoved(uuid, false);

    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.
    
}

void BulletPhysicsService::addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh) {
    // Aggregates get randomly assigned IDs -- if there's a conflict either we
    // got a true conflict (incredibly unlikely) or somebody (prox/query
    // handler) screwed up.
    assert(mLocations.find(uuid) == mLocations.end());

    mLocations[uuid] = LocationInfo();
    LocationMap::iterator it = mLocations.find(uuid);

    LocationInfo& locinfo = it->second;
    locinfo.location = loc;
    locinfo.orientation = orient;
    locinfo.bounds = bnds;
    locinfo.mesh = msh;
    locinfo.local = true;
    locinfo.aggregate = true;

    // Add to the list of local objects
    notifyLocalObjectAdded(uuid, true, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid));
}

void BulletPhysicsService::removeLocalAggregateObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == true );
    mLocations.erase(uuid);

    notifyLocalObjectRemoved(uuid, true);
}

void BulletPhysicsService::updateLocalAggregateLocation(const UUID& uuid, const TimedMotionVector3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.location = newval;
    notifyLocalLocationUpdated( uuid, true, newval );
}
void BulletPhysicsService::updateLocalAggregateOrientation(const UUID& uuid, const TimedMotionQuaternion& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.orientation = newval;
    notifyLocalOrientationUpdated( uuid, true, newval );
}
void BulletPhysicsService::updateLocalAggregateBounds(const UUID& uuid, const BoundingSphere3f& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.bounds = newval;
    notifyLocalBoundsUpdated( uuid, true, newval );
}
void BulletPhysicsService::updateLocalAggregateMesh(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    loc_it->second.mesh = newval;
    notifyLocalMeshUpdated( uuid, true, newval );
}

void BulletPhysicsService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh) {
    // FIXME we should do checks on timestamps to decide which setting is "more" sane
    LocationMap::iterator it = mLocations.find(uuid);

    if (it != mLocations.end()) {
        // It already exists. If its local, ignore the update. If its another replica, somethings out of sync, but perform the update anyway
        LocationInfo& locinfo = it->second;
        if (!locinfo.local) {
            locinfo.location = loc;
            locinfo.orientation = orient;
            locinfo.bounds = bnds;
            locinfo.mesh = msh;
            //local = false
            // FIXME should we notify location and bounds updated info?
        }
        // else ignore
    }
    else {
        // Its a new replica, just insert it
        LocationInfo locinfo;
        locinfo.location = loc;
        locinfo.orientation = orient;
        locinfo.bounds = bnds;
        locinfo.mesh = msh;
        locinfo.local = false;
        locinfo.aggregate = false;
        mLocations[uuid] = locinfo;

        // We only run this notification when the object actually is new
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, true, loc); // FIXME add remote server ID
        notifyReplicaObjectAdded(uuid, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid));
    }
}

void BulletPhysicsService::removeReplicaObject(const Time& t, const UUID& uuid) {
    // FIXME we should maintain some time information and check t against it to make sure this is sane

    LocationMap::iterator it = mLocations.find(uuid);
    if (it == mLocations.end())
        return;

    // If the object is marked as local, this is out of date information.  Just ignore it.
    LocationInfo& locinfo = it->second;
    if (locinfo.local)
        return;

    // Otherwise, remove and notify
    mLocations.erase(uuid);
    CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, false, TimedMotionVector3f()); // FIXME add remote server ID
    notifyReplicaObjectRemoved(uuid);
}


void BulletPhysicsService::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_LOCATION);
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    bool parsed = parsePBJMessage(&contents, msg->payload());

    if (parsed) {
        for(int32 idx = 0; idx < contents.update_size(); idx++) {
            Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

            // Its possible we'll get an out of date update. We only use this update
            // if (a) we have this object marked as a replica object and (b) we don't
            // have this object marked as a local object
            if (type(update.object()) != Replica)
                continue;

            LocationMap::iterator loc_it = mLocations.find( update.object() );
            // We can safely make this assertion right now because space server
            // to space server loc and prox are on the same reliable channel. If
            // this goes away then a) we can't make this assertion and b) we
            // need an OrphanLocUpdateManager to save updates where this
            // condition is false so they can be applied once the prox update
            // arrives.
            assert(loc_it != mLocations.end());

            if (update.has_location()) {
                TimedMotionVector3f newloc(
                    update.location().t(),
                    MotionVector3f( update.location().position(), update.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyReplicaLocationUpdated( update.object(), newloc );

                CONTEXT_SPACETRACE(serverLoc, msg->source_server(), mContext->id(), update.object(), newloc );
            }

            if (update.has_orientation()) {
                TimedMotionQuaternion neworient(
                    update.orientation().t(),
                    MotionQuaternion( update.orientation().position(), update.orientation().velocity() )
                );
                loc_it->second.orientation = neworient;
                notifyReplicaOrientationUpdated( update.object(), neworient );
            }

            if (update.has_bounds()) {
                BoundingSphere3f newbounds = update.bounds();
                loc_it->second.bounds = newbounds;
                notifyReplicaBoundsUpdated( update.object(), newbounds );
            }

            if (update.has_mesh()) {
                String newmesh = update.mesh();
                loc_it->second.mesh = newmesh;
                notifyReplicaMeshUpdated( update.object(), newmesh );
            }
        }
    }

    delete msg;
}

void BulletPhysicsService::locationUpdate(UUID source, void* buffer, uint32 length) {
    Sirikata::Protocol::Loc::Container loc_container;
    bool parse_success = loc_container.ParseFromString( String((char*) buffer, length) );
    if (!parse_success) {
        LOG_INVALID_MESSAGE_BUFFER(standardloc, error, ((char*)buffer), length);
        return;
    }

    if (loc_container.has_update_request()) {
        Sirikata::Protocol::Loc::LocationUpdateRequest request = loc_container.update_request();

        TrackingType obj_type = type(source);
        if (obj_type == Local) {
            LocationMap::iterator loc_it = mLocations.find( source );
            assert(loc_it != mLocations.end());

            if (request.has_location()) {
                TimedMotionVector3f newloc(
                    request.location().t(),
                    MotionVector3f( request.location().position(), request.location().velocity() )
                );
                loc_it->second.location = newloc;
                notifyLocalLocationUpdated( source, loc_it->second.aggregate, newloc );

                CONTEXT_SPACETRACE(serverLoc, mContext->id(), mContext->id(), source, newloc );
            }

            if (request.has_bounds()) {
                BoundingSphere3f newbounds = request.bounds();
                loc_it->second.bounds = newbounds;
                notifyLocalBoundsUpdated( source, loc_it->second.aggregate, newbounds );
            }

            if (request.has_mesh()) {
                String newmesh = request.mesh();
                loc_it->second.mesh = newmesh;
                notifyLocalMeshUpdated( source, loc_it->second.aggregate, newmesh );
            }

            if (request.has_orientation()) {
                TimedMotionQuaternion neworient(
                    request.orientation().t(),
                    MotionQuaternion( request.orientation().position(), request.orientation().velocity() )
                );
                loc_it->second.orientation = neworient;
                notifyLocalOrientationUpdated( source, loc_it->second.aggregate, neworient );
            }

        }
        else {
            // Warn about update to non-local object
        }
    }
}



} // namespace Sirikata
