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

static OptionValue *scenefile;

static const InitializeGlobalOptions globalst (
    "cppoh",
    new OptionValue("scenefile", "scene.txt", OptionValueType<std::string>(), "A text file with locations and filenames of all meshes in a scene.", &scenefile),
    NULL);

class DemoProxyManager :public ProxyManager{
    std::tr1::shared_ptr<ProxyCameraObject> mCamera;
    typedef std::map<SpaceObjectReference, ProxyObjectPtr > ObjectMap;
    ObjectMap mObjects;

    //noncopyable
    DemoProxyManager(const DemoProxyManager&cpy){}

    ProxyObjectPtr addMeshObject(const Transfer::URI &uri, const Location &location,
                                 const Vector3f &scale=Vector3f(1,1,1), const int mode=0) {
        // parentheses around arguments required to resolve function/constructor ambiguity. This is ugly.
        SpaceObjectReference myId((SpaceID(UUID::null())),(ObjectReference(UUID::random())));
        std::cout << "Add Mesh Object " << myId << " = " << uri << " mode: " << mode << std::endl;
        std::tr1::shared_ptr<ProxyMeshObject> myObj(new ProxyMeshObject(this, myId));
        mObjects.insert(ObjectMap::value_type(myId, myObj));
        notify(&ProxyCreationListener::createProxy, myObj);
        myObj->resetPositionVelocity(Time::now(), location);
        myObj->setMesh(uri);
        myObj->setScale(scale);
        physicalParameters pp;
        pp.mode = mode;
        pp.density = 1.0;
        pp.friction = 0.9;
        pp.bounce = 0.03;
        myObj->setPhysical(pp);
        return myObj;
    }

    ProxyObjectPtr addLightObject(const LightInfo &linfo, const Location &location) {
        // parentheses around arguments required to resolve function/constructor ambiguity. This is ugly.
        SpaceObjectReference myId((SpaceID(UUID::null())),(ObjectReference(UUID::random())));

        std::tr1::shared_ptr<ProxyLightObject> myObj(new ProxyLightObject(this, myId));
        mObjects.insert(ObjectMap::value_type(myId, myObj));
        notify(&ProxyCreationListener::createProxy, myObj);
        myObj->resetPositionVelocity(Time::now(), location);
        myObj->update(linfo);
        return myObj;
    }

    void loadSceneObject(FILE *fp) {
        Vector3d pos(0,0,0);
        Quaternion orient(Quaternion::identity());
        Vector3f scale(1,1,1);
        int mode=0;

        fscanf(fp,"(%lf %lf %lf) [%f %f %f %f] <%f %f %f> {%d} \
                ",&pos.x,&pos.y,&pos.z,&orient.w,&orient.x,&orient.y,&orient.z,&scale.x,&scale.y,&scale.z,&mode);
        if (mode) std::cout << "dbm mode: " << mode << std::endl;
        Location location(pos, orient, Vector3f::nil(), Vector3f::nil(), 0.);
        // Read a line into filename.
        std::string filename;
        while (true) {
            char str[1024];
			str[0]='\0';
            fgets(str, 1024, fp);
            std::string append(str);
            if (append.length() == 0) {
                // EOF
                break;
            } else if (append[append.length()-1]=='\n') {
                filename += append.substr(0, append.length()-1);
                break;
            } else {
                filename += append;
            }
        }
        std::string rest;
        std::string::size_type sppos=filename.find(' ');
        if (sppos != std::string::npos) {
            rest = filename.substr(sppos+1);
            filename = filename.substr(0, sppos);
        }
        if (filename=="CAMERA") {
            mCamera->resetPositionVelocity(Time::now(), location);
        } else if (filename=="point" || filename=="directional" || filename=="spotlight") {
            LightInfo::LightTypes lighttype;
            if (filename=="point") {
                lighttype = LightInfo::POINT;
            } else if (filename=="spotlight") {
                lighttype = LightInfo::SPOTLIGHT;
            } else if (filename=="directional") {
                lighttype = LightInfo::DIRECTIONAL;
            }
            LightInfo lightInfo;
            float ambientPower=1, shadowPower=1;
            float x,y,z; // Light direction. Assume 0,1,0 for now.
            int castsshadow=1;
            sscanf(rest.c_str(),"[%f %f %f %f] [%f %f %f %f] <%lf %f %f %f> <%f %f> [%f] %f %d <%f %f %f>",
                   &lightInfo.mDiffuseColor.x,&lightInfo.mDiffuseColor.y,&lightInfo.mDiffuseColor.z,&ambientPower,
                   &lightInfo.mSpecularColor.x,&lightInfo.mSpecularColor.y,&lightInfo.mSpecularColor.z,&shadowPower,
                   &lightInfo.mLightRange,&lightInfo.mConstantFalloff,&lightInfo.mLinearFalloff,&lightInfo.mQuadraticFalloff,
                   &lightInfo.mConeInnerRadians,&lightInfo.mConeOuterRadians,&lightInfo.mPower,&lightInfo.mConeFalloff,
                   &castsshadow,&x,&y,&z);
            lightInfo.mCastsShadow = castsshadow?true:false;

            if (lightInfo.mDiffuseColor.length()&&lightInfo.mPower) {
                lightInfo.mAmbientColor = lightInfo.mDiffuseColor*(ambientPower/lightInfo.mDiffuseColor.length())/lightInfo.mPower;
            } else {
                lightInfo.mAmbientColor = Vector3f(0,0,0);
            }
            if (lightInfo.mSpecularColor.length()&&lightInfo.mPower) {
                lightInfo.mShadowColor = lightInfo.mSpecularColor*(shadowPower/lightInfo.mSpecularColor.length())/lightInfo.mPower;
            } else {
                lightInfo.mShadowColor = Vector3f(0,0,0);
            }
            lightInfo.mWhichFields = LightInfo::ALL;
            addLightObject(lightInfo, location);
        } else {
            addMeshObject(Transfer::URI(filename), location, scale, mode);
        }
    }

    bool loadSceneFile(std::string filename) {
        FILE *fp;
        for (int i = 0; i < 4; ++i) {
            fp = fopen(filename.c_str(), "rt");
            if (fp) break;
            filename = std::string("../")+filename;
        }
        if (!fp) {
            return false;
        }
        while (!feof(fp)) {
            loadSceneObject(fp);
        }
        fclose(fp);
        return true;
    }

public:
    DemoProxyManager()
      : mCamera(new ProxyCameraObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))) {
    }

    virtual void createObject(const ProxyObjectPtr &newObj) {
        notify(&ProxyCreationListener::createProxy,newObj);
        mObjects.insert(ObjectMap::value_type(newObj->getObjectReference(), newObj));
    }

    virtual void destroyObject(const ProxyObjectPtr &newObj) {
        ObjectMap::iterator iter = mObjects.find(newObj->getObjectReference());
        if (iter != mObjects.end()) {
            newObj->destroy();
            notify(&ProxyCreationListener::destroyProxy,newObj);
            mObjects.erase(iter);
        }
    }

    void initialize(){
        notify(&ProxyCreationListener::createProxy,mCamera);
        mCamera->attach("",0,0);
        mCamera->resetPositionVelocity(Time::now(),
                             Location(Vector3d(0,0,50.), Quaternion::identity(),
                                      Vector3f::nil(), Vector3f::nil(), 0.));

        if (loadSceneFile(scenefile->as<std::string>())) {
            return; // success!
        }
        // Otherwise, load fallback scene.

        LightInfo li;
        li.setLightDiffuseColor(Color(0.92,0.92,0.92));
        li.setLightAmbientColor(Color(0.0001,0.0001,0.0001));
        li.setLightSpecularColor(Color(0,0,0));
        li.setLightShadowColor(Color(0,0,0));
        li.setLightPower(0.5);
		li.setLightRange(1000);
		li.setLightFalloff(0.1,0,0);
        addLightObject(li, Location(Vector3d(0,1000.,0), Quaternion::identity(),
                                      Vector3f::nil(), Vector3f::nil(), 0.));

        addMeshObject(Transfer::URI("meru://cplatz@/arcade.mesh"),
                             Location(Vector3d(0,0,0), Quaternion::identity(),
                                      Vector3f(.05,0,0), Vector3f(0,0,0),0));
        addMeshObject(Transfer::URI("meru://cplatz@/arcade.mesh"),
                             Location(Vector3d(5,0,0), Quaternion::identity(),
                                      Vector3f(.05,0,0), Vector3f(0,0,0),0));
        addMeshObject(Transfer::URI("meru://cplatz@/arcade.mesh"),
                             Location(Vector3d(0,5,0), Quaternion::identity(),
                                      Vector3f(.05,0,0), Vector3f(0,0,0),0));
    }
    void destroy() {
        mCamera->destroy();
        notify(&ProxyCreationListener::destroyProxy,mCamera);
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
