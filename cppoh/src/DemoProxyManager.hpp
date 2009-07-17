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
#include <oh/ProxyWebViewObject.hpp>
#include <oh/BulletSystem.hpp>

namespace Sirikata {

using namespace std;

#define DEG2RAD 0.0174532925

class DemoProxyManager :public ProxyManager {
    std::tr1::shared_ptr<ProxyCameraObject> mCamera;
    typedef std::map<SpaceObjectReference, ProxyObjectPtr > ObjectMap;
    ObjectMap mObjects;

    //noncopyable
    DemoProxyManager(const DemoProxyManager&cpy) {}

    ProxyObjectPtr addMeshObject(const Transfer::URI &uri, const Location &location,
                                 const Vector3f &scale=Vector3f(1,1,1),
                                 const int mode=0, const float density=0.f, const float friction=0.f, const float bounce=0.f) {
        // parentheses around arguments required to resolve function/constructor ambiguity. This is ugly.
        SpaceObjectReference myId((SpaceID(UUID::null())),(ObjectReference(UUID::random())));
        std::cout << "Add Mesh Object " << myId << " = " << uri << " mode: " << mode << std::endl;
        std::tr1::shared_ptr<ProxyMeshObject> myObj(new ProxyMeshObject(this, myId));
        mObjects.insert(ObjectMap::value_type(myId, myObj));
        notify(&ProxyCreationListener::createProxy, myObj);
        myObj->resetPositionVelocity(Time::now(), location);
        myObj->setMesh(uri);
        myObj->setScale(scale);
        physicalParameters pp = {0};
        pp.mode = mode;
        pp.density = density;
        pp.friction = friction;
        pp.bounce = bounce;
        myObj->setPhysical(pp);             /// always do this to ensure parameters are valid
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
    ProxyObjectPtr addWebViewObject(const std::string &url, int width=500, int height=400) {
        SpaceObjectReference myId((SpaceID(UUID::null())),(ObjectReference(UUID::random())));
        std::tr1::shared_ptr <ProxyWebViewObject> mWebView(new ProxyWebViewObject(this, SpaceObjectReference(myId)));
        mObjects.insert(ObjectMap::value_type(myId, mWebView));
        notify(&ProxyCreationListener::createProxy,mWebView);
        mWebView->resize(width, height);
        mWebView->setPosition(OverlayPosition(RP_TOPRIGHT));
        mWebView->loadURL(url);
        return mWebView;
    }

    double str2dbl(string s) {
        float f;
        if (s=="") return 0.0;
        sscanf(s.c_str(), "%f", &f);
        return (double)f;
    }

    int str2int(string s) {
        int d;
        if (s=="") return 0;
        sscanf(s.c_str(), "%d", &d);
        return d;
    }

    void parse_csv_values(string line, vector<string>& values) {
        values.clear();
        string temp("");
        for (unsigned int i=0; i<line.size(); i++) {
            char c = line[i];
            if (c!=',') {
                if ((c!=' ') && (c!='"')) {
                    temp.push_back(c);
                }
            }
            else {
                values.push_back(temp);
                temp.clear();
            }
        }
        values.push_back(temp);
    }

    vector<string> headings;

    void getline(FILE* f, string& s) {
        s.clear();
        while (true) {
            char c = fgetc(f);
            if (c<0) {
                s.clear();
                break;
            }
            if (c==0x0d && s.size()==0) continue;            /// should deal with windows \n
            if (c==0x0a || c==0x0d) {
                break;
            }
            s.push_back(c);
        }
    }

    void parse_csv_headings(FILE* fil) {
        string line;
        getline(fil, line);
        parse_csv_values(line, headings);
    }

    map<string, string>* parse_csv_line(FILE* fil) {
        string line;
        getline(fil, line);
        //std::cout << "csv: line-->" << line << "<--" << std::endl;
        vector<string> values;
        map<string, string> *row;
        row = (new map<string, string>());
        if (line.size()>0) {
            parse_csv_values(line, values);
            for (unsigned int i=0; i < values.size(); i++) {
                (*row)[headings[i]] = values[i];
            }
        }
        return row;
    }

    void loadSceneObject(FILE *fp) {
        Vector3d pos(0,0,0);
        Quaternion orient(Quaternion::identity());
        Vector3f scale(1,1,1);
        float density=0;
        float friction=0;
        float bounce=0;

        /// dbm new way:
        map<string, string>& row = *parse_csv_line(fp);
        std::cout << endl;
        if (row["objtype"][0]=='#' || row["objtype"]==string("")) {
            //cout << "csv: loadSceneObject passing, comment or blank line" << endl;
            return;                                         /// comment or blank line
        }
        else {
            string objtype = row["objtype"];
            pos.x = str2dbl(row["pos_x"]);
            pos.y = str2dbl(row["pos_y"]);
            pos.z = str2dbl(row["pos_z"]);
            orient.x = str2dbl(row["orient_x"]);
            orient.y = str2dbl(row["orient_y"]);
            orient.z = str2dbl(row["orient_z"]);
            if (row["orient_w"].size()) {
                orient.w = str2dbl(row["orient_w"]);                /// if w is specified, assume quaternion
            }
            else {                                                  /// if no w, Euler angles
                /// we can replace this later if need be. btQuat takes yaw, pitch, roll with Z as up.
                btQuaternion bq = btQuaternion(DEG2RAD*orient.y, DEG2RAD*orient.x, DEG2RAD*orient.z);
                orient.x = bq.getX();
                orient.y = bq.getY();
                orient.z = bq.getZ();
                orient.w = bq.getW();
            }
            //cout << "csv: orient: " << orient.x <<","<< orient.y <<","<< orient.z <<","<< orient.w << endl;
            Location location(pos, orient, Vector3f::nil(), Vector3f::nil(), 0.);
            scale.x = str2dbl(row["scale_x"]);
            scale.y = str2dbl(row["scale_y"]);
            scale.z = str2dbl(row["scale_z"]);

            if (objtype=="camera") {
                mCamera->resetPositionVelocity(Time::now(), location);
                //cout << "csv: added camera to scene" << endl;
            }
            else if (objtype=="light") {
                LightInfo::LightTypes lighttype;
                if (row["subtype"]=="point") {
                    lighttype = LightInfo::POINT;
                }
                else if (row["subtype"]=="spotlight") {
                    lighttype = LightInfo::SPOTLIGHT;
                }
                else if (row["subtype"]=="directional") {
                    lighttype = LightInfo::DIRECTIONAL;
                }
                else {
                    cout << "parse csv error: unknown light subtype" << endl;
                    assert(false);
                }
                LightInfo lightInfo;
                lightInfo.setLightType(lighttype);
                float ambientPower=1, shadowPower=1;
                float x,y,z; // Light direction. Assume 0,1,0 for now.
                x = str2dbl(row["direct_x"]);
                y = str2dbl(row["direct_y"]);
                z = str2dbl(row["direct_z"]);
                string castsshadow=row["shadow"];
                lightInfo.mDiffuseColor.x = str2dbl(row["diffuse_x"]);
                lightInfo.mDiffuseColor.y = str2dbl(row["diffuse_y"]);
                lightInfo.mDiffuseColor.z = str2dbl(row["diffuse_z"]);
                ambientPower  = str2dbl(row["ambient"]);
                lightInfo.mSpecularColor.x = str2dbl(row["specular_x"]);
                lightInfo.mSpecularColor.y = str2dbl(row["specular_y"]);
                lightInfo.mSpecularColor.z = str2dbl(row["specular_z"]);
                shadowPower = str2dbl(row["shadowpower"]);
                lightInfo.mLightRange = str2dbl(row["range"]);
                lightInfo.mConstantFalloff = str2dbl(row["constfall"]);
                lightInfo.mLinearFalloff = str2dbl(row["linearfall"]);
                lightInfo.mQuadraticFalloff = str2dbl(row["quadfall"]);
                lightInfo.mConeInnerRadians = str2dbl(row["cone_in"]);
                lightInfo.mConeOuterRadians = str2dbl(row["cone_out"]);
                lightInfo.mPower = str2dbl(row["power"]);
                lightInfo.mConeFalloff = str2dbl(row["cone_fall"]);
                lightInfo.mCastsShadow = castsshadow=="true"?true:false;

                if (lightInfo.mDiffuseColor.length()&&lightInfo.mPower) {
                    lightInfo.mAmbientColor = lightInfo.mDiffuseColor*(ambientPower/lightInfo.mDiffuseColor.length())/lightInfo.mPower;
                }
                else {
                    lightInfo.mAmbientColor = Vector3f(0,0,0);
                }
                if (lightInfo.mSpecularColor.length()&&lightInfo.mPower) {
                    lightInfo.mShadowColor = lightInfo.mSpecularColor*(shadowPower/lightInfo.mSpecularColor.length())/lightInfo.mPower;
                }
                else {
                    lightInfo.mShadowColor = Vector3f(0,0,0);
                }
                lightInfo.mWhichFields = LightInfo::ALL;
                addLightObject(lightInfo, location);
                //cout << "csv: added light to scene" << endl;
            }
            else if (objtype=="mesh") {
                int mode=0;
                double density=0.0;
                double friction=0.0;
                double bounce=0.0;
                if (row["subtype"] == "staticmesh") {
                    mode=bulletObj::Static;
                    friction = str2dbl(row["friction"]);
                    bounce = str2dbl(row["bounce"]);
                }
                else if (row["subtype"] == "dynamicbox") {
                    mode=bulletObj::DynamicBox;
                    density = str2dbl(row["density"]);
                    friction = str2dbl(row["friction"]);
                    bounce = str2dbl(row["bounce"]);
                }
                else if (row["subtype"] == "dynamicsphere") {
                    mode=bulletObj::DynamicSphere;
                    density = str2dbl(row["density"]);
                    friction = str2dbl(row["friction"]);
                    bounce = str2dbl(row["bounce"]);
                }
                else if (row["subtype"] == "graphiconly") {
                }
                else {
                    cout << "parse csv error: unknown mesh subtype:" << row["subtype"] << endl;
                    assert(false);
                }
                string meshURI = row["meshURI"];
                if (sizeof(string)==0) {
                    cout << "parse csv error: no meshURI" << endl;
                    assert(false);
                }
                addMeshObject(Transfer::URI(meshURI), location, scale, mode, density, friction, bounce);
                //cout << "csv: added mesh to scene.  subtype: " << row["subtype"] << " mode: " << mode << " density: " << density << endl;
            }
            else {
                cout << "parse csv error: illegal object type" << endl;
                assert(false);
            }
        }
    }

    void loadSceneObject_txt(FILE *fp) {
        Vector3d pos(0,0,0);
        Quaternion orient(Quaternion::identity());
        Vector3f scale(1,1,1);
        int mode=0;
        float density=0;
        float friction=0;
        float bounce=0;

        int ret = fscanf(fp,"(%lf %lf %lf) [%f %f %f %f] <%f %f %f> ",
                         &pos.x,&pos.y,&pos.z,&orient.w,&orient.x,&orient.y,&orient.z,&scale.x,&scale.y,&scale.z);
        Location location(pos, orient, Vector3f::nil(), Vector3f::nil(), 0.);
        // Read a line into filename.
        std::string filename;
        while (true) {
            char str[1024];
            str[0]='\0';
            char* getsptr = fgets(str, 1024, fp);
            if (getsptr == NULL) {
                // Read error or EOF, bail
                break;
            }
            std::string append(str);
            if (append.length() == 0) {
                // EOF
                break;
            }
            else if (append[append.length()-1]=='\n') {
                filename += append.substr(0, append.length()-1);
                break;
            }
            else {
                filename += append;
            }
        }
        if (ret < 7) { // not all options existed in the file.
            return;
        }
        if (sscanf(filename.c_str(),"{%d %f %f %f}", &mode, &density, &friction, &bounce)==4) {
            size_t i;
            for (i = 0; i < filename.length() && filename[i]!='}'; ++i) {
            }
            do {
                ++i;
            }
            while (i < filename.length() && isspace(filename[i]));
            filename = filename.substr(i);
        }
        std::string rest;
        std::string::size_type sppos=filename.find(' ');
        if (sppos != std::string::npos) {
            rest = filename.substr(sppos+1);
            filename = filename.substr(0, sppos);
        }
        if (filename=="CAMERA") {
            mCamera->resetPositionVelocity(Time::now(), location);
        }
        else if (filename=="point" || filename=="directional" || filename=="spotlight") {
            LightInfo::LightTypes lighttype = LightInfo::SPOTLIGHT;
            if (filename=="point") {
                lighttype = LightInfo::POINT;
            }
            else if (filename=="spotlight") {
                lighttype = LightInfo::SPOTLIGHT;
            }
            else if (filename=="directional") {
                lighttype = LightInfo::DIRECTIONAL;
            }
            LightInfo lightInfo;
            lightInfo.setLightType(lighttype);
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
            }
            else {
                lightInfo.mAmbientColor = Vector3f(0,0,0);
            }
            if (lightInfo.mSpecularColor.length()&&lightInfo.mPower) {
                lightInfo.mShadowColor = lightInfo.mSpecularColor*(shadowPower/lightInfo.mSpecularColor.length())/lightInfo.mPower;
            }
            else {
                lightInfo.mShadowColor = Vector3f(0,0,0);
            }
            lightInfo.mWhichFields = LightInfo::ALL;
            addLightObject(lightInfo, location);
        }
        else if (filename=="NULL") {
            SpaceObjectReference myId((SpaceID(UUID::null())),(ObjectReference(UUID::random())));

            std::tr1::shared_ptr<ProxyPositionObject> myObj(new ProxyPositionObject(this, myId));
            mObjects.insert(ObjectMap::value_type(myId, myObj));
            notify(&ProxyCreationListener::createProxy, myObj);
            myObj->resetPositionVelocity(Time::now(), location);
        }
        else if (!filename.empty()) {
            addMeshObject(Transfer::URI(filename), location, scale, mode, density, friction, bounce);
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

        int len=scenefile.size();
        if (len>=4 && scenefile[len-4]=='.' && scenefile[len-3]=='t' && scenefile[len-2]=='x' && scenefile[len-1]=='t') {
            while (!feof(fp)) {
                loadSceneObject_txt(fp);
            }
        }
        else {
            parse_csv_headings(fp);
            while (!feof(fp)) {
                //std::cout << "csv: loading scene object" << std::endl;
                loadSceneObject(fp);
            }
        }
        fclose(fp);
        return true;
    }

public:
    string scenefile;
    DemoProxyManager()
            : mCamera(new ProxyCameraObject(this, SpaceObjectReference(SpaceID(UUID::null()),ObjectReference(UUID::random())))) {
        scenefile="scene.csv";
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

    void initialize() {
        notify(&ProxyCreationListener::createProxy,mCamera);
        mCamera->attach("",0,0);

        addWebViewObject("http://sirikata.com/cgi-bin/virtualchat/irc.cgi", 400, 250);

        mCamera->resetPositionVelocity(Time::now(),
                                       Location(Vector3d(0,0,50.), Quaternion::identity(),
                                                Vector3f::nil(), Vector3f::nil(), 0.));

        if (loadSceneFile(scenefile)) {
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
        }
        else {
            ObjectMap::const_iterator iter = mObjects.find(id);
            if (iter == mObjects.end()) {
                return ProxyObjectPtr();
            }
            else {
                return (*iter).second;
            }
        }
    }
};

}
#endif
