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

#ifndef _SIRIKATA_BULLET_PHYSICS_
#define _SIRIKATA_BULLET_PHYSICS_

#include <util/Platform.hpp>
#include <util/Time.hpp>
#include <util/ListenerProvider.hpp>
#include <oh/TimeSteppedSimulation.hpp>
#include <oh/ProxyObject.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <oh/ProxyMeshObject.hpp>
#include <task/EventManager.hpp>
#include <options/Options.hpp>
#include <transfer/TransferManager.hpp>
#include "btBulletDynamicsCommon.h"

using namespace std;
namespace Sirikata {

/*
                 dead simple, brute-force parsing of ogre.mesh to get the precious vertex data for physics
*/
class parseOgreMesh {
    vector<unsigned char> data;
    vector<double>* vertices;
    vector<int>* indices;
    vector<double>* bounds;
    int ix;
    int lastVertexCount, lastVertexSize;

    int read_ubyte() {
        int r;
        if ((int)data.size()<=ix) {
            return 0;
        }
        r = data[ix];
        ix = (ix+1);
        return r;
    }

    int read_bool() {
        return (read_ubyte()>0);
    }

    int read_u16() {
        return (read_ubyte()+(read_ubyte()*256));
    }

    int read_u32() {
        int n;
        n = 0;
        n = (n+read_ubyte());
        n = (n+(read_ubyte()<<8));
        n = (n+(read_ubyte()<<16));
        n = (n+(read_ubyte()<<24));
        return n;
    }

    double power(double b, int e) {
        double n=1.0;
        if (e<0) {
            e = -e;
            b = 1.0/b;
        }
        for (int i=0; i<e; i++) {
            n *= b;
        }
        return n;
    }

    double read_float() {
        /// ok, this may look ridiculous, but I'm concerned about alternative architectures
        /// the format is IEEE single-precision float; I don't trust casting to work everywhere
        /// note this conversion does not deal properly with NAN, INF...
        double man, v;
        int exp, sign, u;
        u = read_u32();
        if ((!u)) {
            return 0.0;
        }
        exp = (u>>23)&255;
        sign = u>>31?-1:1;
        man = (((double)(u&0x7fffff))/((double)0x800000))+1.0;
        v = (sign * power(2.0, (exp-127)) ) * man;
        return v;
    }

    string read_string() {
        string s;
        int b;
        b = read_ubyte();
        while (!(b==10)) {
            s.push_back(b);
            b = read_ubyte();
        }
        return s;
    }

    void read_chunks(int count) {
        int start;
        start = ix;
        while ( ix<(start+count) & ix<(int)data.size() ) {
            read_chunk();
        }
    }

    void read_chunk() {
        string version;
        int count, floatsPerVertex, i, id, ii, indexCount, indexes32Bit, jj, skeletallyAnimated, skip, useSharedVertices;

        id = read_u16();
        if ((id==0x1000)) {                                 /// HEADER
            version = read_string();
        }
        else {
            count = read_u32();
            if ((id==0x3000)) {                             /// MESH
                skeletallyAnimated = read_bool();
                read_chunks(count);
            }
            else if ((id==0x4000)) {                        /// SUBMESH
                read_string();
                useSharedVertices = read_bool();
                indexCount = read_u32();
                indexes32Bit = read_bool();

                for (i=0; i<indexCount; i+=1) {
                    indices->push_back(indexes32Bit?read_u32():read_u16());
                }

                if ((!useSharedVertices)) {
                    read_chunk();
                }
            }
            else if ((id==0x5000)) {                        /// GEOMETRY
                lastVertexCount = read_u32();
                read_chunks((count-10));
            }
            else if ((id==0x5100)) {                        /// GEOMETRY_VERTEX_DECLARATION
                read_chunks((count-6));
            }
            else if ((id==0x5110)) {                        /// GEOMETRY_VERTEX_ELEMENT
                read_u16();
                read_u16();
                read_u16();
                read_u16();
                read_u16();
            }
            else if ((id==0x5200)) {                        /// GEOMETRY_VERTEX_BUFFER
                read_u16();
                lastVertexSize = read_u16();
                read_chunk();
            }
            else if ((id==0x5210)) {                        /// GEOMETRY_VERTEX_BUFFER_DATA
                floatsPerVertex = lastVertexSize>>2;
                skip = (floatsPerVertex-3);

                for (ii=0; ii<lastVertexCount; ii+=1) {
                    for (jj=0; jj<3; jj+=1) {
                        vertices->push_back(read_float());
                    }
                    for (jj=0; jj<skip; jj+=1) {
                        read_u32();
                    }
                }
            }
            /*
            else if ((id==0x8110)) {
                string name = read_string();
                cout << "dbm:mesh name: " << name << endl;
            }
            */
            else if ((id==0x9000)) {                        /// MESH_BOUNDS
                for (i=0; i<7; i+=1) {
                    bounds->push_back(read_float());
                }
            }
            else {
                ix = (ix+(count-6));
            }
        }
    }
public:
    bool parseData(const unsigned char* rawdata, int bytes, vector<double>& vertices, vector<int>& indices, vector<double>& bounds)    {
        char temp;
        this->vertices = &vertices;
        this->indices = &indices;
        this->bounds = &bounds;
        data.clear();
        ix=0;
        for (int i=0; i<bytes; i++) {
            data.push_back(rawdata[i]);
        }
//        data.pop_back();                                    // why? whence extra byte?
        printf("read data %d bytes\n", data.size());
        read_chunks(data.size());
        return true;
    }

    bool parseFile(const char* filename, vector<double>& vertices, vector<int>& indices, vector<double>& bounds)    {
        char temp;
        this->vertices = &vertices;
        this->indices = &indices;
        this->bounds = &bounds;
        data.clear();
        ix=0;
        ifstream f;
        f.open(filename);
        if (!f) {
            return false;
        }
        while (!f.eof()) {
            f.read(&temp, 1);
            data.push_back(temp);
        }
        f.close();
        data.pop_back();                                    // why? whence extra byte?
        printf("read data %d bytes\n", data.size());
        read_chunks(data.size());
        return true;
    }
};//class parseOgreMesh


typedef tr1::shared_ptr<ProxyMeshObject> ProxyMeshObjectPtr;

struct positionOrientation {
    Vector3d p;
    Quaternion o;
    positionOrientation() {
    };
    positionOrientation(const Vector3d &p, const Quaternion &o) {
        this->p = p;
        this->o = o;
    };
    positionOrientation(const btVector3 &p, const btQuaternion &o) {
        this->p = Vector3d(p.getX(), p.getY(), p.getZ());
        this->o = Quaternion(o.getX(), o.getY(), o.getZ(), o.getW(), Quaternion::XYZW());
    };
};

class BulletSystem;

class bulletObj : public MeshListener,Noncopyable {
    friend class BulletSystem;
    enum shapeID {
        ShapeMesh,
        ShapeBox,
        ShapeSphere
    };
    BulletSystem* system;
    void setPhysical (const physicalParameters &pp);
    void meshChanged (const URI &newMesh);
    void setScale (const Vector3f &newScale);

    /// these guys seem to need to stay around for the lifetime of the object.  Otherwise we crash
    btScalar* mBtVertices;//<-- this dude must be aligned on 16 byte boundaries
    vector<double> mVertices;
    vector<int> mIndices;
    btTriangleIndexVertexArray* mIndexArray;
    btDefaultMotionState* mMotionState;
    float mDensity;
    float mFriction;
    float mBounce;
    bool mActive;              /// anything that bullet sees is active
    bool mDynamic;             /// but only some are dynamic (affected by forces)
    shapeID mShape;
    positionOrientation mInitialPo;
    Vector3d mVelocity;
    btRigidBody* mBulletBodyPtr;
    btCollisionShape* mColShape;
    ProxyMeshObjectPtr mMeshptr;
    URI mMeshname;
    float mSizeX;
    float mSizeY;
    float mSizeZ;
    string mName;
public:
    /// public members -- yes, I use 'em.  No, I don't always thicken my code with gettr/settr's
    int colMask;
    int colMsg;

    enum mode {
        Disabled,               /// non-active, remove from physics
        Static,                 /// collisions, no dynamic movement (bullet mass==0)
        DynamicBox,                 /// fully physical -- collision & dynamics
        DynamicSphere                 /// fully physical -- collision & dynamics
    };
    /// public methods
    bulletObj(BulletSystem* sys) :
            mBtVertices(NULL),
            mIndexArray(NULL),
            mMotionState(NULL),
            mActive(false),
            mDynamic(false),
            mVelocity(Vector3d()),
            mBulletBodyPtr(NULL),
            mColShape(NULL),
            mSizeX(0),
            mSizeY(0),
            mSizeZ(0),
            mName("") {
        system = sys;
    }
    ~bulletObj();
    positionOrientation getBulletState();
    void setBulletState(positionOrientation pq);
    void buildBulletBody(const unsigned char*, int);
    void buildBulletShape(const unsigned char* meshdata, int meshbytes, float& mass);
};

class customDispatch :public btCollisionDispatcher {
    /// the entire point of this subclass is to flag collisions in collisionPairs
public:
    map<btCollisionObject*, bulletObj*>* bt2siri;
    map<set<bulletObj*>, int> collisionPairs;
    customDispatch (btCollisionConfiguration* collisionConfiguration,
                    map<btCollisionObject*, bulletObj*>* bt2siri) :
            btCollisionDispatcher(collisionConfiguration) {
        this->bt2siri=bt2siri;
    }
};

class BulletSystem: public TimeSteppedSimulation {
    bool initialize(Provider<ProxyCreationListener*>*proxyManager,
                    const String&options);
    Vector3d gravity;
    double groundlevel;
    OptionValue* mTempTferManager;
    OptionValue* mWorkQueue;
    OptionValue* mEventManager;
    Task::AbsTime mStartTime;

    ///local bullet stuff:
    btDefaultCollisionConfiguration* collisionConfiguration;
    customDispatch* dispatcher;
    btAxisSweep3* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btCollisionShape* groundShape;
    btRigidBody* groundBody;

public:
    BulletSystem();
    map<btCollisionObject*, bulletObj*> bt2siri;  /// map bullet bodies (what we get in the callbacks) to bulletObj's
    btDiscreteDynamicsWorld* dynamicsWorld;
    vector<bulletObj*>objects;
//    btAlignedObjectArray<btCollisionShape*> collisionShapes;
    Transfer::TransferManager*transferManager;
    void addPhysicalObject(bulletObj* obj, positionOrientation po,
                           float density, float friction, float bounce,
                           float sizx, float sizy, float sizz);
    void removePhysicalObject(bulletObj*);
    static TimeSteppedSimulation* create(Provider<ProxyCreationListener*>*proxyManager,
                                         const String&options) {
        BulletSystem*os= new BulletSystem;
        if (os->initialize(proxyManager,options))
            return os;
        delete os;
        return NULL;
    }
    void test();
    virtual void createProxy(ProxyObjectPtr p);
    virtual void destroyProxy(ProxyObjectPtr p);
    virtual Duration desiredTickRate()const {
        return Duration::seconds(0.1);
    };
    Task::EventResponse downloadFinished(Task::EventPtr evbase, bulletObj* bullobj);
    ///returns if rendering should continue
    virtual bool tick();
    ~BulletSystem();
};
}
#endif
