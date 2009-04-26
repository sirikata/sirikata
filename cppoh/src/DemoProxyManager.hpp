/*  Sirikata
 *  main.cpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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
#ifndef _DEMO_PROXY_MANAGER_HPP_
#define _DEMO_PROXY_MANAGER_HPP_
#include <oh/ProxyManager.hpp>
#include <oh/ProxyCameraObject.hpp>
#include <oh/ProxyLightObject.hpp>
#include <oh/ProxyMeshObject.hpp>
namespace Sirikata {
class DemoProxyManager :public ProxyManager{
    std::tr1::shared_ptr<ProxyCameraObject> mCamera;
    std::tr1::shared_ptr<ProxyLightObject> mLight;
    std::tr1::shared_ptr<ProxyMeshObject> mAttachedMesh, mMesh;
    //noncopyable
    DemoProxyManager(const DemoProxyManager&cpy){}
public:
    DemoProxyManager()
      : mCamera(new ProxyCameraObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))),
        mLight(new ProxyLightObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))),
        mAttachedMesh(new ProxyMeshObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))),
        mMesh(new ProxyMeshObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))) {

    }
    void initialize(){
        notify(&ProxyCreationListener::createProxy,mCamera);
        notify(&ProxyCreationListener::createProxy,mLight);
        notify(&ProxyCreationListener::createProxy,mAttachedMesh);
        notify(&ProxyCreationListener::createProxy,mMesh);
        mCamera->attach("",0,0);
        LightInfo li;
        li.setLightDiffuseColor(Color(1,.5,.5));
        li.setLightAmbientColor(Color(.3,.3,.3));
        li.setLightPower(1);
        mLight->update(li);

/*
        li.setLightDiffuseColor(Color(.7,0,1));
        li.setLightAmbientColor(Color(1,1,0));
        li.setLightPower(100);
        mAttachedLight->update(li);
*/
//        mMesh->setMesh("file:///razor.mesh");
//        mAttachedMesh->setMesh("file:///razor.mesh");
        mAttachedMesh->setMesh("meru://cplatz@/arcade.mesh");
        //mAttachedMesh->setMesh("meru:///arcade.mesh");
        //mAttachedMesh->setScale(Vector3f(.1,.1,.1));
        mCamera->resetPositionVelocity(Time::now(),
                             Location(Vector3d(0,0,100.), Quaternion::identity(),
                                      Vector3f::nil(), Vector3f::nil(), 0.));
        mLight->resetPositionVelocity(Time::now()-Duration::seconds(.5),
                             Location(Vector3d(0,1000.,0), Quaternion::identity(),
                                      Vector3f(-1,0,0), Vector3f(0,1,0), 0.5));
        mLight->setPositionVelocity(Time::now()+Duration::seconds(.5),
                             Location(Vector3d(0,1000.,0), Quaternion::identity(),
                                      Vector3f::nil(), Vector3f::nil(), 0.));

        mMesh->resetPositionVelocity(Time::now()-Duration::seconds(1),
                             Location(Vector3d(0,0,100.), Quaternion::identity(),
                                      Vector3f(0,0,0), Vector3f(0,0,0), 0.));

/*
        mMesh->setPositionVelocity(Time::now(),
                             Location(Vector3d(0,0,0), Quaternion::identity(),
                                      Vector3f(0,1,0), Vector3f(0.71,0.71,0), 0.5));
*/
        mAttachedMesh->setParent(mMesh, Time::now(),
                             Location(Vector3d(2.5,5.,0), Quaternion::identity(),
                                      Vector3f(0,0,0), Vector3f::nil(), 0.),
                             Location(Vector3d(2.5,5.,0), Quaternion::identity(),
                                      Vector3f(0,0,0), Vector3f::nil(), 0.));
/*
        mAttachedMesh->resetPositionVelocity(Time::now(),
                             Location(Vector3d(-10,0,0), Quaternion::identity(),
                                      Vector3f(0,0,0), Vector3f::nil(), 0.));
*/
        mMesh->resetPositionVelocity(Time::now(),
                             Location(Vector3d(0,0,0), Quaternion::identity(),
                                      Vector3f(2,0,0), Vector3f(0,0,0),0));
        mMesh->destroy();
//        mAttachedMesh->unsetParent(Time::now());
/*
        mAttachedMesh->unsetParent(Time::now(),
                             Location(Vector3d(2.5,5.,0), Quaternion::identity(),
                                      Vector3f(0,-3,0), Vector3f::nil(), 0.));
*/
    }
    void fiveSeconds() {
        mMesh->destroy();
//        mAttachedMesh->setParent(mMesh, Time::now());
    }
    void tenSeconds() {
        mAttachedMesh->unsetParent(Time::now());
    }
    void destroy() {
        mCamera->destroy();
        mLight->destroy();
        mAttachedMesh->destroy();
        //mMesh->destroy();
        notify(&ProxyCreationListener::destroyProxy,mCamera);
        notify(&ProxyCreationListener::destroyProxy,mAttachedMesh);
        notify(&ProxyCreationListener::destroyProxy,mLight);
        notify(&ProxyCreationListener::destroyProxy,mMesh);
    }
    ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const {
        if (id == mCamera->getObjectReference()) {
            return mCamera;
        } else if (id == mLight->getObjectReference()) {
            return mLight;
        } else if (id == mAttachedMesh->getObjectReference()) {
            return mAttachedMesh;
        } else if (id == mMesh->getObjectReference()) {
            return mMesh;
        } else {
            return ProxyObjectPtr();
        }
    }
};

}
#endif
