/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystem.cpp
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
#include <sirikata/proxyobject/Platform.hpp>

#include <sirikata/core/util/Thread.hpp>

#include "OgreSystem.hpp"
#include "OgreSystemMouseHandler.hpp"
#include "OgrePlugin.hpp"
#include <sirikata/ogre/task/Event.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include "ProxyCamera.hpp"
#include "ProxyEntity.hpp"
#include <Ogre.h>
#include "CubeMap.hpp"
#include <sirikata/ogre/input/InputDevice.hpp>
#include <sirikata/ogre/input/InputEvents.hpp>
#include "OgreMeshRaytrace.hpp"

#include <stdio.h>

#include "Protocol_JSMessage.pbj.hpp"
#include <sirikata/core/util/KnownMessages.hpp>
#include <sirikata/core/util/KnownScriptTypes.hpp>

#include <sirikata/mesh/CompositeFilter.hpp>

using namespace std;

namespace Sirikata {
namespace Graphics {


OgreSystem::OgreSystem(Context* ctx)
 : OgreRenderer(ctx),
   mPrimaryCamera(NULL)
{
    increfcount();
    mCubeMap=NULL;
    mInputManager=NULL;
    mOgreOwnedRenderWindow = false;
    mRenderTarget=NULL;
    mRenderWindow = NULL;
    mSceneManager=NULL;
    mMouseHandler=NULL;
    mRayQuery=NULL;

    {
        std::vector<String> names_and_args;
        names_and_args.push_back("reduce-draw-calls"); names_and_args.push_back("");
        names_and_args.push_back("center"); names_and_args.push_back("");
        mModelFilter = new Mesh::CompositeFilter(names_and_args);
    }
}


Time OgreSystem::simTime() {
    return mContext->simTime();
}

void OgreSystem::attachCamera(const String &renderTargetName, Camera* entity) {
    OgreRenderer::attachCamera(renderTargetName, entity);

    if (renderTargetName.empty()) {
        dlPlanner->setCamera(entity);
        std::vector<String> cubeMapNames;

        std::vector<Vector3f> cubeMapOffsets;
        std::vector<float> cubeMapNearPlanes;
        cubeMapNames.push_back("ExteriorCubeMap");
        cubeMapOffsets.push_back(Vector3f(0,0,0));
        cubeMapNearPlanes.push_back(10);
        cubeMapNames.push_back("InteriorCubeMap");
        cubeMapOffsets.push_back(Vector3f(0,0,0));
        cubeMapNearPlanes.push_back(0.1);
        try {
            mCubeMap=new CubeMap(this,cubeMapNames,512,cubeMapOffsets, cubeMapNearPlanes);
        }catch (std::bad_alloc&) {
            mCubeMap=NULL;
        }
    }
}

void OgreSystem::detachCamera(Camera* entity) {
    OgreRenderer::detachCamera(entity);

    if (mPrimaryCamera == entity) {
        mPrimaryCamera = NULL;
        delete mCubeMap;
        mCubeMap = NULL;
    }
}

void OgreSystem::instantiateAllObjects(ProxyManagerPtr pman)
{
    std::vector<SpaceObjectReference> allORefs;
    pman->getAllObjectReferences(allORefs);

    for (std::vector<SpaceObjectReference>::iterator iter = allORefs.begin(); iter != allORefs.end(); ++iter)
    {
        //instantiate each object in graphics system separately.
        ProxyObjectPtr toAdd = pman->getProxyObject(*iter);
        onCreateProxy(toAdd);
    }
}

bool OgreSystem::initialize(VWObjectPtr viewer, const SpaceObjectReference& presenceid, const String& options) {
    if(!OgreRenderer::initialize(options)) return false;

    mViewer = viewer;
    mPresenceID = presenceid;

    ProxyManagerPtr proxyManager = mViewer->presence(presenceid);
    mViewer->addListener((SessionEventListener*)this);
    proxyManager->addListener(this);

    //initialize the Resource Download Planner
    dlPlanner = new SAngleDownloadPlanner(mContext);

    allocMouseHandler();

    //finish instantiation here
    instantiateAllObjects(proxyManager);

    return true;
}

void OgreSystem::windowResized(Ogre::RenderWindow *rw) {
    SILOG(ogre,insane,"Ogre resized window: " << rw->getWidth() << "x" << rw->getHeight());
    if (mPrimaryCamera)
        mPrimaryCamera->windowResized();
    mMouseHandler->windowResized(rw->getWidth(), rw->getHeight());
}

OgreSystem::~OgreSystem() {
    decrefcount();
    destroyMouseHandler();
}

static void KillWebView(OgreSystem*ogreSystem,ProxyObjectPtr p) {
    SILOG(ogre,detailed,"Killing WebView!");
    p->getProxyManager()->destroyObject(p);
}

void OgreSystem::onCreateProxy(ProxyObjectPtr p)
{
    bool created = false;

    ProxyEntity* mesh = new ProxyEntity(this,p);
    dlPlanner->addNewObject(p,mesh);

    bool is_viewer = (p->getObjectReference() == mPresenceID);
    if (is_viewer)
    {
        assert(mPrimaryCamera == NULL);
        ProxyCamera* cam = new ProxyCamera(this, mesh);
        cam->initialize();
        cam->attach("", 0, 0, mBackgroundColor);
        attachCamera("", cam);
        // Only store as primary camera now because doing it earlier loops back
        // to detachCamera, which then *removes* it as primary camera. It does
        // this because attach does a "normal" cleanup procedure before attaching.
        mPrimaryCamera = cam;
    }
}

void OgreSystem::onDestroyProxy(ProxyObjectPtr p)
{
    dlPlanner->removeObject(p);
}


struct RayTraceResult {
    Ogre::Real mDistance;
    Ogre::MovableObject *mMovableObject;
    IntersectResult mResult;
    int mSubMesh;
    RayTraceResult() { mDistance=3.0e38f;mMovableObject=NULL;mSubMesh=-1;}
    RayTraceResult(Ogre::Real distance,
                   Ogre::MovableObject *moveableObject) {
        mDistance=distance;
        mMovableObject=moveableObject;
    }
    bool operator<(const RayTraceResult&other)const {
        if (mDistance==other.mDistance) {
            return mMovableObject<other.mMovableObject;
        }
        return mDistance<other.mDistance;
    }
    bool operator==(const RayTraceResult&other)const {
        return mDistance==other.mDistance&&mMovableObject==other.mMovableObject;
    }
};

Entity* OgreSystem::rayTrace(const Vector3d &position,
                             const Vector3f &direction,
                             int&resultCount,
                             double &returnResult,
                             Vector3f&returnNormal,
                             int&subent,
                             int which) const{
    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    return internalRayTrace(traceFrom,false,resultCount,returnResult,returnNormal, subent,NULL,false,which);
}
Entity* OgreSystem::rayTraceAABB(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     int&subent,
                     int which) const{
    Vector3f normal;
    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    return internalRayTrace(traceFrom,true,resultCount,returnResult,normal,subent,NULL,false,which);
}

ProxyEntity* OgreSystem::getEntity(const SpaceObjectReference &proxyId) const {
    SceneEntitiesMap::const_iterator iter = mSceneEntities.find(proxyId.toString());
    if (iter != mSceneEntities.end()) {
        Entity* ent = (*iter).second;
        return static_cast<ProxyEntity*>(ent);
    } else {
        return NULL;
    }
}
ProxyEntity* OgreSystem::getEntity(const ProxyObjectPtr &proxy) const {
    return getEntity(proxy->getObjectReference());
}

bool OgreSystem::queryRay(const Vector3d&position,
                          const Vector3f&direction,
                          const double maxDistance,
                          ProxyObjectPtr ignore,
                          double &returnDistance,
                          Vector3f &returnNormal,
                          SpaceObjectReference &returnName) {
    int resultCount=0;
    int subent;
    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    ProxyEntity * retval=internalRayTrace(traceFrom,false,resultCount,returnDistance,returnNormal,subent,NULL,false,0);
    if (retval != NULL) {
        returnName= retval->getProxy().getObjectReference();
        return true;
    }
    return false;
}
ProxyEntity *OgreSystem::internalRayTrace(const Ogre::Ray &traceFrom, bool aabbOnly,int&resultCount,double &returnresult, Vector3f&returnNormal, int& returnSubMesh, IntersectResult *intersectResult, bool texcoord, int which) const {
    Ogre::RaySceneQuery* mRayQuery;
    mRayQuery = mSceneManager->createRayQuery(Ogre::Ray(), Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK);
    mRayQuery->setRay(traceFrom);
    mRayQuery->setSortByDistance(aabbOnly);
    const Ogre::RaySceneQueryResult& resultList = mRayQuery->execute();

    ProxyEntity *toReturn = NULL;
    returnresult = 0;
    int count = 0;
    std::vector<RayTraceResult> fineGrainedResults;
    for (Ogre::RaySceneQueryResult::const_iterator iter  = resultList.begin();
         iter != resultList.end(); ++iter) {
        const Ogre::RaySceneQueryResultEntry &result = (*iter);
        Ogre::Entity *foundEntity = dynamic_cast<Ogre::Entity*>(result.movable);
        if (!foundEntity) continue;
        ProxyEntity *ourEntity = ProxyEntity::fromMovableObject(result.movable);
        if (!ourEntity) continue;
        if (ourEntity->id() == mPresenceID.toString()) continue;

        RayTraceResult rtr(result.distance,result.movable);
        bool passed=aabbOnly&&result.distance > 0;
        if (aabbOnly==false) {
            rtr.mDistance=3.0e38f;
            Ogre::Ray meshRay = OgreMesh::transformRay(ourEntity->getSceneNode(), traceFrom);
            Ogre::Mesh *mesh = foundEntity->getMesh().get();
            uint16 numSubMeshes = mesh->getNumSubMeshes();
            std::vector<TriVertex> sharedVertices;
            for (uint16 ndx = 0; ndx < numSubMeshes; ndx++) {
                Ogre::SubMesh *submesh = mesh->getSubMesh(ndx);
                OgreMesh ogreMesh(submesh, texcoord, sharedVertices);
                IntersectResult intRes;
                ogreMesh.intersect(ourEntity->getSceneNode(), meshRay, intRes);
                if (intRes.intersected && intRes.distance < rtr.mDistance && intRes.distance > 0 ) {
                    rtr.mResult = intRes;
                    rtr.mMovableObject = result.movable;
                    rtr.mDistance=intRes.distance;
                    rtr.mSubMesh=ndx;
                    passed=true;
                }
            }
        }
        if (passed) {
            fineGrainedResults.push_back(rtr);
            ++count;
        }
    }
    if (!aabbOnly) {
        std::sort(fineGrainedResults.begin(),fineGrainedResults.end());
    }
    if (count > 0) {
        which %= count;
        if (which < 0) {
            which += count;
        }
        for (std::vector<RayTraceResult>::const_iterator iter  = fineGrainedResults.begin()+which,iterEnd=fineGrainedResults.end();
             iter != iterEnd; ++iter) {
            const RayTraceResult &result = (*iter);
            ProxyEntity *foundEntity = ProxyEntity::fromMovableObject(result.mMovableObject);
            if (foundEntity) {
                toReturn = foundEntity;
                returnresult = result.mDistance;
                returnNormal=result.mResult.normal;
                returnSubMesh=result.mSubMesh;
                if(intersectResult)*intersectResult=result.mResult;
                break;
            }
        }
    }
    mRayQuery->clearResults();
    if (mRayQuery) {
        mSceneManager->destroyQuery(mRayQuery);
    }
    resultCount=count;
    return toReturn;
}

void OgreSystem::poll(){
    OgreRenderer::poll();
    Task::LocalTime curFrameTime(Task::LocalTime::now());
    tickInputHandler(curFrameTime);
}

bool OgreSystem::renderOneFrame(Task::LocalTime t, Duration frameTime) {
    bool cont = OgreRenderer::renderOneFrame(t, frameTime);

    if(WebViewManager::getSingletonPtr())
    {
        // HACK: WebViewManager is static, but points to a RenderTarget! If OgreRenderer dies, we will crash.
        static bool webViewInitialized = false;
        if(!webViewInitialized) {
            if (mPrimaryCamera) {
                WebViewManager::getSingleton().setDefaultViewport(mPrimaryCamera->getViewport());
                webViewInitialized = true;
            }
            // else, keep waiting for a camera to appear (may require connecting to a space).
        }
        if (webViewInitialized) {
            WebViewManager::getSingleton().Update();
        }
    }

    return cont;
}

void OgreSystem::preFrame(Task::LocalTime currentTime, Duration frameTime) {
    OgreRenderer::preFrame(currentTime, frameTime);
    std::list<Entity*>::iterator iter;
    for (iter = mMovingEntities.begin(); iter != mMovingEntities.end();) {
        ProxyEntity *current = static_cast<ProxyEntity*>(*iter);
        ++iter;
        SpaceID space(current->getProxy().getObjectReference().space());
        Time cur_time = simTime();
        current->extrapolateLocation(cur_time);
    }
}

void OgreSystem::postFrame(Task::LocalTime current, Duration frameTime) {
    OgreRenderer::postFrame(current, frameTime);
    Ogre::FrameEvent evt;
    evt.timeSinceLastEvent=frameTime.toMicroseconds()*1000000.;
    evt.timeSinceLastFrame=frameTime.toMicroseconds()*1000000.;
    if (mCubeMap) {
        mCubeMap->frameEnded(evt);
    }
}

void OgreSystem::screenshot(const String& filename) {
    if (mRenderTarget != NULL)
        mRenderTarget->writeContentsToFile(filename);
}

// ConnectionEventListener Interface
void OgreSystem::onConnected(const Network::Address& addr)
{
}

void OgreSystem::onDisconnected(const Network::Address& addr, bool requested, const String& reason) {
    if (!requested) {
        SILOG(ogre,fatal,"Got disconnected from space server: " << reason);
        quit(); // FIXME
    }
    else
        SILOG(ogre,warn,"Disconnected from space server.");
}

void OgreSystem::onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name) {
    mViewer->removeListener((SessionEventListener*)this);
    SILOG(ogre,info,"Got disconnected from space server.");
    mMouseHandler->alert("Disconnected", "Lost connection to space server...");
}


void OgreSystem::allocMouseHandler() {
    mMouseHandler = new OgreSystemMouseHandler(this);
    mMouseHandler->ensureUI();
}
void OgreSystem::destroyMouseHandler() {
    if (mMouseHandler) {
        delete mMouseHandler;
    }
}

void OgreSystem::tickInputHandler(const Task::LocalTime& t) const {
    if (mMouseHandler != NULL)
        mMouseHandler->tick(t);
}

boost::any OgreSystem::invoke(vector<boost::any>& params)
{
    // Decode the command. First argument is the "function name"
    if (params.empty() || !Invokable::anyIsString(params[0]))
        return NULL;

    string name = Invokable::anyAsString(params[0]);
    SILOG(ogre,detailed,"Invoking the function " << name);

    if(name == "createWindow")
        return createWindow(params);
    else if(name == "createWindowFile")
        return createWindowFile(params);
    else if(name == "addModuleToUI")
        return addModuleToUI(params);
    else if(name == "createWindowHTML")
        return createWindowHTML(params);
    else if(name == "setInputHandler")
        return setInputHandler(params);
    else if(name == "quit")
        quit();
    else if (name == "suspend")
        suspend();
    else if (name == "toggleSuspend")
        toggleSuspend();
    else if (name == "resume")
        resume();
    else if (name == "screenshot")
        screenshot("screenshot.png");
    else if (name == "pick")
        return pick(params);
    else if (name == "bbox")
        return bbox(params);
    else if (name == "initScript")
        return initScript(params);
    else if (name == "camera")
        return getCamera(params);
    else if (name == "setCameraMode")
        return setCameraMode(params);
    else if (name == "setCameraOffset")
        return setCameraOffset(params);
    else {
        SILOG(ogre, warn, "Function " << name << " was invoked but this function was not found.");
    }

    return boost::any();
}

boost::any OgreSystem::createWindow(const String& window_name, bool is_html, bool is_file, String content, uint32 width, uint32 height) {
    WebViewManager* wvManager = WebViewManager::getSingletonPtr();
    WebView* ui_wv = wvManager->getWebView(window_name);
    if(!ui_wv)
    {
        ui_wv = wvManager->createWebView(window_name, window_name, width, height, OverlayPosition(RP_TOPLEFT));
        if (is_html)
            ui_wv->loadHTML(content);
        else if (is_file)
            ui_wv->loadFile(content);
        else
            ui_wv->loadURL(content);
    }
    Invokable* inn = ui_wv;
    return Invokable::asAny(inn);
}

boost::any OgreSystem::createWindow(vector<boost::any>& params) {
    // Create a window using the specified url
    if (params.size() < 3) return NULL;
    if (!Invokable::anyIsString(params[1]) || !Invokable::anyIsString(params[2])) return NULL;

    String window_name = Invokable::anyAsString(params[1]);
    String html_url = Invokable::anyAsString(params[2]);
    uint32 width = (params.size() > 3 && Invokable::anyIsNumeric(params[3])) ? Invokable::anyAsNumeric(params[3]) : 300;
    uint32 height = (params.size() > 4 && Invokable::anyIsNumeric(params[4])) ? Invokable::anyAsNumeric(params[4]) : 300;
    return createWindow(window_name, false, false, html_url, width, height);
}

boost::any OgreSystem::createWindowFile(vector<boost::any>& params) {
    // Create a window using the specified url
    if (params.size() < 3) return NULL;
    if (!Invokable::anyIsString(params[1]) || !Invokable::anyIsString(params[2])) return NULL;

    String window_name = Invokable::anyAsString(params[1]);
    String html_url = Invokable::anyAsString(params[2]);
    uint32 width = (params.size() > 3 && Invokable::anyIsNumeric(params[3])) ? Invokable::anyAsNumeric(params[3]) : 300;
    uint32 height = (params.size() > 4 && Invokable::anyIsNumeric(params[4])) ? Invokable::anyAsNumeric(params[4]) : 300;

    return createWindow(window_name, false, true, html_url, width, height);
}

boost::any OgreSystem::addModuleToUI(std::vector<boost::any>& params) {
    if (params.size() != 3) return NULL;
    if (!anyIsString(params[1]) || !anyIsString(params[2])) return NULL;

    String window_name = anyAsString(params[1]);
    String html_url = anyAsString(params[2]);

    if (!mMouseHandler) return NULL;

    //This is disabled and we put these directly in the ui.html
    //script currently because evaluateJS may execute before the page
    //finishes loading, resulting in loadModule not being defined
    //yet. This could be considered either our problem or a problem
    //with Berkelium.
    //mMouseHandler->mUIWidgetView->evaluateJS("loadModule('" + html_url + "')");
    Invokable* inn = mMouseHandler->mUIWidgetView;
    return Invokable::asAny(inn);
}

boost::any OgreSystem::createWindowHTML(vector<boost::any>& params) {
    // Create a window using the specified HTML content
    if (params.size() < 3) return NULL;
    if (!Invokable::anyIsString(params[1]) || !Invokable::anyIsString(params[2])) return NULL;

    String window_name = Invokable::anyAsString(params[1]);
    String html_script = Invokable::anyAsString(params[2]);
    uint32 width = (params.size() > 3 && Invokable::anyIsNumeric(params[3])) ? Invokable::anyAsNumeric(params[3]) : 300;
    uint32 height = (params.size() > 4 && Invokable::anyIsNumeric(params[4])) ? Invokable::anyAsNumeric(params[4]) : 300;

    return createWindow(window_name, true, false, html_script, width, height);
}

boost::any OgreSystem::setInputHandler(vector<boost::any>& params) {
    if (params.size() < 2) return NULL;
    if (!Invokable::anyIsInvokable(params[1])) return NULL;

    Invokable* handler = Invokable::anyAsInvokable(params[1]);
    mMouseHandler->setDelegate(handler);
    return boost::any();
}


boost::any OgreSystem::pick(vector<boost::any>& params) {
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsNumeric(params[1])) return boost::any();
    if (!Invokable::anyIsNumeric(params[2])) return boost::any();

    float x = Invokable::anyAsNumeric(params[1]);
    float y = Invokable::anyAsNumeric(params[2]);
    SpaceObjectReference result = mMouseHandler->pick(Vector2f(x,y), 1);
    return Invokable::asAny(result);
}


boost::any OgreSystem::bbox(vector<boost::any>& params) {
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsObject(params[1])) return boost::any();
    if (!Invokable::anyIsBoolean(params[2])) return boost::any();

    SpaceObjectReference objid = Invokable::anyAsObject(params[1]);
    bool setting = Invokable::anyAsBoolean(params[2]);

    if (mSceneEntities.find(objid.toString()) == mSceneEntities.end()) return boost::any();
    Entity* ent = mSceneEntities.find(objid.toString())->second;
    ent->setSelected(setting);

    return boost::any();
}

/**
   This function sends out a message on KnownServices port
   LISTEN_FOR_SCRIPT_BEGIN to the HostedObject.  Presumably, the hosted
   object receives the message and attaches a JSObjectScript to the
   HostedObject.

   FIXME this should go away in favor of a default script loading instead.
*/
boost::any OgreSystem::initScript(vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsObject(params[1])) return boost::any();

    SpaceObjectReference objid = Invokable::anyAsObject(params[1]);

    ProxyObjectPtr obj = mViewer->presence(mPresenceID)->getProxyObject(objid);

    Sirikata::JS::Protocol::ScriptingInit init_script;

    // Filter out the script type from rest of args
    //String script_type = "js"; // FIXME how to decide this?
    init_script.set_script(ScriptTypes::JS_SCRIPT_TYPE);
    init_script.set_messager(KnownMessages::INIT_SCRIPT);
    String serializedInitScript;
    init_script.SerializeToString(&serializedInitScript);

    obj->sendMessage(
        Services::LISTEN_FOR_SCRIPT_BEGIN,
        MemoryReference(serializedInitScript.data(), serializedInitScript.length())
    );

    return boost::any();
}

boost::any OgreSystem::getCamera(vector<boost::any>& params) {
    if (mPrimaryCamera == NULL) return boost::any();

    Ogre::Camera* cam = mPrimaryCamera->getOgreCamera();

    // Just returns a "struct" with basic camera info
    Invokable::Dict camera_info;

    float32 aspect = cam->getAspectRatio();
    camera_info["aspectRatio"] = Invokable::asAny(aspect);

    float32 fovy = cam->getFOVy().valueRadians();
    float32 fovx = fovy * aspect;
    Invokable::Dict camera_fov;
    camera_fov["x"] = Invokable::asAny(fovx);
    camera_fov["y"] = Invokable::asAny(fovy);
    camera_info["fov"] = Invokable::asAny(camera_fov);

    Invokable::Dict camera_pos;
    Vector3d pos = mPrimaryCamera->getPosition();
    camera_pos["x"] = Invokable::asAny(pos.x);
    camera_pos["y"] = Invokable::asAny(pos.y);
    camera_pos["z"] = Invokable::asAny(pos.z);
    camera_info["position"] = Invokable::asAny(camera_pos);

    return Invokable::asAny(camera_info);
}

boost::any OgreSystem::setCameraMode(vector<boost::any>& params) {
    if (mPrimaryCamera == NULL) return boost::any();
    if (params.size() < 2 || !Invokable::anyIsString(params[1])) return boost::any();

    String mode = Invokable::anyAsString(params[1]);
    if (mode == "first")
        mPrimaryCamera->setMode(Camera::FirstPerson);
    else if (mode == "third")
        mPrimaryCamera->setMode(Camera::ThirdPerson);

    return boost::any();
}

boost::any OgreSystem::setCameraOffset(vector<boost::any>& params) {
    if (mPrimaryCamera == NULL) return boost::any();
    if (params.size() < 4) return boost::any();
    if (!Invokable::anyIsNumeric(params[1]) || !Invokable::anyIsNumeric(params[2]) || !Invokable::anyIsNumeric(params[3])) return boost::any();
    Vector3d offset( Invokable::anyAsNumeric(params[1]), Invokable::anyAsNumeric(params[2]), Invokable::anyAsNumeric(params[3]) );
    mPrimaryCamera->setOffset(offset);
    return boost::any();
}

}
}
