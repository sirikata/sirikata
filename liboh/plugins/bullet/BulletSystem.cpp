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
    if (isPhysical) {
        system->addPhysicalObject(this);
    }
    else {
        system->removePhysicalObject(this);
    }
}

bulletObj::bulletObj(BulletSystem* sys) {
    cout << "dbm: I am bulletObj constructor! sys: " << sys << endl;
    system = sys;
    isPhysical=false;
    velocity = Vector3d(0, 0, 0);
}

void BulletSystem::addPhysicalObject(bulletObj* obj) {
    cout << "dbm: adding physical object: " << obj << endl;
    physicalObjects.push_back(obj);
}

void BulletSystem::removePhysicalObject(bulletObj* obj) {
    cout << "dbm: removing physical object: " << obj << endl;
    for (unsigned int i=0; i<physicalObjects.size(); i++) {
        if (physicalObjects[i] == obj) {
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
    static double groundlevel = 3044.0;
    Task::AbsTime now = Task::AbsTime::now();
    Task::DeltaTime delta;
    Vector3d oldpos;
    Vector3d newpos;

    cout << "dbm: BulletSystem::tick time: " << (now-starttime).toSeconds() << endl;
    if (now > lasttime + waittime) {
        delta = now-lasttime;
        if (delta.toSeconds() > 0.05) delta = delta.seconds(0.05);           /// avoid big time intervals, they are trubble
        lasttime = now;
        if (((int)(now-starttime) % 15)<5) {
            for (unsigned int i=0; i<physicalObjects.size(); i++) {
                cout << "  dbm: BS:tick moving object: " << physicalObjects[i] << endl;
                oldpos = physicalObjects[i]->meshptr->getPosition();
                physicalObjects[i]->velocity += gravity*delta.toSeconds();
                newpos = oldpos + physicalObjects[i]->velocity;              /// should work, acc. to Newton
                if (newpos.y < groundlevel) {
                    newpos.y = groundlevel;
                    physicalObjects[i]->velocity = Vector3d(0, 0, 0);
                    cout << "    dbm: BS:tick groundlevel reached" << endl;
                }
                physicalObjects[i]->meshptr->setPosition(now, newpos, Quaternion(Vector3f(.0,.0,.0),1.0));
                cout << "    dbm: BS:tick old position: " << oldpos << " new position: " << newpos << endl;
            }
        }
    }
    cout << endl;
    return 0;
}

bool BulletSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    /// HelloWorld from Bullet/Demos
    {
        btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
        btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
        btVector3 worldAabbMin(-10000,-10000,-10000);
        btVector3 worldAabbMax(10000,10000,10000);
        int maxProxies = 1024;
        btAxisSweep3* overlappingPairCache = new btAxisSweep3(worldAabbMin,worldAabbMax,maxProxies);
        btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
        btDiscreteDynamicsWorld* dynamicsWorld = 
            new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0,-10,0));
        btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.),btScalar(50.),btScalar(50.)));
        btAlignedObjectArray<btCollisionShape*> collisionShapes;
        collisionShapes.push_back(groundShape);
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0,-56,0));

        btVector3 localInertia(0,0,0);
        groundShape->calculateLocalInertia(0.0f,localInertia);
        btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f,myMotionState,groundShape,localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);
        dynamicsWorld->addRigidBody(body);

        //btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
        btCollisionShape* colShape = new btSphereShape(btScalar(1.));
        collisionShapes.push_back(colShape);
        btTransform startTransform;
        startTransform.setIdentity();

        localInertia = btVector3(0,0,0);
        colShape->calculateLocalInertia(1.0f,localInertia);
        startTransform.setOrigin(btVector3(2,10,0));
        myMotionState = new btDefaultMotionState(startTransform);
        rbInfo=btRigidBody::btRigidBodyConstructionInfo(1.0f,myMotionState,colShape,localInertia);
        body = new btRigidBody(rbInfo);
        dynamicsWorld->addRigidBody(body);
/// Do some simulation
        
        for (int i=0;i<100;i++)
        {
            dynamicsWorld->stepSimulation(1.f/60.f,10);
            for (int j=dynamicsWorld->getNumCollisionObjects()-1; j>=0 ;j--)
            {
                btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[j];
                btRigidBody* body = btRigidBody::upcast(obj);
                if (body && body->getMotionState())
                {
                    btTransform trans;
                    body->getMotionState()->getWorldTransform(trans);
                    printf("world pos = %f,%f,%f\n",
                           float(trans.getOrigin().getX()),float(trans.getOrigin().getY()),float(trans.getOrigin().getZ()));
                }
            }
        }

/// cleanup
        
        for (int i=dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
        {
            btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState())
            {
                delete body->getMotionState();
            }
            dynamicsWorld->removeCollisionObject( obj );
            delete obj;
        }
        for (int j=0;j<collisionShapes.size();j++)
        {
            btCollisionShape* shape = collisionShapes[j];
            collisionShapes[j] = 0;
            delete shape;
        }
        delete dynamicsWorld;
        delete solver;
        delete overlappingPairCache;
        delete dispatcher;
        delete collisionConfiguration;
    }

    proxyManager->addListener(this);
    cout << "dbm: BulletSystem::initialized, including test bullet object" << endl;
    return true;
}

BulletSystem::BulletSystem() {
    gravity = Vector3d(0, -9.8, 0);
    cout << "dbm: I am the BulletSystem constructor!" << endl;
}

BulletSystem::~BulletSystem() {
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
