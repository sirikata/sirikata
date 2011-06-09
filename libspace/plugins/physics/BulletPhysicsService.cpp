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

// Property tree for parsing the physics info
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define BULLETLOG(lvl, msg) SILOG(BulletPhysics, lvl, msg)

namespace Sirikata {
#ifdef _WIN32
	//FIXME: is this the right thing? DRH
typedef float float_t;
#endif


BulletPhysicsService::LocationInfo::LocationInfo()
 : location(),
   orientation(),
   bounds(),
   mesh(),
   physics(),
   local(),
   aggregate(),
   isFixed(true),
   objTreatment(BULLET_OBJECT_TREATMENT_IGNORE),
   objBBox(BULLET_OBJECT_BOUNDS_SPHERE),
   mass(1.f),
   objShape(NULL),
   objMotionState(NULL),
   objRigidBody(NULL)
{
}

namespace {
void bulletPhysicsInternalTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    BulletPhysicsService *bps = static_cast<BulletPhysicsService*>(world->getWorldUserInfo());
    bps->internalTickCallback();
}
}

BulletPhysicsService::BulletPhysicsService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : LocationService(ctx, update_policy)
{

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setInternalTickCallback(bulletPhysicsInternalTickCallback, (void*)this);
    dynamicsWorld->setGravity(btVector3(0,-1,0));

    mLastTime = mContext->simTime();

    mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");
    mTransferMediator = &(Transfer::TransferMediator::getSingleton());
    mTransferPool = mTransferMediator->registerClient("BulletPhysics");

    BULLETLOG(detailed, "Service Loaded");
}

BulletPhysicsService::~BulletPhysicsService(){
	delete dynamicsWorld;
	delete solver;
	delete dispatcher;
	delete collisionConfiguration;
	delete broadphase;
	delete mModelsSystem;

	BULLETLOG(detailed,"Service Unloaded");
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
	//get the time elapsed between calls to this service and
	//move the simulation forward by that amount
    Time now = mContext->simTime();
    Duration delTime = now - mLastTime;
    mLastTime = now;
    float simForwardTime = delTime.toMilliseconds() / 1000.0f;

    dynamicsWorld->stepSimulation(simForwardTime);

    for(UUIDSet::iterator i = physicsUpdates.begin(); i != physicsUpdates.end(); i++) {
        LocationMap::iterator it = mLocations.find(*i);
        if(it != mLocations.end())
            notifyLocalLocationUpdated(*i, it->second.aggregate, it->second.location );
    }
    physicsUpdates.clear();

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

const String& BulletPhysicsService::physics(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    const LocationInfo& locinfo = it->second;
    return locinfo.physics;
}

bool BulletPhysicsService::isFixed(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    const LocationInfo& locinfo = it->second;
    return locinfo.isFixed;
}

void BulletPhysicsService::setLocation(const UUID& uuid, const TimedMotionVector3f& newloc) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.location = newloc;
    notifyLocalLocationUpdated( uuid, locinfo.aggregate, newloc );
}

void BulletPhysicsService::setOrientation(const UUID& uuid, const TimedMotionQuaternion& neworient) {
    LocationMap::iterator it = mLocations.find(uuid);
    assert(it != mLocations.end());

    LocationInfo& locinfo = it->second;
    locinfo.orientation = neworient;
    notifyLocalOrientationUpdated( uuid, locinfo.aggregate, neworient );
}

void BulletPhysicsService::getMesh(const std::string meshURI, const UUID uuid, MeshdataParsedCallback cb) {
    Transfer::ResourceDownloadTaskPtr dl = Transfer::ResourceDownloadTask::construct(
        Transfer::URI(meshURI), mTransferPool, 1.0,
        std::tr1::bind(&BulletPhysicsService::getMeshCallback, this, _1, _2, cb)
    );
    mMeshDownloads[uuid] = dl;
    dl->start();
}

void BulletPhysicsService::getMeshCallback(Transfer::ChunkRequestPtr request, Transfer::DenseDataPtr response, MeshdataParsedCallback cb) {
    // This callback can come in on a separate thread (e.g. from a tranfer
    // thread) so make sure we get it back on the main thread.
    if (request && response) {
        MeshdataPtr mesh = mModelsSystem->load(request->getURI(), request->getMetadata().getFingerprint(), response);
        mContext->mainStrand->post(std::tr1::bind(cb, mesh));
    }
    else {
        mContext->mainStrand->post(std::tr1::bind(cb, MeshdataPtr()));
    }
}

void BulletPhysicsService::addLocalObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh, const String& phy) {
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
    locinfo.physics = phy;
    locinfo.local = true;
    locinfo.aggregate = false;

    // FIXME: we might want to verify that location(uuid) and bounds(uuid) are
    // reasonable compared to the loc and bounds passed in

    // Add to the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, true, loc);
    notifyLocalObjectAdded(uuid, false, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid));

    updatePhysicsWorld(uuid);
}

#define DEFAULT_TREATMENT BULLET_OBJECT_TREATMENT_IGNORE
#define DEFAULT_BOUNDS BULLET_OBJECT_BOUNDS_SPHERE
#define DEFAULT_MASS 1.f

void BulletPhysicsService::updatePhysicsWorld(const UUID& uuid) {
    LocationMap::iterator it = mLocations.find(uuid);
    LocationInfo& locinfo = it->second;
    const String& msh = locinfo.mesh;
    const String& phy = locinfo.physics;

    /*BEGIN: Bullet Physics Code*/

    // Default values
    bulletObjTreatment objTreatment = DEFAULT_TREATMENT;
    bulletObjBBox objBBox = DEFAULT_BOUNDS;
    btScalar mass = DEFAULT_MASS;

    if (!phy.empty()) {
        // Parsing stage
        using namespace boost::property_tree;
        ptree pt;
        try {
            std::stringstream phy_json(phy);
            read_json(phy_json, pt);
        }
        catch(json_parser::json_parser_error exc) {
            BULLETLOG(error, "Error parsing physics properties: " << phy << " (" << exc.what() << ")");
            return;
        }

        String objTreatmentString = pt.get("treatment", String("ignore"));
        if (objTreatmentString == "static") objTreatment = BULLET_OBJECT_TREATMENT_STATIC;
        if (objTreatmentString == "dynamic") objTreatment = BULLET_OBJECT_TREATMENT_DYNAMIC;

        String objBBoxString = pt.get("bounds", String("sphere"));
        if (objBBoxString == "box") objBBox = BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT;
        if (objBBoxString == "triangles") objBBox = BULLET_OBJECT_BOUNDS_PER_TRIANGLE;

        mass = pt.get("mass", DEFAULT_MASS);
    }

    // Check if anything has changed.
    if (locinfo.objTreatment == objTreatment &&
        locinfo.objBBox == objBBox &&
        locinfo.mass == mass)
        return;

    // Clear out previous state from the simulation.
    removeRigidBody(uuid, locinfo);

    // Store updated info
    locinfo.objTreatment = objTreatment;
    locinfo.objBBox = objBBox;
    locinfo.mass = mass;

    // And then proceed to add the new simulated object into bullet

    //objTreatment enum defined in header file
    //using if/elseif here to avoid switch/case compiler complaints (initializing variables in a case)
    if(objTreatment == BULLET_OBJECT_TREATMENT_IGNORE) {
        BULLETLOG(detailed," This mesh will not be added to the bullet world: " << msh[0]);
        return;
    }
    else if(objTreatment == BULLET_OBJECT_TREATMENT_STATIC) {
        BULLETLOG(detailed, "This mesh will not move: " << msh[0]);
        //this is a variable in loc structure that sets the item to be static
        locinfo.isFixed = true;
        //if the mass is 0, Bullet treats the object as static
        locinfo.mass = 0;
    }
    else if(objTreatment == BULLET_OBJECT_TREATMENT_DYNAMIC) {
        BULLETLOG(detailed, "This mesh will move: " << msh[0]);
        locinfo.isFixed = false;
    }
    else {
        BULLETLOG(detailed,"Error in objTreatment initialization!");
    }

    // We may need the mesh in order to continue. We need it only if:
    // treatment != ignore (see above check) && bounds != sphere.
    if (objBBox == BULLET_OBJECT_BOUNDS_SPHERE) {
        // Invoke directly since we have all the data we need
        updatePhysicsWorldWithMesh(uuid, MeshdataPtr());
    }
    else {
        getMesh(msh, uuid,
            std::tr1::bind(&BulletPhysicsService::updatePhysicsWorldWithMesh, this, uuid, _1)
        );
    }
}

void BulletPhysicsService::updatePhysicsWorldWithMesh(const UUID& uuid, MeshdataPtr retrievedMesh) {
    LocationMap::iterator it = mLocations.find(uuid);
    LocationInfo& locinfo = it->second;

    // Spheres can be handled trivially
    if(locinfo.objBBox == BULLET_OBJECT_BOUNDS_SPHERE ||
        !retrievedMesh) {
        locinfo.objShape = new btSphereShape(locinfo.bounds.radius());
        BULLETLOG(detailed, "sphere radius: " << locinfo.bounds.radius());
        addRigidBody(uuid, locinfo);
        return;
    }

    // Other types require processing the retrieved mesh.

    //initialize the world transformation
    Matrix4x4f worldTransformation = Matrix4x4f::identity();

    /***Let's now find the bounding box for the entire object, which is needed for re-scaling purposes.
	* Supposedly the system scales every mesh down to a unit sphere and then scales up by the scale factor
	* from the scene file. We try to emulate this behavior here, but this should really be on the CDN side
	* (we retrieve the precomputed bounding box as well as the mesh) ***/
    BoundingBox3f3f bbox = BoundingBox3f3f::null();
    double mesh_rad = 0;

    // Mesh should be available in retrievedMesh. If it isn't
    Meshdata::GeometryInstanceIterator geoIter = retrievedMesh->getGeometryInstanceIterator();
    uint32 indexInstance; Matrix4x4f transformInstance;
    //loop through the instances, expand the bounding box and find the radius
    while(geoIter.next(&indexInstance, &transformInstance)) {
        GeometryInstance* geoInst = &(retrievedMesh->instances[indexInstance]);
        BoundingBox3f3f inst_bnds;
        double rad = 0;
        geoInst->computeTransformedBounds(retrievedMesh, transformInstance, &inst_bnds, &rad);
        if (bbox == BoundingBox3f3f::null())
            bbox = inst_bnds;
        else
            bbox.mergeIn(inst_bnds);
        mesh_rad = std::max(mesh_rad, rad);
    }
    BULLETLOG(detailed, "bbox: " << bbox);
    Vector3f diff = bbox.max() - bbox.min();

    //objBBox enum defined in header file
    //using if/elseif here to avoid switch/case compiler complaints (initializing variables in a case)
    if(locinfo.objBBox == BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT) {
        double scalingFactor = locinfo.bounds.radius()/mesh_rad;
        BULLETLOG(detailed, "bbox half extents: " << fabs(diff.x/2)*scalingFactor << ", " << fabs(diff.y/2)*scalingFactor << ", " << fabs(diff.z/2)*scalingFactor);
        locinfo.objShape = new btBoxShape(btVector3(fabs((diff.x/2)*scalingFactor), fabs((diff.y/2)*scalingFactor), fabs((diff.z/2)*scalingFactor)));
    }
    //do NOT attempt to collide two btBvhTriangleMeshShapes, it will not work
    else if(locinfo.objBBox == BULLET_OBJECT_BOUNDS_PER_TRIANGLE) {
		//we found the bounding box and radius, so let's scale the mesh down by the radius and up by the scaling factor from the scene file (bnds.radius())
		worldTransformation = worldTransformation * Matrix4x4f::scale((float) locinfo.bounds.radius()/mesh_rad);
		//reset the instance iterator for a second round
		geoIter = retrievedMesh->getGeometryInstanceIterator();
		//we need to pass the triangles to Bullet
		btTriangleMesh * meshToConstruct = new btTriangleMesh(false, false);
		//loop through the instances again, applying the new transformations to vertices and adding them to the Bullet mesh
		while(geoIter.next(&indexInstance, &transformInstance)) {
			transformInstance = transformInstance * worldTransformation;
			GeometryInstance* geoInst = &(retrievedMesh->instances[indexInstance]);

			unsigned int geoIndx = geoInst->geometryIndex;
			SubMeshGeometry* subGeom = &(retrievedMesh->geometry[geoIndx]);
			unsigned int numOfPrimitives = subGeom->primitives.size();
			std::vector<int> gIndices;
			std::vector<Vector3f> gVertices;
			for(unsigned int i = 0; i < numOfPrimitives; i++) {
				//create bullet triangle array from our data structure
				Vector3f transformedVertex;
                                BULLETLOG(detailed, "subgeom indices: ");
				for(unsigned int j=0; j < subGeom->primitives[i].indices.size(); j++) {
					gIndices.push_back((int)(subGeom->primitives[i].indices[j]));
                                        BULLETLOG(detailed, (int)(subGeom->primitives[i].indices[j]) << ", ");
				}
                                BULLETLOG(detailed, "gIndices size: " << (int) gIndices.size());
				for(unsigned int j=0; j < subGeom->positions.size(); j++) {
					//printf("preTransform Vertex: %f, %f, %f\n", subGeom->positions[j].x, subGeom->positions[j].y, subGeom->positions[j].z);
					transformedVertex = transformInstance * subGeom->positions[j];
					//printf("Transformed Vertex: %f, %f, %f\n", transformedVertex.x, transformedVertex.y, transformedVertex.z);
					gVertices.push_back(transformedVertex);
				}
				//TODO: check memleak, check divisible by 3
				/*printf("btTriangleIndexVertexArray:\n");
				printf("argument 1: %d\n", (int) (gIndices.size())/3);
				printf("argument 3: %d\n", (int) 3*sizeof(int));
				printf("argument 4: %d\n", gVertices.size());
				printf("argument 6: %d\n", (int) sizeof(btVector3));
				btTriangleIndexVertexArray* indexVertexArrays = new btTriangleIndexVertexArray((int) gIndices.size()/3, &gIndices[0], (int) 3*sizeof(int), gVertices.size(), (btScalar *) &gVertices[0].x, (int) sizeof(btVector3));*/
			}
			for(unsigned int j=0; j < gIndices.size(); j+=3) {
				//printf("triangle %d: %d, %d, %d\n", j/3, j, j+1, j+2);
				//printf("triangle %d:\n",  j/3);
				//printf("vertex 1: %f, %f, %f\n", gVertices[gIndices[j]].x, gVertices[gIndices[j]].y, gVertices[gIndices[j]].z);
				//printf("vertex 2: %f, %f, %f\n", gVertices[gIndices[j+1]].x, gVertices[gIndices[j+1]].y, gVertices[gIndices[j+1]].z);
				//printf("vertex 3: %f, %f, %f\n\n", gVertices[gIndices[j+2]].x, gVertices[gIndices[j+2]].y, gVertices[gIndices[j+2]].z);
				meshToConstruct->addTriangle(
                                    btVector3( gVertices[gIndices[j]].x, gVertices[gIndices[j]].y, gVertices[gIndices[j]].z ),
                                    btVector3( gVertices[gIndices[j+1]].x, gVertices[gIndices[j+1]].y, gVertices[gIndices[j+1]].z ),
                                    btVector3( gVertices[gIndices[j+2]].x, gVertices[gIndices[j+2]].y, gVertices[gIndices[j+2]].z )
                                );
			}
			Vector3f bMin = bbox.min();
		}
                BULLETLOG(detailed, "total bounds: " << bbox);
                BULLETLOG(detailed, "bounds radius: " << mesh_rad);
                BULLETLOG(detailed, "Num of triangles in mesh: " << meshToConstruct->getNumTriangles());
		//btVector3 aabbMin(-1000,-1000,-1000),aabbMax(1000,1000,1000);
		locinfo.objShape  = new btBvhTriangleMeshShape(meshToConstruct,true);
	}
	//FIXME bug somewhere else? bnds.radius()/mesh_rad should be
	//the correct radius, but it is not...

    addRigidBody(uuid, locinfo);
}

void BulletPhysicsService::addRigidBody(const UUID& uuid, LocationInfo& locinfo) {
    //register the motion state (callbacks) for Bullet
    locinfo.objMotionState = new SirikataMotionState(this, uuid);

    //set a placeholder for the inertial vector
    btVector3 objInertia(0,0,0);
    //calculate the inertia
    locinfo.objShape->calculateLocalInertia(locinfo.mass,objInertia);
    //make a constructionInfo object
    btRigidBody::btRigidBodyConstructionInfo objRigidBodyCI(locinfo.mass, locinfo.objMotionState, locinfo.objShape, objInertia);
    //CREATE: make the rigid body
    locinfo.objRigidBody = new btRigidBody(objRigidBodyCI);
    //locinfo.objRigidBody->setRestitution(0.5);
    //set initial velocity
    Vector3f objVelocity = locinfo.location.velocity();
    locinfo.objRigidBody->setLinearVelocity(btVector3(objVelocity.x, objVelocity.y, objVelocity.z));
    Quaternion objAngVelocity = locinfo.orientation.velocity();
    Vector3f angvel_axis;
    float32 angvel_angle;
    objAngVelocity.toAngleAxis(angvel_angle, angvel_axis);
    Vector3f angvel = angvel_axis.normal() * angvel_angle;
    locinfo.objRigidBody->setAngularVelocity(btVector3(angvel.x, angvel.y, angvel.z));
    //add to the dynamics world
    dynamicsWorld->addRigidBody(locinfo.objRigidBody);
    // And if its dynamic, make sure its in our list of objects to
    // track for sanity checking
    mDynamicPhysicsObjects.insert(uuid);
}

void BulletPhysicsService::removeRigidBody(const UUID& uuid, LocationInfo& locinfo) {
    if (locinfo.objRigidBody) {
        dynamicsWorld->removeRigidBody(locinfo.objRigidBody);

        delete locinfo.objShape;
        locinfo.objShape = NULL;
        delete locinfo.objMotionState;
        locinfo.objMotionState = NULL;
        delete locinfo.objRigidBody;
        locinfo.objRigidBody = NULL;

        UUIDSet::iterator dynamic_obj_it = mDynamicPhysicsObjects.find(uuid);
        if (dynamic_obj_it != mDynamicPhysicsObjects.end())
            mDynamicPhysicsObjects.erase(dynamic_obj_it);
    }
}

void BulletPhysicsService::removeLocalObject(const UUID& uuid) {
    // Remove from mLocations, but save the cached state
    assert( mLocations.find(uuid) != mLocations.end() );
    assert( mLocations[uuid].local == true );
    assert( mLocations[uuid].aggregate == false );

    // FIXME we might want to give a grace period where we generate a replica if one isn't already there,
    // instead of immediately removing all traces of the object.
    // However, this needs to be handled carefully, prefer updates from another server, and expire
    // automatically.

    LocationInfo& locinfo = mLocations[uuid];
    removeRigidBody(uuid, locinfo);

    mLocations.erase(uuid);

    // Remove from the list of local objects
    CONTEXT_SPACETRACE(serverObjectEvent, mContext->id(), mContext->id(), uuid, false, TimedMotionVector3f());
    notifyLocalObjectRemoved(uuid, false);
}

void BulletPhysicsService::updateBulletFromObject(const UUID& uuid, btTransform& worldTrans) {
    Vector3f objPosition = currentPosition(uuid);
    Quaternion objOrient = currentOrientation(uuid);
    worldTrans = btTransform(
        btQuaternion(objOrient.x,objOrient.y,objOrient.z,objOrient.w),
        btVector3(objPosition.x,objPosition.y,objPosition.z)
    );
}

void BulletPhysicsService::updateObjectFromBullet(const UUID& uuid, const btTransform& worldTrans) {
    assert(isFixed(uuid) == false);

    LocationInfo& locinfo = mLocations[uuid];

    btVector3 pos = worldTrans.getOrigin();
    btVector3 vel = locinfo.objRigidBody->getLinearVelocity();
    TimedMotionVector3f newLocation(context()->simTime(), MotionVector3f(Vector3f(pos.x(), pos.y(), pos.z()), Vector3f(vel.x(), vel.y(), vel.z())));
    setLocation(uuid, newLocation);

    btQuaternion rot = worldTrans.getRotation();
    btVector3 angvel = locinfo.objRigidBody->getAngularVelocity();
    Vector3f angvel_siri(angvel.x(), angvel.y(), angvel.z());
    float angvel_angle = angvel_siri.normalizeThis();
    TimedMotionQuaternion newOrientation(
        context()->simTime(),
        MotionQuaternion(
            Quaternion(rot.x(), rot.y(), rot.z(), rot.w()),
            Quaternion(angvel_siri, angvel_angle)
        )
    );
    setOrientation(uuid, newOrientation);

    physicsUpdates.insert(uuid);
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

void BulletPhysicsService::internalTickCallback() {
    for(UUIDSet::iterator id_it = mDynamicPhysicsObjects.begin(); id_it != mDynamicPhysicsObjects.end(); id_it++) {
        LocationInfo& locinfo = mLocations[*id_it];
        assert(locinfo.objRigidBody != NULL);
        capLinearVelocity(locinfo.objRigidBody, 100);
        capAngularVelocity(locinfo.objRigidBody, 4*3.14159);
    }
}

void BulletPhysicsService::addLocalAggregateObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh, const String& phy) {
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
    locinfo.physics = phy;
    locinfo.local = true;
    locinfo.aggregate = true;

    // Add to the list of local objects
    notifyLocalObjectAdded(uuid, true, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid));
    updatePhysicsWorld(uuid);
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
void BulletPhysicsService::updateLocalAggregatePhysics(const UUID& uuid, const String& newval) {
    LocationMap::iterator loc_it = mLocations.find(uuid);
    assert(loc_it != mLocations.end());
    assert(loc_it->second.aggregate == true);
    String oldval = loc_it->second.physics;
    loc_it->second.physics = newval;
    notifyLocalPhysicsUpdated( uuid, true, newval );
    if (oldval != newval) updatePhysicsWorld(uuid);
}

void BulletPhysicsService::addReplicaObject(const Time& t, const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& msh, const String& phy) {
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
            locinfo.physics = phy;
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
        locinfo.physics = phy;
        locinfo.local = false;
        locinfo.aggregate = false;
        mLocations[uuid] = locinfo;

        // We only run this notification when the object actually is new
        CONTEXT_SPACETRACE(serverObjectEvent, 0, mContext->id(), uuid, true, loc); // FIXME add remote server ID
        notifyReplicaObjectAdded(uuid, location(uuid), orientation(uuid), bounds(uuid), mesh(uuid), physics(uuid));
        updatePhysicsWorld(uuid);
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

            if (update.has_physics()) {
                String newphy = update.physics();
                String oldphy = loc_it->second.physics;
                loc_it->second.physics = newphy;
                notifyReplicaPhysicsUpdated( update.object(), newphy );
                if (oldphy != newphy) updatePhysicsWorld(update.object());
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

            if (request.has_physics()) {
                String newphy = request.physics();
                String oldphy = loc_it->second.physics;
                loc_it->second.physics = newphy;
                notifyLocalPhysicsUpdated( source, loc_it->second.aggregate, newphy );
                if (oldphy != newphy) updatePhysicsWorld(source);
            }

        }
        else {
            // Warn about update to non-local object
        }
    }
}



} // namespace Sirikata
