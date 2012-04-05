// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "BulletObject.hpp"
#include "BulletPhysicsService.hpp"

#include "btBulletDynamicsCommon.h"

#include <sirikata/mesh/Bounds.hpp>

namespace Sirikata {

// Kinda sucks that we need this to be outside the implementations of
// BulletObject, but we essentially need to specify a matrix of all possible
// interactions
void BulletObject::getCollisionMask(bulletObjTreatment treatment, bulletObjCollisionMaskGroup* mygroup, bulletObjCollisionMaskGroup* collide_with) {
    switch(treatment) {
      case BULLET_OBJECT_TREATMENT_IGNORE:
        // We shouldn't be trying to add this to a sim
        assert(treatment != BULLET_OBJECT_TREATMENT_IGNORE);
        break;
      case BULLET_OBJECT_TREATMENT_STATIC:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_STATIC;
        // Static collides with everything
        *collide_with = (bulletObjCollisionMaskGroup)(BULLET_OBJECT_COLLISION_GROUP_STATIC | BULLET_OBJECT_COLLISION_GROUP_DYNAMIC | BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED);
        break;
      case BULLET_OBJECT_TREATMENT_DYNAMIC:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_DYNAMIC;
        *collide_with = (bulletObjCollisionMaskGroup)(BULLET_OBJECT_COLLISION_GROUP_STATIC | BULLET_OBJECT_COLLISION_GROUP_DYNAMIC | BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED);
        break;
      case BULLET_OBJECT_TREATMENT_LINEAR_DYNAMIC:
      case BULLET_OBJECT_TREATMENT_VERTICAL_DYNAMIC:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED;
        // Only collide with static objects because the constrained
        // movement can become problematic otherwise, e.g. the
        // vertical only movement can result in interpenetrating
        // objects which can't resolve their collision normally and
        // the energy ends up in the vertically moving object,
        // throwing it up in the air
        *collide_with = BULLET_OBJECT_COLLISION_GROUP_STATIC;
        break;
      case BULLET_OBJECT_TREATMENT_CHARACTER:
        *mygroup = BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED;
        // TODO(ewencp) Is this the right set of things? Does this cause any
        // weird interactions given the constraints of the character?
        *collide_with = (bulletObjCollisionMaskGroup)(BULLET_OBJECT_COLLISION_GROUP_STATIC | BULLET_OBJECT_COLLISION_GROUP_DYNAMIC | BULLET_OBJECT_COLLISION_GROUP_CONSTRAINED);
        break;
    };
}


btCollisionShape* BulletObject::computeCollisionShape(const UUID& id, bulletObjBBox shape_type, Mesh::MeshdataPtr retrievedMesh) {
    const LocationInfo& locinfo = mParent->info(id);

    // Spheres can be handled trivially
    if(shape_type == BULLET_OBJECT_BOUNDS_SPHERE || !retrievedMesh) {
        BULLETLOG(detailed, "sphere radius: " << locinfo.props.bounds().radius());
        btCollisionShape* shape = new btSphereShape(locinfo.props.bounds().radius());
        return shape;
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
    if(shape_type == BULLET_OBJECT_BOUNDS_ENTIRE_OBJECT) {
        double scalingFactor = locinfo.props.bounds().radius()/mesh_rad;
        BULLETLOG(detailed, "bbox half extents: " << fabs(diff.x/2)*scalingFactor << ", " << fabs(diff.y/2)*scalingFactor << ", " << fabs(diff.z/2)*scalingFactor);
        btCollisionShape* shape = new btBoxShape(btVector3(fabs((diff.x/2)*scalingFactor), fabs((diff.y/2)*scalingFactor), fabs((diff.z/2)*scalingFactor)));
        return shape;
    }

    //do NOT attempt to collide two btBvhTriangleMeshShapes, it will not work
    //else if(shape_type == BULLET_OBJECT_BOUNDS_PER_TRIANGLE) {
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
        // Note the condition on the loop. Sometimes we get lists with weird
        // setups, e.g. only 2 indices, so we need to make sure all 3 indices
        // we'll use are in range.
        for(unsigned int j=0; j+2 < gIndices.size(); j+=3) {
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
    }
    BULLETLOG(detailed, "total bounds: " << bbox);
    BULLETLOG(detailed, "bounds radius: " << mesh_rad);
    BULLETLOG(detailed, "Num of triangles in mesh: " << meshToConstruct->getNumTriangles());
    //btVector3 aabbMin(-1000,-1000,-1000),aabbMax(1000,1000,1000);
    btCollisionShape* shape = new btBvhTriangleMeshShape(meshToConstruct,true);
    // Apply additional scaling factor to get from unit
    // scale up to requested scale.
    float32 rad_scale = locinfo.props.bounds().radius();
    shape->setLocalScaling(btVector3(rad_scale, rad_scale, rad_scale));

    //FIXME bug somewhere else? bnds.radius()/mesh_rad should be
    //the correct radius, but it is not...

    return shape;
}

} // namespace Sirikata
