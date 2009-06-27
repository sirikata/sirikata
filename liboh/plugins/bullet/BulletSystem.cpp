/*  Sirikata liboh -- Bullet Graphics Plugin
 *  BulletGraphics.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

//#include <iostream>
#include <oh/Platform.hpp>
#include "BulletSystem.hpp"
#include <oh/SimulationFactory.hpp>
#include <oh/ProxyObject.hpp>
#include "btBulletDynamicsCommon.h"

using namespace std;
static int core_plugin_refcount = 0;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    cout << "dbm: plugin init" << endl;
    if (core_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("bulletphysics",
                &BulletSystem::create,
//                NULL,
                true);
    core_plugin_refcount++;
    cout << "dbm: plugin init return" << endl;
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
    cout << "dbm:    meshlistener: " << newMesh << endl;
    meshname = newMesh;
}

void bulletObj::setScale (const Vector3f &newScale) {
}

void bulletObj::setPhysical (const bool flag) {
    cout << "dbm: setPhysical: " << flag << endl;
    isPhysical=flag;
    Vector3d pos = meshptr->getPosition();
    Vector3f size = meshptr->getScale();
    if (isPhysical) {
        bulletBodyPtr = system->addPhysicalObject(this, pos.x, pos.y, pos.z, size.x, size.y, size.z);
    }
    else {
        system->removePhysicalObject(this);
        bulletBodyPtr=NULL;
    }
}

posquat bulletObj::getBulletState() {
    if (this->bulletBodyPtr && this->bulletBodyPtr->getMotionState()) {
        btTransform trans;
        this->bulletBodyPtr->getMotionState()->getWorldTransform(trans);
        posquat pq;
        btVector3 p = trans.getOrigin();
        btQuaternion q = trans.getRotation();
        pq.p = Vector3d(p.getX(), p.getY(), p.getZ());
        pq.q = Quaternion(q.getX(), q.getY(), q.getZ(), q.getW(), Quaternion::XYZW());
        return pq;
    }
    else {
        printf("dbm: error -- this is not a bullet object with a motionstate!\n");
        return posquat();
    }
}

void bulletObj::setBulletState(Vector3d pos, Quaternion q) {
    /* /// this fails to work -- next stepSim restores old position
    btTransform trans;
    this->bulletBodyPtr->getMotionState()->getWorldTransform(trans);
    trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
    this->bulletBodyPtr->getMotionState()->setWorldTransform(trans);
    */
    btTransform trans;
    bulletBodyPtr->getMotionState()->getWorldTransform(trans);
    trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
    trans.setRotation(btQuaternion(q.x, q.y, q.z, q.w));
    bulletBodyPtr->proceedToTransform(trans);
    bulletBodyPtr->activate(true);      /// wake up, you lazy slob!
}

bulletObj::bulletObj(BulletSystem* sys) {
    cout << "dbm: I am bulletObj constructor! sys: " << sys << endl;
    system = sys;
    isPhysical=false;
    velocity = Vector3d(0, 0, 0);
}

btRigidBody* BulletSystem::addPhysicalObject(bulletObj* obj,
        double posX, double posY, double posZ,
        float sizeX, float sizeY, float sizeZ) {
    btCollisionShape* colShape;
    btTransform startTransform;
    btVector3 localInertia(0,0,0);
    btDefaultMotionState* myMotionState;
    btRigidBody* body;

    cout << "dbm: adding physical object: " << obj << endl;
    /// complete hack for demo:
    if (posY > 3055.0) {
        cout << "dbm: shape=sphere " << posY << endl;
        colShape = new btSphereShape(btScalar(sizeX*100));
    }
    else {
        cout << "dbm: shape=boxen " << posY << endl;
        colShape = new btBoxShape(btVector3(sizeX*.5, sizeY*.5, sizeZ*.5));
    }
    collisionShapes.push_back(colShape);
    localInertia = btVector3(0,0,0);
    colShape->calculateLocalInertia(1.0f,localInertia);
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(posX,posY,posZ));
    myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(1.0f,myMotionState,colShape,localInertia);
    body = new btRigidBody(rbInfo);
    body->setRestitution(0.5);
    dynamicsWorld->addRigidBody(body);

    physicalObjects.push_back(obj);
    return body;
}

void BulletSystem::removePhysicalObject(bulletObj* obj) {
    /// need to clean up bullet stuff
    cout << "dbm: removing physical object: " << obj << endl;
    for (unsigned int i=0; i<physicalObjects.size(); i++) {
        if (physicalObjects[i] == obj) {
            dynamicsWorld->removeRigidBody(obj->bulletBodyPtr);
            physicalObjects.erase(physicalObjects.begin()+i);
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
    posquat pq;

    cout << "dbm: BulletSystem::tick time: " << (now-starttime).toSeconds() << endl;
    if (now > lasttime + waittime) {
        delta = now-lasttime;
        if (delta.toSeconds() > 0.05) delta = delta.seconds(0.05);           /// avoid big time intervals, they are trubble
        lasttime = now;
        //if (((int)(now-starttime) % 15)<5) {
        if ((now-starttime) > 15.0) {
            for (unsigned int i=0; i<physicalObjects.size(); i++) {
                if (physicalObjects[i]->meshptr->getPosition() != physicalObjects[i]->getBulletState().p) {
                    /// if object has been moved, reset bullet position accordingly
                    cout << "    dbm: item, " << i << " moved by user!"
                    << " meshpos: " << physicalObjects[i]->meshptr->getPosition()
                    << " bulletpos before reset: " << physicalObjects[i]->getBulletState().p;
                    physicalObjects[i]->setBulletState(
                        physicalObjects[i]->meshptr->getPosition(),
                        physicalObjects[i]->meshptr->getOrientation());
                    cout << "bulletpos after reset: " << physicalObjects[i]->getBulletState().p
                    << endl;
                }
            }
            dynamicsWorld->stepSimulation(delta,0);
            for (unsigned int i=0; i<physicalObjects.size(); i++) {
                pq = physicalObjects[i]->getBulletState();
                cout << "    dbm: item, " << i << ", delta, " << delta.toSeconds() << ", newpos, " << pq.p << endl;
                physicalObjects[i]->meshptr->setPosition(now, pq.p, pq.q);
            }
        }
    }
    cout << endl;
    return 0;
}

bool BulletSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    /// HelloWorld from Bullet/Demos
    gravity = Vector3d(0, -9.8, 0);
    groundlevel = 3044.0;
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
    cout << "dbm: BulletSystem::initialized, including test bullet object" << endl;
    return true;
}

BulletSystem::BulletSystem() {
    cout << "dbm: I am the BulletSystem constructor!" << endl;
}

BulletSystem::~BulletSystem() {
/// cleanup

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
    cout << "dbm: I am the BulletSystem destructor!" << endl;
}

void BulletSystem::createProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        cout << "dbm: createProxy ptr:" << meshptr << " mesh: " << meshptr->getMesh() << endl;
        objects.push_back(new bulletObj(this));     /// clean up memory!!!
        objects.back()->meshptr = meshptr;
        meshptr->MeshProvider::addListener(objects.back());
    }
}

void BulletSystem::destroyProxy(ProxyObjectPtr p) {
}

}//namespace sirikata
