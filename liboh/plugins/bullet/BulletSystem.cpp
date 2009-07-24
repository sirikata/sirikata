/*  Sirikata liboh -- Bullet Graphics Plugin
 *  BulletSystem.cpp
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
#include <oh/BulletSystem.hpp>

using namespace std;
using std::tr1::placeholders::_1;
static int core_plugin_refcount = 0;

//#define DEBUG_OUTPUT(x) x
#define DEBUG_OUTPUT(x)

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
    mMeshname = newMesh;
}

void bulletObj::setPhysical (const physicalParameters &pp) {
    DEBUG_OUTPUT(cout << "dbm: setPhysical: " << this << " mode=" << pp.mode << " mesh: " << mMeshname << endl);
    mName = pp.name;
    colMask = pp.colMask;
    colMsg = pp.colMsg;
    switch (pp.mode) {
    case Disabled:
        DEBUG_OUTPUT(cout << "  dbm: debug setPhysical: Disabled" << endl);
        mActive = false;
        mDynamic = false;
        break;
    case Static:
        mDynamic = false;
        mShape = ShapeMesh;
        break;
    case DynamicBox:
        mDynamic = true;
        mShape = ShapeBox;
        break;
    case DynamicSphere:
        mDynamic = true;
        mShape = ShapeSphere;
        break;
    }
    if (!(pp.mode==Disabled)) {
        DEBUG_OUTPUT(cout << "  dbm: debug setPhysical: adding to bullet" << endl);
        positionOrientation po;
        po.p = mMeshptr->getPosition();
        po.o = mMeshptr->getOrientation();
        Vector3f size = mMeshptr->getScale();
        system->addPhysicalObject(this, po, pp.density, pp.friction, pp.bounce, size.x, size.y, size.z);
    }
}

positionOrientation bulletObj::getBulletState() {
    btTransform trans;
    this->mBulletBodyPtr->getMotionState()->getWorldTransform(trans);
    return positionOrientation(trans.getOrigin(),trans.getRotation());
}

void bulletObj::setBulletState(positionOrientation po) {
    btTransform trans;
    mBulletBodyPtr->getMotionState()->getWorldTransform(trans);
    trans.setOrigin(btVector3(po.p.x, po.p.y, po.p.z));
    trans.setRotation(btQuaternion(po.o.x, po.o.y, po.o.z, po.o.w));
    /// more Bullet mojo: dynamic vs kinematic
    if (mDynamic) {
        mBulletBodyPtr->proceedToTransform(trans);           /// how to move dynamic objects
    }
    else {
        mBulletBodyPtr->getMotionState()->setWorldTransform(trans);   /// how to move 'kinematic' objects (animated)
    }
    mBulletBodyPtr->activate(true);      /// wake up, you lazy slob!
}

void bulletObj::setScale (const Vector3f &newScale) {
    if (mSizeX == 0)         /// this gets called once before the bullet stuff is ready
        return;
    if (mSizeX==newScale.x && mSizeY==newScale.y && mSizeZ==newScale.z)
        return;
    mSizeX = newScale.x;
    mSizeY = newScale.y;
    mSizeZ = newScale.z;
    float mass;
    btVector3 localInertia(0,0,0);
    buildBulletShape(NULL, 0, mass);        /// null, 0 means re-use original vertices
    if (mDynamic) {                          /// inertia meaningless for static objects
        if (!mShape==ShapeMesh) {
            mColShape->calculateLocalInertia(mass,localInertia);
        }
        else {
            /// note: this code path not tested, as we don't yet support dynamic mesh
            cout << "using bounding box for inertia, Bullet does not calculate for mesh!" << endl;
            localInertia = btVector3(mSizeX, mSizeY, mSizeZ);      /// does this make sense?  it does to me
        }
    }
    mBulletBodyPtr->setCollisionShape(mColShape);
    mBulletBodyPtr->setMassProps(mass, localInertia);
    mBulletBodyPtr->setGravity(btVector3(0, -9.8, 0));                              /// otherwise gravity assumes old inertia!
    mBulletBodyPtr->activate(true);
    DEBUG_OUTPUT(cout << "dbm: setScale " << newScale << " old X: " << mSizeX << " mass: "
                 << mass << " localInertia: " << localInertia.getX() << "," << localInertia.getY() << "," << localInertia.getZ() << endl);
}

void bulletObj::buildBulletShape(const unsigned char* meshdata, int meshbytes, float &mass) {
    /// if meshbytes = 0, reuse vertices & indices (for rescaling)
    if (mColShape) delete mColShape;
    if (mDynamic) {
        if (mShape == ShapeSphere) {
            DEBUG_OUTPUT(cout << "dbm: shape=sphere " << endl);
            mColShape = new btSphereShape(btScalar(mSizeX));
            mass = mSizeX*mSizeX*mSizeX * mDensity * 4.189;                         /// Thanks, Wolfram Alpha!
        }
        else if (mShape == ShapeBox) {
            DEBUG_OUTPUT(cout << "dbm: shape=boxen " << endl);
            mColShape = new btBoxShape(btVector3(mSizeX*.5, mSizeY*.5, mSizeZ*.5));
            mass = mSizeX * mSizeY * mSizeZ * mDensity;
        }
    }
    else {
        /// create a mesh-based static (not dynamic ie forces, though kinematic, ie movable) object
        /// assuming !dynamic; in future, may support dynamic mesh through gimpact collision
        vector<double> bounds;
        if (mBtVertices)
            btAlignedFree(mBtVertices);
        mBtVertices=NULL;
        unsigned int i,j;

        if (meshbytes) {
            mVertices.clear();
            mIndices.clear();
            parseOgreMesh parser;
            parser.parseData(meshdata, meshbytes, mVertices, mIndices, bounds);
        }
        DEBUG_OUTPUT (cout << "dbm:mesh " << mVertices.size() << " vertices:" << endl);
        mBtVertices=(btScalar*)btAlignedAlloc(mVertices.size()/3*sizeof(btScalar)*4,16);
        for (i=0; i<mVertices.size()/3; i+=1) {
            DEBUG_OUTPUT ( cout << "dbm:mesh");
            for (j=0; j<3; j+=1) {
                DEBUG_OUTPUT (cout <<" " << mVertices[i*3+j]);
            }
            mBtVertices[i*4]=mVertices[i*3]*mSizeX;
            mBtVertices[i*4+1]=mVertices[i*3+1]*mSizeY;
            mBtVertices[i*4+2]=mVertices[i*3+2]*mSizeZ;
            mBtVertices[i*4+3]=1;
            DEBUG_OUTPUT (cout << endl);
        }
        DEBUG_OUTPUT (cout << endl);
        DEBUG_OUTPUT (cout << "dbm:mesh " << mIndices.size() << " indices:");
        for (i=0; i<mIndices.size(); i++) {
            DEBUG_OUTPUT (cout << " " << mIndices[i]);
        }
        DEBUG_OUTPUT (cout << endl);
        DEBUG_OUTPUT (cout << "dbm:mesh bounds:");
        for (i=0; i<bounds.size(); i++) {
            DEBUG_OUTPUT (cout << " " << bounds[i]);
        }
        DEBUG_OUTPUT (cout << endl);

        if (mIndexArray) delete mIndexArray;
        mIndexArray = new btTriangleIndexVertexArray(
            mIndices.size()/3,                       // # of triangles (int)
            &(mIndices[0]),                          // ptr to list of indices (int)
            sizeof(int)*3,                          // index stride, in bytes (typically 3X sizeof(int) = 12
            mVertices.size()/3,                      // # of vertices (int)
            mBtVertices,         // (btScalar*) pointer to vertex list
            sizeof(btScalar)*4);                     // vertex stride, in bytes
        btVector3 aabbMin(-10000,-10000,-10000),aabbMax(10000,10000,10000);
        mColShape  = new btBvhTriangleMeshShape(mIndexArray,false, aabbMin, aabbMax);
        DEBUG_OUTPUT(cout << "dbm: shape=trimesh mColShape: " << mColShape <<
                     " triangles: " << mIndices.size()/3 << " verts: " << mVertices.size()/3 << endl);
        mass = 0.0;
    }
}
bulletObj::~bulletObj() {
    DEBUG_OUTPUT(cout << "dbm: bulletObj destructor " << this << endl);
    if (mBtVertices!=NULL)
        btAlignedFree(mBtVertices);
    if (mMotionState!=NULL) delete mMotionState;
    if (mIndexArray!=NULL) delete mIndexArray;
    if (mColShape!=NULL) delete mColShape;
    if (mBulletBodyPtr!=NULL) delete mBulletBodyPtr;
}

void bulletObj::buildBulletBody(const unsigned char* meshdata, int meshbytes) {
    float mass;
    btTransform startTransform;
    btVector3 localInertia(0,0,0);
    btRigidBody* body;

    buildBulletShape(meshdata, meshbytes, mass);

    DEBUG_OUTPUT(cout << "dbm: mass = " << mass << endl;)
    if (mDynamic) {
        mColShape->calculateLocalInertia(mass,localInertia);
    }
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(mInitialPo.p.x, mInitialPo.p.y, mInitialPo.p.z));
    startTransform.setRotation(btQuaternion(mInitialPo.o.x, mInitialPo.o.y, mInitialPo.o.z, mInitialPo.o.w));
    mMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,mMotionState,mColShape,localInertia);
    body = new btRigidBody(rbInfo);
    body->setFriction(mFriction);
    body->setRestitution(mBounce);
    if (!mDynamic) {
        /// voodoo recommendations from the bullet tutorials
        body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
    }
    system->dynamicsWorld->addRigidBody(body);
    mBulletBodyPtr=body;
    mActive=true;
    system->bt2siri[body]=this;
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
                                     float mSizeX, float mSizeY, float mSizeZ) {
    /// a bit annoying -- we have to keep all these around in case our mesh isn't available
    /// note that presently these values are not updated during simulation (particularly po)
    obj->mDensity = density;
    obj->mFriction = friction;
    obj->mBounce = bounce;
    obj->mSizeX = mSizeX;
    obj->mSizeY = mSizeY;
    obj->mSizeZ = mSizeZ;
    obj->mInitialPo = po;
    DEBUG_OUTPUT(cout << "dbm: adding active object: " << obj << " shape: " << (int)obj->mShape << endl);
    if (obj->mDynamic) {
        /// create the object now
        obj->buildBulletBody(NULL, 0);                /// no mesh data
    }
    else {
        /// set up a mesh download; callback (downloadFinished) calls buildBulletBody and completes object
        transferManager->download(obj->mMeshptr->getMesh(), std::tr1::bind(&Sirikata::BulletSystem::downloadFinished,
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
            if (objects[i]->mActive) {
                dynamicsWorld->removeRigidBody(obj->mBulletBodyPtr);
            }
            delete obj;
            break;
        }
    }
}

bool BulletSystem::tick() {
    static Task::AbsTime lasttime = mStartTime;
    static Task::DeltaTime waittime = Task::DeltaTime::seconds(0.02);
    static int mode = 0;
    Task::AbsTime now = Task::AbsTime::now();
    Task::DeltaTime delta;
    positionOrientation po;

    DEBUG_OUTPUT(cout << "dbm: BulletSystem::tick time: " << (now-mStartTime).toSeconds() << endl;)
    if (now > lasttime + waittime) {
        delta = now-lasttime;
        if (delta.toSeconds() > 0.05) delta = delta.seconds(0.05);           /// avoid big time intervals, they are trubble
        lasttime = now;
        if ((now-mStartTime) > 10.0) {
            for (unsigned int i=0; i<objects.size(); i++) {
                if (objects[i]->mActive) {
                    if (objects[i]->mMeshptr->getPosition() != objects[i]->getBulletState().p ||
                            objects[i]->mMeshptr->getOrientation() != objects[i]->getBulletState().o) {
                        /// if object has been moved, reset bullet position accordingly
                        DEBUG_OUTPUT(cout << "    dbm: object, " << objects[i]->mName << " moved by user!"
                                     << " meshpos: " << objects[i]->mMeshptr->getPosition()
                                     << " bulletpos before reset: " << objects[i]->getBulletState().p;)
                        objects[i]->setBulletState(
                            positionOrientation (
                                objects[i]->mMeshptr->getPosition(),
                                objects[i]->mMeshptr->getOrientation()
                            ));
                        DEBUG_OUTPUT(cout << "bulletpos after reset: " << objects[i]->getBulletState().p << endl;)
                    }
                }
            }
            dynamicsWorld->stepSimulation(delta,10);
            for (unsigned int i=0; i<objects.size(); i++) {
                if (objects[i]->mActive) {
                    po = objects[i]->getBulletState();
                    DEBUG_OUTPUT(cout << "    dbm: object, " << objects[i]->mName << ", delta, "
                                 << delta.toSeconds() << ", newpos, " << po.p << "obj: " << objects[i] << endl;)
                    Location loc (objects[i]->mMeshptr->globalLocation(now));
                    loc.setPosition(po.p);
                    loc.setOrientation(po.o);
                    objects[i]->mMeshptr->setLocation(now, loc);
                }
            }

            /// collision messages
            for (map<set<bulletObj*>, int>::iterator i=dispatcher->collisionPairs.begin();
                    i != dispatcher->collisionPairs.end(); ++i) {
                set<bulletObj*>::const_iterator j=i->first.begin() ;
                bulletObj* b0=*j++;
                bulletObj* b1=*j;
                if (i->second==1) {             /// recently colliding; send msg & change mode
                    if (b1->colMsg & b0->colMask) {
                        cout << "   begin collision msg: " << b0->mName << " --> " << b1->mName
                                << " time: " << (Task::AbsTime::now()-mStartTime).toSeconds() << endl;
                    }
                    if (b0->colMsg & b1->colMask) {
                        cout << "   begin collision msg: " << b1->mName << " --> " << b0->mName
                                << " time: " << (Task::AbsTime::now()-mStartTime).toSeconds() << endl;
                    }
                    dispatcher->collisionPairs[i->first]=2;
                }
                else if (i->second==2) {        /// didn't get flagged again; collision now over
                    if (b1->colMsg & b0->colMask) {
                        cout << "     end collision msg: " << b0->mName << " --> " << b1->mName
                                << " time: " << (Task::AbsTime::now()-mStartTime).toSeconds() << endl;
                    }
                    if (b0->colMsg & b1->colMask) {
                        cout << "     end collision msg: " << b1->mName << " --> " << b0->mName
                                << " time: " << (Task::AbsTime::now()-mStartTime).toSeconds() << endl;
                    }
                    map<set<bulletObj*>, int>::iterator temp = i++;   /// (sigh) rage against the machine
                    dispatcher->collisionPairs.erase(temp);
                    if (i==dispatcher->collisionPairs.end()) break;
                }
                else if (i->second==3) {        /// re-flagged, so still colliding. clear flag
                    dispatcher->collisionPairs[i->first]=2;
                }
            }
        }
    }
    DEBUG_OUTPUT(cout << endl;)
    return true;
}

void customNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher,
                        const btDispatcherInfo& dispatchInfo) {
    /// we gots to do the stuff Bullet does ourselves to capture the collisionPair

    btCollisionObject* colObj0 = (btCollisionObject*)collisionPair.m_pProxy0->m_clientObject;
    btCollisionObject* colObj1 = (btCollisionObject*)collisionPair.m_pProxy1->m_clientObject;

    if (dispatcher.needsCollision(colObj0,colObj1)) {
        //dispatcher will keep algorithms persistent in the collision pair
        if (!collisionPair.m_algorithm) {
            collisionPair.m_algorithm = dispatcher.findAlgorithm(colObj0,colObj1);
        }

        if (collisionPair.m_algorithm) {
            btManifoldResult contactPointResult(colObj0,colObj1);

            if (dispatchInfo.m_dispatchFunc ==      btDispatcherInfo::DISPATCH_DISCRETE) {
                //discrete collision detection query
                collisionPair.m_algorithm->processCollision(colObj0,colObj1,dispatchInfo,&contactPointResult);
            }
            else {
                //continuous collision detection query, time of impact (toi)
                btScalar toi = collisionPair.m_algorithm->calculateTimeOfImpact(colObj0,colObj1,dispatchInfo,&contactPointResult);
                if (dispatchInfo.m_timeOfImpact > toi)
                    dispatchInfo.m_timeOfImpact = toi;
            }
            int contacts = contactPointResult.getPersistentManifold()->getNumContacts();
            if (contacts) {
                bulletObj* siri0 = ((customDispatch*)(&dispatcher))->bt2siri[0][colObj0];
                bulletObj* siri1 = ((customDispatch*)(&dispatcher))->bt2siri[0][colObj1];
                if (siri0 && siri1) {
                    if (siri0->colMask & siri1->colMask) {
                        set<bulletObj*> temp;
                        temp.insert(siri0);
                        temp.insert(siri1);
                        ((customDispatch*)(&dispatcher))->collisionPairs[temp] |= 1;
                    }
                }
            }
        }
    }
}

bool BulletSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    DEBUG_OUTPUT(cout << "dbm: BulletSystem::initialize options: " << options << endl);
    /// HelloWorld from Bullet/Demos
    mTempTferManager = new OptionValue("transfermanager","0", OptionValueType<void*>(),"dummy");
    mWorkQueue = new OptionValue("workqueue","0",OptionValueType<void*>(),"Memory address of the WorkQueue");
    mEventManager = new OptionValue("eventmanager","0",OptionValueType<void*>(),"Memory address of the EventManager<Event>");
    InitializeClassOptions("bulletphysics",this, mTempTferManager, mWorkQueue, mEventManager, NULL);
    OptionSet::getOptions("bulletphysics",this)->parse(options);
    Transfer::TransferManager* tm = (Transfer::TransferManager*)mTempTferManager->as<void*>();
    this->transferManager = tm;

    gravity = Vector3d(0, -9.8, 0);
    //groundlevel = 3044.0;
    groundlevel = 0.0;
    btTransform groundTransform;
    btDefaultMotionState* mMotionState;
    btVector3 worldAabbMin(-10000,-10000,-10000);
    btVector3 worldAabbMax(10000,10000,10000);
    int maxProxies = 1024;
    btVector3 localInertia(0,0,0);

    /// set up bullet stuff
    collisionConfiguration = new btDefaultCollisionConfiguration();
    //dispatcher = new btCollisionDispatcher(collisionConfiguration);
    dispatcher = new customDispatch(collisionConfiguration, &bt2siri);
    dispatcher->setNearCallback(customNearCallback);
    overlappingPairCache= new btAxisSweep3(worldAabbMin,worldAabbMax,maxProxies);
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

    /// create ground
    groundShape= new btBoxShape(btVector3(btScalar(1500.),btScalar(1.0),btScalar(1500.)));
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0,groundlevel-1,0));
    groundShape->calculateLocalInertia(0.0f,localInertia);
    mMotionState = new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f,mMotionState,groundShape,localInertia);
    groundBody = new btRigidBody(rbInfo);
    groundBody->setRestitution(0.5);                 /// bouncy for fun & profit
    dynamicsWorld->addRigidBody(groundBody);
    proxyManager->addListener(this);
    DEBUG_OUTPUT(cout << "dbm: BulletSystem::initialized, including test bullet object" << endl);
    /// we don't delete these, the ProxyManager does (I think -- someone does anyway)
//    delete mTempTferManager;
//    delete mWorkQueue;
//    delete mEventManager;
    return true;
}

BulletSystem::BulletSystem() :             mStartTime(Task::AbsTime::now()) {
    DEBUG_OUTPUT(cout << "dbm: I am the BulletSystem constructor!" << endl);
}

BulletSystem::~BulletSystem() {
    DEBUG_OUTPUT(cout << "dbm: BulletSystem destructor" << endl);

    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
    delete groundBody;
    delete groundShape;
    DEBUG_OUTPUT(cout << "dbm: BulletSystem destructor finished" << endl;)
}

void BulletSystem::createProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        DEBUG_OUTPUT(cout << "dbm: createProxy ptr:" << meshptr << " mesh: " << meshptr->getMesh() << endl;)
        objects.push_back(new bulletObj(this));     /// clean up memory!!!
        objects.back()->mMeshptr = meshptr;
        meshptr->MeshProvider::addListener(objects.back());
    }
}

void BulletSystem::destroyProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    for (unsigned int i=0; i<objects.size(); i++) {
        if (objects[i]->mMeshptr==meshptr) {
            DEBUG_OUTPUT(cout << "dbm: destroyProxy, object=" << objects[i] << endl);
            meshptr->MeshProvider::removeListener(objects[i]);
            removePhysicalObject(objects[i]);
            objects.erase(objects.begin()+i);
            break;
        }
    }
}

}//namespace sirikata
