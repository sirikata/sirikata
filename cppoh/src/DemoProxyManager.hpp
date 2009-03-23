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
    std::tr1::shared_ptr<ProxyMeshObject> mMesh;
    //noncopyable
    DemoProxyManager(const DemoProxyManager&cpy){}
public:
    DemoProxyManager()
        : mCamera(new ProxyCameraObject(SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))),
        mLight(new ProxyLightObject(SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))),
         mMesh(new ProxyMeshObject(SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))) {
        
    }
    void initialize(){
        notify(&ProxyCreationListener::createProxy,mCamera);
        notify(&ProxyCreationListener::createProxy,mLight);
        notify(&ProxyCreationListener::createProxy,mMesh);
        mCamera->attach("",0,0);
        mCamera->setPosition(Time::now(), Vector3d(-100,0,0), Quaternion::identity());
        LightInfo li;
        li.setLightDiffuseColor(Color(1,.5,.5));
        li.setLightAmbientColor(Color(.3,.3,.3));
        li.setLightPower(1);
        mLight->update(li);
        mLight->setPosition(Time::now(), Vector3d(0,0,1000.), Quaternion::identity());
        mMesh->setMesh("file:///razor.mesh");
        mMesh->setPosition(Time::now(), Vector3d(0,0,0), Quaternion::identity());
    }
    void destroy() {
        mCamera->detach();
        notify(&ProxyCreationListener::destroyProxy,mCamera);
        notify(&ProxyCreationListener::destroyProxy,mLight);
        notify(&ProxyCreationListener::destroyProxy,mMesh);
    }
};

}
#endif
