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

#ifndef _SIRIKATA_BULLET_PHYSICS_
#define _SIRIKATA_BULLET_PHYSICS_

#include <util/Platform.hpp>
#include <util/Time.hpp>
#include <util/ListenerProvider.hpp>
#include <oh/TimeSteppedSimulation.hpp>
#include <oh/ProxyObject.hpp>
#include <oh/ProxyMeshObject.hpp>

using namespace std;
namespace Sirikata {

typedef tr1::shared_ptr<ProxyMeshObject> ProxyMeshObjectPtr;
//vector<ProxyMeshObjectPtr>mymesh;

class BulletSystem;

class bulletObj : public MeshListener {
    void meshChanged (const URI &newMesh);
    void setScale (const Vector3f &newScale);
    BulletSystem* system;
public:
    Vector3d velocity;
    bulletObj(BulletSystem* sys);
    ProxyMeshObjectPtr meshptr;
    URI meshname;
};

class BulletSystem: public TimeSteppedSimulation {
    bool initialize(Provider<ProxyCreationListener*>*proxyManager,
                    const String&options);
    vector<bulletObj*>objects;
    vector<bulletObj*>physicalObjects;
    Vector3d gravity;
public:
    BulletSystem();
    void addPhysicalObject(bulletObj*);
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
    ///returns if rendering should continue
    virtual bool tick();
    ~BulletSystem();
};
}
#endif
