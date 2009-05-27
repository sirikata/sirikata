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
    typedef std::map<SpaceObjectReference, std::tr1::shared_ptr<ProxyMeshObject> > ObjectMap;
    ObjectMap mObjects;

    //noncopyable
    DemoProxyManager(const DemoProxyManager&cpy){}
    ProxyObjectPtr addMeshObject(const Transfer::URI &uri, const Location &location) {
        SpaceObjectReference myId(SpaceID(UUID::null()),ObjectReference(UUID::random()));
        std::tr1::shared_ptr<ProxyMeshObject> myObj(new ProxyMeshObject(this, myId));
        mObjects.insert(ObjectMap::value_type(myId, myObj));
        notify(&ProxyCreationListener::createProxy, myObj);
        myObj->resetPositionVelocity(Time::now(), location);
        myObj->setMesh(uri);
        return myObj;
    }
public:
    DemoProxyManager()
      : mCamera(new ProxyCameraObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))),
        mLight(new ProxyLightObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))) {
    }
    void initialize(){
        notify(&ProxyCreationListener::createProxy,mCamera);
        notify(&ProxyCreationListener::createProxy,mLight);
        mCamera->attach("",0,0);
        LightInfo li;
        li.setLightDiffuseColor(Color(0,0,1));
        li.setLightAmbientColor(Color(0,0,0));
        li.setLightPower(0);
        mLight->update(li);

/*
        li.setLightDiffuseColor(Color(.7,0,1));
        li.setLightAmbientColor(Color(1,1,0));
        li.setLightPower(100);
        mAttachedLight->update(li);
*/
//        mMesh->setMesh("file:///razor.mesh");
//        mAttachedMesh->setMesh("file:///razor.mesh");
        addMeshObject(Transfer::URI("meru://cplatz@/arcade.mesh"),
                             Location(Vector3d(0,0,0), Quaternion::identity(),
                                      Vector3f(.05,0,0), Vector3f(0,0,0),0));
        addMeshObject(Transfer::URI("meru://cplatz@/arcade.mesh"),
                             Location(Vector3d(5,0,0), Quaternion::identity(),
                                      Vector3f(.05,0,0), Vector3f(0,0,0),0));
        addMeshObject(Transfer::URI("meru://cplatz@/arcade.mesh"),
                             Location(Vector3d(0,5,0), Quaternion::identity(),
                                      Vector3f(.05,0,0), Vector3f(0,0,0),0));

        mCamera->resetPositionVelocity(Time::now(),
                             Location(Vector3d(0,0,50.), Quaternion::identity(),
                                      Vector3f::nil(), Vector3f::nil(), 0.));
        mLight->resetPositionVelocity(Time::now()-Duration::seconds(.5),
                             Location(Vector3d(0,1000.,0), Quaternion::identity(),
                                      Vector3f(-1,0,0), Vector3f(0,1,0), 0.5));
        mLight->setPositionVelocity(Time::now()+Duration::seconds(.5),
                             Location(Vector3d(0,1000.,0), Quaternion::identity(),
                                      Vector3f::nil(), Vector3f::nil(), 0.));
    }
    void destroy() {
        mCamera->destroy();
        notify(&ProxyCreationListener::destroyProxy,mCamera);
        mLight->destroy();
        notify(&ProxyCreationListener::destroyProxy,mLight);
        for (ObjectMap::const_iterator iter = mObjects.begin();
             iter != mObjects.end(); ++iter) {
            (*iter).second->destroy();
            notify(&ProxyCreationListener::destroyProxy,(*iter).second);
        }
        mObjects.clear();
    }
    ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const {
        if (id == mCamera->getObjectReference()) {
            return mCamera;
        } else if (id == mLight->getObjectReference()) {
            return mLight;
        } else {
            ObjectMap::const_iterator iter = mObjects.find(id);
            if (iter == mObjects.end()) {
                return ProxyObjectPtr();
            } else {
                return (*iter).second;
            }
        }
    }
};

}
#endif
