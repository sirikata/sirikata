// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "BulletRigidBodyObject.hpp"
#include "BulletPhysicsService.hpp"

#include <sirikata/mesh/Bounds.hpp>

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
    }
}

BulletRigidBodyObject::~BulletRigidBodyObject() {
    removeRigidBody();
}

void BulletRigidBodyObject::load(MeshdataPtr retrievedMesh) {
    const LocationInfo& locinfo = mParent->info(mID);
    // Spheres can be handled trivially
    if(mBBox == BULLET_OBJECT_BOUNDS_SPHERE || !retrievedMesh) {
        mObjShape = new btSphereShape(locinfo.bounds.radius());
        BULLETLOG(detailed, "sphere radius: " << locinfo.bounds.radius());
        addRigidBody(locinfo);
        return;
    }

    // Other types require processing the retrieved mesh.

    /***Let's now find the bounding box for the entire object, which is needed for re-scaling purposes.
	* Supposedly the system scales every mesh down to a unit sphere and then scales up by the scale factor
	* from the scene file. We try to emulate this behavior here, but this should really be on the CDN side
	* (we retrieve the precomputed bounding box as well as the mesh) ***/
    BoundingBox3f3f bbox;
    double mesh_rad;
    ComputeBounds(retrievedMesh, &bbox, &mesh_rad);

    BULLETLOG(detailed, "bbox: " << bbox);
    Vector3f diff = bbox.max() - bbox.min();

    //objBBox enum defined in header file
    //using if/elseif here to avoid switch/case compiler complaints (initializing variables in a case)
    if(mBBox == BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT) {
        double scalingFactor = locinfo.bounds.radius()/mesh_rad;
        BULLETLOG(detailed, "bbox half extents: " << fabs(diff.x/2)*scalingFactor << ", " << fabs(diff.y/2)*scalingFactor << ", " << fabs(diff.z/2)*scalingFactor);
        mObjShape = new btBoxShape(btVector3(fabs((diff.x/2)*scalingFactor), fabs((diff.y/2)*scalingFactor), fabs((diff.z/2)*scalingFactor)));
    }
    //do NOT attempt to collide two btBvhTriangleMeshShapes, it will not work
    else if(mBBox == BULLET_OBJECT_BOUNDS_PER_TRIANGLE) {
        //we found the bounding box and radius, so let's scale the mesh down by the radius and up by the scaling factor from the scene file (bnds.radius())
        //initialize the world transformation
        // The raw mesh data is scaled down to unit size, see below
        // for scaling up to the requested size.
        Matrix4x4f scale_to_unit = Matrix4x4f::scale(1.f/mesh_rad);
		//reset the instance iterator for a second round
                Meshdata::GeometryInstanceIterator geoIter = retrievedMesh->getGeometryInstanceIterator();
		//we need to pass the triangles to Bullet
		btTriangleMesh * meshToConstruct = new btTriangleMesh(false, false);
		//loop through the instances again, applying the new
		//transformations to vertices and adding them to the Bullet mesh
                uint32 indexInstance;
                Matrix4x4f transformInstance;
		while(geoIter.next(&indexInstance, &transformInstance)) {
                    // Note: Scale to unit *after* transforming the
                    // instanced geometry to its location --
                    // scale_to_unit is applied to the mesh as a whole!
			transformInstance = scale_to_unit * transformInstance;
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
		mObjShape  = new btBvhTriangleMeshShape(meshToConstruct,true);
                // Apply additional scaling factor to get from unit
                // scale up to requested scale.
                float32 rad_scale = locinfo.bounds.radius();
                mObjShape->setLocalScaling(btVector3(rad_scale, rad_scale, rad_scale));
	}
	//FIXME bug somewhere else? bnds.radius()/mesh_rad should be
	//the correct radius, but it is not...

    addRigidBody(locinfo);
}

void BulletRigidBodyObject::addRigidBody(const LocationInfo& locinfo) {
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
    Vector3f objVelocity = locinfo.location.velocity();
    mObjRigidBody->setLinearVelocity(btVector3(objVelocity.x, objVelocity.y, objVelocity.z));
    Quaternion objAngVelocity = locinfo.orientation.velocity();
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
    mParent->addDynamicObject(mID);
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

        mParent->removeDynamicObject(mID);
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

void BulletRigidBodyObject::internalTick() {
    assert(mObjRigidBody != NULL);
    capLinearVelocity(mObjRigidBody, 100);
    capAngularVelocity(mObjRigidBody, 4*3.14159);
}

void BulletRigidBodyObject::deactivationTick() {
    if (mObjRigidBody != NULL && !mObjRigidBody->isActive())
        mParent->updateObjectFromDeactivation(mID);
}

} // namespace Sirikata
