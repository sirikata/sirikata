/*  Sirikata liboh -- Bullet Graphics Plugin
 *  BulletGraphics.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn & Daniel Braxton Miller
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

#include <fstream>
#include <oh/Platform.hpp>
#include <oh/SimulationFactory.hpp>
#include <oh/ProxyObject.hpp>
#include <options/Options.hpp>
#include <transfer/TransferManager.hpp>
#include "btBulletDynamicsCommon.h"
#include "BulletSystem.hpp"

using namespace std;
using std::tr1::placeholders::_1;
static int core_plugin_refcount = 0;

#define DEBUG_OUTPUT(x) x
//#define DEBUG_OUTPUT(x)

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    DEBUG_OUTPUT(cout << "dbm: plugin init" << endl;)
    if (core_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("bulletphysics",
                &BulletSystem::create,
                true);
    core_plugin_refcount++;
    DEBUG_OUTPUT(cout << "dbm: plugin init return" << endl;)
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++core_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(core_plugin_refcount>0);
    return --core_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (core_plugin_refcount>0) {
        core_plugin_refcount--;
        assert(core_plugin_refcount==0);
        if (core_plugin_refcount==0)
            SimulationFactory::getSingleton().unregisterConstructor("bulletphysics",true);
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "bulletphysics";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}

namespace Sirikata {


void bulletObj::meshChanged (const URI &newMesh) {
    DEBUG_OUTPUT(cout << "dbm:    meshlistener: " << newMesh << endl;)
    meshname = newMesh;
}

void bulletObj::setPhysical (const physicalParameters &pp) {
    DEBUG_OUTPUT(cout << "dbm: setPhysical: " << (long)this << " mode=" << pp.mode << " mesh: " << meshname << endl;)
    switch (pp.mode) {
    case Disabled:
        active = false;
        dynamic = false;
        break;
    case Static:
        dynamic = false;
        shape = ShapeMesh;
        break;
    case DynamicBox:
        dynamic = true;
        shape = ShapeBox;
        break;
    case DynamicSphere:
        dynamic = true;
        shape = ShapeSphere;
        break;
    }
    if (!Disabled) {
        positionOrientation po;
        po.p = meshptr->getPosition();
        po.o = meshptr->getOrientation();
        Vector3f size = meshptr->getScale();
        system->addPhysicalObject(this, po, pp.density, pp.friction, pp.bounce, size.x, size.y, size.z);
    }
}

positionOrientation bulletObj::getBulletState() {
    btTransform trans;
    this->bulletBodyPtr->getMotionState()->getWorldTransform(trans);
    return positionOrientation(trans.getOrigin(),trans.getRotation());
}

void bulletObj::setBulletState(positionOrientation po) {
    btTransform trans;
    bulletBodyPtr->getMotionState()->getWorldTransform(trans);
    trans.setOrigin(btVector3(po.p.x, po.p.y, po.p.z));
    trans.setRotation(btQuaternion(po.o.x, po.o.y, po.o.z, po.o.w));
    /// more Bullet mojo: dynamic vs kinematic
    if (dynamic) {
        bulletBodyPtr->proceedToTransform(trans);           /// how to move dynamic objects
    }
    else {
        bulletBodyPtr->getMotionState()->setWorldTransform(trans);   /// how to move 'kinematic' objects (animated)
    }
    bulletBodyPtr->activate(true);      /// wake up, you lazy slob!
}

void bulletObj::setScale (const Vector3f &newScale) {
    if (sizeX == 0)         /// this gets called once before the bullet stuff is ready
        return;
    if (sizeX==newScale.x && sizeY==newScale.y && sizeZ==newScale.z)
        return;
    sizeX = newScale.x;
    sizeY = newScale.y;
    sizeZ = newScale.z;
    float mass;
    btVector3 localInertia(0,0,0);
    buildBulletShape(NULL, 0, mass);        /// null, 0 means re-use original vertices
    if (dynamic) {                          /// inertia meaningless for static objects
        if (!shape==ShapeMesh) {
            colShape->calculateLocalInertia(mass,localInertia);
        }
        else {
            /// note: this code path not tested, as we don't yet support dynamic mesh
            cout << "using bounding box for inertia, Bullet does not calculate for mesh!" << endl;
            localInertia = btVector3(sizeX, sizeY, sizeZ);      /// does this make sense?  it does to me
        }
    }
    bulletBodyPtr->setCollisionShape(colShape);
    bulletBodyPtr->setMassProps(mass, localInertia);
    bulletBodyPtr->setGravity(btVector3(0, -9.8, 0));                              /// otherwise gravity assumes old inertia!
    bulletBodyPtr->activate(true);
    DEBUG_OUTPUT(cout << "dbm: setScale " << newScale << " old X: " << sizeX << " mass: "
                 << mass << " localInertia: " << localInertia.getX() << "," << localInertia.getY() << "," << localInertia.getZ() << endl);
}

void bulletObj::buildBulletShape(const unsigned char* meshdata, int meshbytes, float &mass) {
    /// if meshbytes = 0, reuse vertices & indices (for rescaling)
    if (colShape) delete colShape;
    if (dynamic) {
        if (shape == ShapeSphere) {
            DEBUG_OUTPUT(cout << "dbm: shape=sphere " << endl);
            colShape = new btSphereShape(btScalar(sizeX));
            mass = sizeX*sizeX*sizeX * density * 4.189;                         /// Thanks, Wolfram Alpha!
        }
        else if (shape == ShapeBox) {
            DEBUG_OUTPUT(cout << "dbm: shape=boxen " << endl);
            colShape = new btBoxShape(btVector3(sizeX*.5, sizeY*.5, sizeZ*.5));
            mass = sizeX * sizeY * sizeZ * density;
        }
    }
    else {
        /// create a mesh-based static (not dynamic ie forces, though kinematic, ie movable) object
        /// assuming !dynamic; in future, may support dynamic mesh through gimpact collision
        vector<double> bounds;
        if (btVertices)
            btAlignedFree(btVertices);
        btVertices=NULL;
        unsigned int i,j;

        if (meshbytes) {
            vertices.clear();
            indices.clear();
            parseOgreMesh parser;
            parser.parseData(meshdata, meshbytes, vertices, indices, bounds);
        }
        DEBUG_OUTPUT (cout << "dbm:mesh " << vertices.size() << " vertices:" << endl);
        btVertices=(btScalar*)btAlignedAlloc(vertices.size()/3*sizeof(btScalar)*4,16);
        for (i=0; i<vertices.size()/3; i+=1) {
            DEBUG_OUTPUT ( cout << "dbm:mesh");
            for (j=0; j<3; j+=1) {
                DEBUG_OUTPUT (cout <<" " << vertices[i*3+j]);
            }
            btVertices[i*4]=vertices[i*3]*sizeX;
            btVertices[i*4+1]=vertices[i*3+1]*sizeY;
            btVertices[i*4+2]=vertices[i*3+2]*sizeZ;
            btVertices[i*4+3]=1;
            DEBUG_OUTPUT (cout << endl);
        }
        DEBUG_OUTPUT (cout << endl);
        DEBUG_OUTPUT (cout << "dbm:mesh " << indices.size() << " indices:");
        for (i=0; i<indices.size(); i++) {
            DEBUG_OUTPUT (cout << " " << indices[i]);
        }
        DEBUG_OUTPUT (cout << endl);
        DEBUG_OUTPUT (cout << "dbm:mesh bounds:");
        for (i=0; i<bounds.size(); i++) {
            DEBUG_OUTPUT (cout << " " << bounds[i]);
        }
        DEBUG_OUTPUT (cout << endl);

        if (indexarray) delete indexarray;
        indexarray = new btTriangleIndexVertexArray(
            indices.size()/3,                       // # of triangles (int)
            &(indices[0]),                          // ptr to list of indices (int)
            sizeof(int)*3,                          // index stride, in bytes (typically 3X sizeof(int) = 12
            vertices.size()/3,                      // # of vertices (int)
            btVertices,         // (btScalar*) pointer to vertex list
            sizeof(btScalar)*4);                     // vertex stride, in bytes
        btVector3 aabbMin(-10000,-10000,-10000),aabbMax(10000,10000,10000);
        colShape  = new btBvhTriangleMeshShape(indexarray,false, aabbMin, aabbMax);
        DEBUG_OUTPUT(cout << "dbm: shape=trimesh colShape: " << colShape <<
                     " triangles: " << indices.size()/3 << " verts: " << vertices.size()/3 << endl);

        mass = 0.0;

        /// try to clean up memory usage
        //vertices.clear();      /// inexplicably, I can't do this
    }
}
bulletObj::~bulletObj() {
    if (btVertices!=NULL)
        btAlignedFree(btVertices);
    if (myMotionState!=NULL) delete myMotionState;
    if (indexarray!=NULL) delete indexarray;
    if (colShape!=NULL) delete colShape;

}
void bulletObj::buildBulletBody(const unsigned char* meshdata, int meshbytes) {
    float mass;
    btTransform startTransform;
    btVector3 localInertia(0,0,0);
    btRigidBody* body;

    buildBulletShape(meshdata, meshbytes, mass);

    system->collisionShapes.push_back(colShape);
    DEBUG_OUTPUT(cout << "dbm: mass = " << mass << endl;)
    if (dynamic) {
        colShape->calculateLocalInertia(mass,localInertia);
    }
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(initialPo.p.x, initialPo.p.y, initialPo.p.z));
    startTransform.setRotation(btQuaternion(initialPo.o.x, initialPo.o.y, initialPo.o.z, initialPo.o.w));
    myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,colShape,localInertia);
    body = new btRigidBody(rbInfo);
    body->setFriction(friction);
    body->setRestitution(bounce);
    if (!dynamic) {
        /// voodoo recommendations from the bullet tutorials
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }
    system->dynamicsWorld->addRigidBody(body);
    bulletBodyPtr=body;
    active=true;
}

Task::EventResponse BulletSystem::downloadFinished(Task::EventPtr evbase, bulletObj* bullobj) {
    Transfer::DownloadEventPtr ev = std::tr1::static_pointer_cast<Transfer::DownloadEvent> (evbase);
    DEBUG_OUTPUT (cout << "dbm: downloadFinished: status:" << (int)ev->getStatus()
                  << " success: " << (int)Transfer::TransferManager::SUCCESS
                  << " bullet obj: " << bullobj
                  << " length = " << (int)ev->data().length() << endl);
    if (!ev->getStatus()==Transfer::TransferManager::SUCCESS) {
        cout << "BulletSystem::downloadFinished failed, bullet object will not be built" << endl;
    }
    else {
        Transfer::DenseDataPtr flatData = ev->data().flatten();
        const unsigned char* realData = flatData->data();
        DEBUG_OUTPUT (cout << "dbm downloadFinished: data: " << (char*)&realData[2] << endl);
        bullobj->buildBulletBody(realData, ev->data().length());
    }
    return Task::EventResponse::del();
}

void BulletSystem::addPhysicalObject(bulletObj* obj,
                                     positionOrientation po,
                                     float density, float friction, float bounce,
                                     float sizeX, float sizeY, float sizeZ) {
    /// a bit annoying -- we have to keep all these around in case our mesh isn't available
    /// note that presently these values are not updated during simulation (particularly po)
    obj->density = density;
    obj->friction = friction;
    obj->bounce = bounce;
    obj->sizeX = sizeX;
    obj->sizeY = sizeY;
    obj->sizeZ = sizeZ;
    obj->initialPo = po;
    DEBUG_OUTPUT(cout << "dbm: adding active object: " << obj << " shape: " << (int)obj->shape << endl);
    if (obj->dynamic) {
        /// create the object now
        obj->buildBulletBody(NULL, 0);                /// no mesh data
    }
    else {
        /// set up a mesh download; callback (downloadFinished) calls buildBulletBody and completes object
        transferManager->download(obj->meshptr->getMesh(), std::tr1::bind(&Sirikata::BulletSystem::downloadFinished,
                                  this, _1, obj), Transfer::Range(true));
    }
}

void BulletSystem::removePhysicalObject(bulletObj* obj) {
    /// this is tricky, and not well tested
    /// memory issues:
    /// there are a number of objects created during the instantiation of a bulletObj
    /// if they really need to be kept around, we should keep track of them & delete them
    DEBUG_OUTPUT(cout << "dbm: removing active object: " << obj << endl;)
    for (unsigned int i=0; i<objects.size(); i++) {
        if (objects[i] == obj) {
            if (objects[i]->active) {
                dynamicsWorld->removeRigidBody(obj->bulletBodyPtr);
                delete obj;
            }
            break;
        }
    }
}

bool BulletSystem::tick() {
    static Task::AbsTime starttime = Task::AbsTime::now();
    static Task::AbsTime lasttime = starttime;
    static Task::DeltaTime waittime = Task::DeltaTime::seconds(0.02);
    static int mode = 0;
    Task::AbsTime now = Task::AbsTime::now();
    Task::DeltaTime delta;
    positionOrientation po;

    DEBUG_OUTPUT(cout << "dbm: BulletSystem::tick time: " << (now-starttime).toSeconds() << endl;)
    if (now > lasttime + waittime) {
        delta = now-lasttime;
        if (delta.toSeconds() > 0.05) delta = delta.seconds(0.05);           /// avoid big time intervals, they are trubble
        lasttime = now;
        if ((now-starttime) > 10.0) {
            for (unsigned int i=0; i<objects.size(); i++) {
                if (objects[i]->active) {
                    if (objects[i]->meshptr->getPosition() != objects[i]->getBulletState().p) {
                        /// if object has been moved, reset bullet position accordingly
                        DEBUG_OUTPUT(cout << "    dbm: item, " << i << " moved by user!"
                                     << " meshpos: " << objects[i]->meshptr->getPosition()
                                     << " bulletpos before reset: " << objects[i]->getBulletState().p;)
                        objects[i]->setBulletState(
                            positionOrientation (
                                objects[i]->meshptr->getPosition(),
                                objects[i]->meshptr->getOrientation()
                            ));
                        DEBUG_OUTPUT(cout << "bulletpos after reset: " << objects[i]->getBulletState().p << endl;)
                    }
                }
            }
            //dynamicsWorld->stepSimulation(delta,0);
            dynamicsWorld->stepSimulation(delta,10);
            for (unsigned int i=0; i<objects.size(); i++) {
                if (objects[i]->active) {
                    po = objects[i]->getBulletState();
                    DEBUG_OUTPUT(cout << "    dbm: item, " << i << ", delta, " << delta.toSeconds() << ", newpos, " << po.p
                                 << "obj: " << objects[i] << endl;)
                    objects[i]->meshptr->setPosition(now, po.p, po.o);
                }
            }
        }
    }
    DEBUG_OUTPUT(cout << endl;)
    return 0;
}

bool BulletSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    DEBUG_OUTPUT(cout << "dbm: BulletSystem::initialize options: " << options << endl);
    /// HelloWorld from Bullet/Demos
    OptionValue* tempTferManager = new OptionValue("transfermanager","0", OptionValueType<void*>(),"dummy");
    OptionValue* workQueue = new OptionValue("workqueue","0",OptionValueType<void*>(),"Memory address of the WorkQueue");
    OptionValue* eventManager = new OptionValue("eventmanager","0",OptionValueType<void*>(),"Memory address of the EventManager<Event>");
    InitializeClassOptions("bulletphysics",this, tempTferManager, workQueue, eventManager, NULL);
    OptionSet::getOptions("bulletphysics",this)->parse(options);
    Transfer::TransferManager* tm = (Transfer::TransferManager*)tempTferManager->as<void*>();
    this->transferManager = tm;

    gravity = Vector3d(0, -9.8, 0);
    //groundlevel = 3044.0;
    groundlevel = 4500.0;
    btCollisionShape* groundShape;
    btTransform groundTransform;
    btRigidBody* body;
    btDefaultMotionState* myMotionState;
    btVector3 worldAabbMin(-10000,-10000,-10000);
    btVector3 worldAabbMax(10000,10000,10000);
    int maxProxies = 1024;
    btVector3 localInertia(0,0,0);

    /// set up bullet stuff
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache= new btAxisSweep3(worldAabbMin,worldAabbMax,maxProxies);
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

    /// create ground
    groundShape= new btBoxShape(btVector3(btScalar(1500.),btScalar(1.0),btScalar(1500.)));
    collisionShapes.push_back(groundShape);
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0,groundlevel-1,0));
    groundShape->calculateLocalInertia(0.0f,localInertia);
    myMotionState = new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f,myMotionState,groundShape,localInertia);
    body = new btRigidBody(rbInfo);
    body->setRestitution(0.5);                 /// bouncy for fun & profit
    dynamicsWorld->addRigidBody(body);

    proxyManager->addListener(this);
    DEBUG_OUTPUT(cout << "dbm: BulletSystem::initialized, including test bullet object" << endl);
//    delete tempTferManager;
//    delete workQueue;
//    delete eventManager;
    return true;
}

BulletSystem::BulletSystem() {
    DEBUG_OUTPUT(cout << "dbm: I am the BulletSystem constructor!" << endl;)
}

BulletSystem::~BulletSystem() {
/// this never gets called AFAICS
    DEBUG_OUTPUT(cout << "dbm: BulletSystem destructor" << endl);

    for (int i=dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject( obj );
        delete obj;
    }
    for (int j=0;j<collisionShapes.size();j++) {
        btCollisionShape* shape = collisionShapes[j];
        collisionShapes[j] = 0;
        delete shape;
    }
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
    DEBUG_OUTPUT(cout << "dbm: I am the BulletSystem destructor!" << endl;)
}

void BulletSystem::createProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        DEBUG_OUTPUT(cout << "dbm: createProxy ptr:" << meshptr << " mesh: " << meshptr->getMesh() << endl;)
        objects.push_back(new bulletObj(this));     /// clean up memory!!!
        objects.back()->meshptr = meshptr;
        meshptr->MeshProvider::addListener(objects.back());
    }
}

void BulletSystem::destroyProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    for (unsigned int i=0; i<objects.size(); i++) {
        if (objects[i]->meshptr==meshptr) {
            DEBUG_OUTPUT(cout << "dbm: destroyProxy, object=" << objects[i] << endl);
            removePhysicalObject(objects[i]);
            objects.erase(objects.begin()+i);
            break;
        }
    }
}

}//namespace sirikata
