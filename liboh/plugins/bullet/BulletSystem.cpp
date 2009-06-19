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

static URI streetlight("mhash:///66b189d665dd968d94a1415393f0037299fc2d9a18ca457dba5038f90083157c");
static Task::AbsTime starttime = Task::AbsTime::now();
static Task::AbsTime lasttime = starttime;
static Task::DeltaTime onesec = Task::DeltaTime::seconds(1.0);
static int bugoffset = 0;

bool BulletSystem::tick() {
    Task::AbsTime now = Task::AbsTime::now();
    cout << "dbm: BulletSystem::tick time: " << (now-starttime).toSeconds() << endl;
    for (unsigned int i=0; i<objects.size(); i++) {
        cout << "  dbm: BS:tick object: " << objects[i]->meshname << endl;
        if (objects[i]->meshname == streetlight) {
            if(now > lasttime + onesec) {
                lasttime = now;
                cout << "    tick -- moving!" << endl;
                bugoffset ^= 1;
                objects[i]->meshptr->setPosition(now,
                        Vector3d(580+bugoffset, 3049+bugoffset, 1046+bugoffset),
                        Quaternion());
            }
            cout << "     tick, that's a nice streetlight! it's at: " <<
                    objects[i]->meshptr->getPosition()
                    << endl;
        }
    }
    cout << endl;
    return 0;
}

bool BulletSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    cout << "dbm: BulletSystem::initialize" << endl;
    proxyManager->addListener(this);
    return true;
}

BulletSystem::BulletSystem() {
    cout << "dbm: I am the BulletSystem constructor!" << endl;
}

BulletSystem::~BulletSystem() {
    cout << "dbm: I am the BulletSystem destructor!" << endl;
}

void BulletSystem::createProxy(ProxyObjectPtr p) {
    ProxyMeshObjectPtr meshptr(tr1::dynamic_pointer_cast<ProxyMeshObject>(p));
    if (meshptr) {
        objects.push_back(new bulletObj());     /// clean up memory!!!
        objects.back()->meshptr = meshptr;
        meshptr->MeshProvider::addListener(objects.back()); /// dbm -- buggy phorn looking at
    }
}

void BulletSystem::destroyProxy(ProxyObjectPtr p) {
}

}//namespace sirikata
