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
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include "ProxyEntity.hpp"
#include <Ogre.h>
#include "CubeMap.hpp"
#include <sirikata/ogre/input/InputDevice.hpp>
#include <sirikata/ogre/input/InputEvents.hpp>
#include "OgreMeshRaytrace.hpp"
#include <sirikata/ogre/Camera.hpp>
#include <stdio.h>
#include <sirikata/ogre/ResourceDownloadPlanner.hpp>
#include <sirikata/core/network/Address.hpp>


using namespace std;

namespace Sirikata {
namespace Graphics {



OgreSystem::OgreSystem(Context* ctx,Network::IOStrandPtr sStrand)
 : OgreRenderer(ctx,sStrand),
   mConnectionEventProvider(NULL),
   mOnReadyCallback(NULL),
   mOnResetReadyCallback(NULL),
   mPrimaryCamera(NULL),
   mOverlayCamera(NULL),
   mReady(false),
   initialized(false)
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
}


Time OgreSystem::simTime() {
    return mContext->simTime();
}

void OgreSystem::attachCamera(const String &renderTargetName, Camera* entity) {
    OgreRenderer::attachCamera(renderTargetName, entity);

    if (renderTargetName.empty()) {
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


//Be careful about calling this function in too many other places:
//its use of iOnCreateProxy with true passed in is tricky.
void OgreSystem::instantiateAllObjects(ProxyManagerPtr pman)
{
    std::vector<SpaceObjectReference> allORefs;
    pman->getAllObjectReferences(allORefs);

    for (std::vector<SpaceObjectReference>::iterator iter = allORefs.begin(); iter != allORefs.end(); ++iter)
    {
        //instantiate each object in graphics system separately.
      ProxyObjectPtr toAdd = pman->getProxyObject(*iter);
        iOnCreateProxy(livenessToken(),toAdd,true);
    }
}

namespace {
Transfer::DenseDataPtr read_file(const String& filename)
{
    ifstream myfile;
    myfile.open (filename.c_str());
    if (!myfile.is_open()) return Transfer::DenseDataPtr();

    myfile.seekg(0, ios::end);
    int length = myfile.tellg();
    myfile.seekg(0, ios::beg);

    Transfer::MutableDenseDataPtr output(new Transfer::DenseData(Transfer::Range(true)));
    output->setLength(length, true);

    // read data as a block:
    myfile.read((char*)output->writableData(), length);
    myfile.close();

    return output;
}
}

bool OgreSystem::initialize(ConnectionEventProvider* cevtprovider, VWObjectPtr viewer, const SpaceObjectReference& presenceid, const String& options) {
    if(!OgreRenderer::initialize(options)) return false;


    mConnectionEventProvider = cevtprovider;
    mConnectionEventProvider->addListener(this);

    mViewer = viewer;
    mPresenceID = presenceid;

    ProxyManagerPtr proxyManager = mViewer->presence(presenceid);
    mViewer->addListener((SessionEventListener*)this);
    proxyManager->addListener(this);

    allocMouseHandler();

    // The default mesh is just loaded from a known local file
    using namespace boost::filesystem;
    String cube_path = (path(mResourcesDir) / "cube.dae").string();
    Transfer::DenseDataPtr cube_data = read_file(cube_path);
    Transfer::Fingerprint hash = Transfer::Fingerprint::computeDigest(cube_data->data(), cube_data->size());
    Transfer::RemoteFileMetadata fakeMetadata(hash, Transfer::URI("file:///fake.dae"), cube_data->length(), Transfer::ChunkList(), Transfer::FileHeaders());
    mDefaultMesh = parseMeshWorkSync(fakeMetadata, Transfer::Fingerprint::null(), cube_data, false);

    //finish instantiation here
    instantiateAllObjects(proxyManager);
    mMouseHandler->mUIWidgetView->setReadyCallback( std::tr1::bind(&OgreSystem::handleUIReady, this) );
    mMouseHandler->mUIWidgetView->setResetReadyCallback( std::tr1::bind(&OgreSystem::handleUIResetReady, this) );
    mMouseHandler->mUIWidgetView->setUpdateViewportCallback( std::tr1::bind(&OgreSystem::handleUpdateUIViewport, this, _1, _2, _3, _4) );

    vector<boost::any> temp;
    setMat(temp);

    Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create("x-axis-pos-mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0.3, 0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0.3, 0, 0);
    mat->setPointSize(5);

    mat = Ogre::MaterialManager::getSingleton().create("y-axis-pos-mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0, 0, 0.3, 0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0, 0, 0.3);

    mat = Ogre::MaterialManager::getSingleton().create("z-axis-pos-mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0, 0.3, 0, 0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0, 0.3, 0);

    mat = Ogre::MaterialManager::getSingleton().create("x-axis-neg-mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0.5, 0.25, 0.25, 0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0.5, 0.25, 0.25);

    mat = Ogre::MaterialManager::getSingleton().create("y-axis-neg-mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0.25, 0.25, 0.5, 0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0.25, 0.25, 0.5);

    mat = Ogre::MaterialManager::getSingleton().create("z-axis-neg-mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mat->setReceiveShadows(false);
    mat->getTechnique(0)->setLightingEnabled(true);
    mat->getTechnique(0)->getPass(0)->setDiffuse(0.25, 0.5, 0.25, 0);
    mat->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
    mat->getTechnique(0)->getPass(0)->setSelfIllumination(0.25, 0.5, 0.25);

    initialized = true;


    // If we already set the skybox before initialization, make sure we load it.
    if (mSkybox && (*mSkybox))
        mSkybox->load(mSceneManager, mResourceLoader, mTransferPool);

    return true;
}


void OgreSystem::handleUIReady()
{
    simStrand->post(
        std::tr1::bind(&OgreSystem::iHandleUIReady,this,
            livenessToken()),
        "OgreSystem::iHandleUIReady"
    );
}


void OgreSystem::iHandleUIReady(Liveness::Token osAlive)
{
    if (!osAlive)
        return;

    // Currently the only blocker for being ready is that the UI loaded. If we
    // end up with more, we may need to make this just set a flag and then check
    // if all conditions are met.
    if (mOnReadyCallback != NULL) mOnReadyCallback->invoke();
    mMouseHandler->uiReady();
    mReady= true;
}

void OgreSystem::handleUIResetReady() {
    // Currently the only blocker for being ready is that the UI loaded. If we
    // end up with more, we may need to make this just set a flag and then check
    // if all conditions are met.
    if (mOnResetReadyCallback != NULL) mOnResetReadyCallback->invoke();
    mMouseHandler->uiReady(); // Probably not really necessary since
                              // it's been called once already?
}

void OgreSystem::handleUpdateUIViewport(int32 left, int32 top, int32 right, int32 bottom) {
    if (mPrimaryCamera)
        mPrimaryCamera->setViewportDimensions(left, top, right, bottom);
}

void OgreSystem::windowResized(Ogre::RenderWindow *rw) {
    OgreRenderer::windowResized(rw);
    mMouseHandler->windowResized(rw->getWidth(), rw->getHeight());
}

bool OgreSystem::translateToDisplayViewport(float32 x, float32 y, float32* ox, float32* oy) {
    // x and y come in as [-1, 1], get it to [0,1] to match the normal viewport
    x = (x + 1.0f)/2.0f;
    y = (y + 1.0f)/2.0f;
    // Subtract out the offset to the subframe
    x -= mPrimaryCamera->getViewport()->getLeft();
    y -= mPrimaryCamera->getViewport()->getTop();
    // And scale to make the new [0-1] range fit in the correct sized
    // box
    x /= mPrimaryCamera->getViewport()->getWidth();
    y /= mPrimaryCamera->getViewport()->getHeight();
    // And convert back to [-1, 1] as we set the output values
    *ox = x * 2.f - 1.f;
    *oy = y * 2.f - 1.f;
    return !(*ox < -1.f || *ox > 1.f || *oy < -1.f || *ox > 1.f);
}

OgreSystem::~OgreSystem() {
    Liveness::letDie();
    decrefcount();
    destroyMouseHandler();
}

void OgreSystem::stop()
{
    //note: leaving command to mViewer here so that access to mViewer is still
    //within mainStrand
    if (mViewer) {
        ProxyManagerPtr proxyManager = mViewer->presence(mPresenceID);
        if (proxyManager) // May have been disconnected
            proxyManager->removeListener(this);
    }

    if (mConnectionEventProvider) {
        mConnectionEventProvider->removeListener(this);
        mConnectionEventProvider = NULL;
    }

    OgreRenderer::stop();
}

void OgreSystem::onCreateProxy(ProxyObjectPtr p)
{
    simStrand->post(
        std::tr1::bind(&OgreSystem::iOnCreateProxy,this,
            livenessToken(),p, false),
        "OgreSystem::iOnCreateProxy"
    );
}



void OgreSystem::iOnCreateProxy(
    Liveness::Token osAlive, ProxyObjectPtr p, bool inInit)
{
    if (!osAlive) return;
    Liveness::Lock locked(osAlive);
    if (!locked)
    {
        SILOG(ogre,error,"Received onCreateProxy after having deleted ogre system");
        return;
    }

    if (stopped)
    {
        SILOG(ogre,warn,"Received onCreateProxy after having stopped");
        return;
    }

    if ( !p->isValid() ) {
      return;
    }

    //busy wait until initialized.  note that the initialization code actually
    //calls iOnCreateProxy itself (through instantiateAllObjects).  If the
    //inInit param is true then we know that the initialization code called this
    //function, and we should not busy wait.
    while (!inInit && !initialized)
    {}

    bool created = false;

    ProxyEntity* mesh = NULL;
    if (mEntityMap.find(p->getObjectReference()) != mEntityMap.end())
        mesh = mEntityMap[p->getObjectReference()];
    if (mesh == NULL)
        mesh = new ProxyEntity(this,p);
    mesh->initializeToProxy(p);
    mEntityMap[p->getObjectReference()] = mesh;
    // Force validation. In the case of existing ProxyObjects, this
    // should trigger the download + display process
    mesh->validated(p);

    bool is_viewer = (p->getObjectReference() == mPresenceID);
    if (is_viewer)
    {
        if (mPrimaryCamera == NULL) {
            assert(mOverlayCamera == NULL);
            mOverlayCamera = new Camera(this, getOverlaySceneManager(), "overlay_camera");
            mOverlayCamera->attach("", 0, 0, Vector4f(0, 0, 0, 0), 1);

            Camera* cam = new Camera(this, getSceneManager(), String("Camera:") + mesh->id());
            cam->attach("", 0, 0, mBackgroundColor, 0);
            attachCamera("", cam);
            cam->setPosition(mesh->getOgrePosition());
            // Only store as primary camera now because doing it earlier loops back
            // to detachCamera, which then *removes* it as primary camera. It does
            // this because attach does a "normal" cleanup procedure before attaching.
            mPrimaryCamera = cam;
        }
    }
}

void OgreSystem::entityDestroyed(ProxyEntity* p) {
    mEntityMap.erase(p->getProxyPtr()->getObjectReference());
    // No deletion, this is invoked as the ProxyEntity is self-destructing.
}

void OgreSystem::onDestroyProxy(ProxyObjectPtr p)
{
    simStrand->post(
        std::tr1::bind(&OgreSystem::iOnDestroyProxy,this,
            livenessToken(), p),
        "OgreSystem::iOnDestroyProxy"
    );
}

void OgreSystem::iOnDestroyProxy(
    Liveness::Token osAlive,ProxyObjectPtr p)
{
    // We don't clean anything up here since the entity could be
    // masking an addition/removal. Instead, we just wait and let the
    // ProxyEntity tell us when it's destroyed.
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
    int which, SpaceObjectReference ignore) const{

    Ogre::Ray traceFrom(toOgre(position, getOffset()), toOgre(direction));
    return internalRayTrace(traceFrom,false,resultCount,returnResult,returnNormal, subent,NULL,false,which,ignore);
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
    ProxyEntity * retval=internalRayTrace(traceFrom,false,resultCount,returnDistance,returnNormal,subent,NULL,false,0,mPresenceID);
    if (retval != NULL) {
        returnName= retval->getProxy().getObjectReference();
        return true;
    }
    return false;
}
ProxyEntity *OgreSystem::internalRayTrace(const Ogre::Ray &traceFrom, bool aabbOnly,int&resultCount,double &returnresult, Vector3f&returnNormal, int& returnSubMesh, IntersectResult *intersectResult, bool texcoord, int which, SpaceObjectReference ignore) const {
    Ogre::RaySceneQuery* mRayQuery;
    mRayQuery = mSceneManager->createRayQuery(Ogre::Ray());
    mRayQuery->setRay(traceFrom);
    mRayQuery->setSortByDistance(aabbOnly);
    mRayQuery->setQueryTypeMask(Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK | Ogre::SceneManager::ENTITY_TYPE_MASK | Ogre::SceneManager::FX_TYPE_MASK);
    const Ogre::RaySceneQueryResult& resultList = mRayQuery->execute();

    ProxyEntity *toReturn = NULL;
    returnresult = 0;
    int count = 0;
    std::vector<RayTraceResult> fineGrainedResults;
    for (Ogre::RaySceneQueryResult::const_iterator iter  = resultList.begin();
         iter != resultList.end(); ++iter) {
        const Ogre::RaySceneQueryResultEntry &result = (*iter);

        Ogre::BillboardSet* foundBillboard = dynamic_cast<Ogre::BillboardSet*>(result.movable);
        Ogre::Entity *foundEntity = dynamic_cast<Ogre::Entity*>(result.movable);

        if (foundEntity != NULL) {
            ProxyEntity *ourEntity = ProxyEntity::fromMovableObject(result.movable);
            if (!ourEntity) continue;
            if (ourEntity->id() == ignore.toString()) continue;
        }

        RayTraceResult rtr(result.distance,result.movable);
        bool passed=aabbOnly&&result.distance > 0;
        if (aabbOnly==false) {
            if (foundEntity != NULL) {
                rtr.mDistance=3.0e38f;
                ProxyEntity *ourEntity = ProxyEntity::fromMovableObject(result.movable);
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
            else if (foundBillboard != NULL) {
                // FIXME real check against the object if checking bounding box
                // isn't sufficient (but maybe it is for billboards since ogre
                // forces them to face the user? but they also have an
                // orientation? but the billboard set seems to let you control
                // orientation? I don't know...)
                // FIXME fake distances
                float32 dist = (foundBillboard->getWorldBoundingSphere().getCenter() - traceFrom.getOrigin()).length();
                IntersectResult intRes;
                intRes.intersected = true;
                intRes.distance = dist;
                intRes.normal = fromOgre(-traceFrom.getDirection().normalisedCopy());
                // No triangle or uv...
                rtr.mResult = intRes;
                rtr.mMovableObject = foundBillboard;
                rtr.mDistance = dist;
                rtr.mSubMesh = -1;
                passed = true;
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
            if (mOverlayCamera) {
                WebViewManager::getSingleton().setDefaultViewport(mOverlayCamera->getViewport());
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


// ConnectionEventListener Interface
void OgreSystem::onConnected(const Network::Address& addr)
{
}


void OgreSystem::onDisconnected(const Network::Address& addr, bool requested, const String& reason)
{
    //ugh!  This is an ugly function.
    simStrand->post(
        std::tr1::bind(
            (&OgreSystem::iOnNetworkDisconnected),this,
            livenessToken(), addr,requested,reason),
        "OgreSystem::iOnNetworkDisconnected"
    );
}

void OgreSystem::iOnNetworkDisconnected(
    Liveness::Token osAlive,const Network::Address& addr,
    bool requested, const String& reason)
{
    if (!osAlive) return;
    Liveness::Lock locked(osAlive);
    if (!locked)
    {
        SILOG(ogre,error,"Received disconnect after having deleted ogre system");
        return;
    }

    if (stopped)
    {
        SILOG(ogre,error,"Received iOnDisconnecte after having stopped ogre system");
        return;
    }
    //don't want to disconnect before we were done connecting.
    while (!initialized){}


    if (!requested) {
        SILOG(ogre,fatal,"Got disconnected from space server: " << reason);
        quit(); // FIXME
    }
    else
        SILOG(ogre,warn,"Disconnected from space server.");
}

void OgreSystem::onDisconnected(
    SessionEventProviderPtr from, const SpaceObjectReference& name)
{
    simStrand->post(
        std::tr1::bind(
            &OgreSystem::iOnSessionDisconnected,this,
            livenessToken(),from,name),
        "OgreSystem::iOnSessionDisconnected"
    );
}

void OgreSystem::iOnSessionDisconnected(
    Liveness::Token osAlive, SessionEventProviderPtr from,
    const SpaceObjectReference& name)
{
    if (!osAlive) return;
    Liveness::Lock locked(osAlive);
    if (!locked)
    {
        SILOG(ogre,error,"Received session disconnect after having deleted ogre system");
        return;
    }

    while(!initialized){}

    //don't do anything if it's just one of the main presence's siblings that
    //was disconnected.
    if (name != mPresenceID)
        return;

    if (stopped)
    {
        SILOG(ogre,error,"Received iOnDisconnecte after having stopped ogre system");
        return;
    }

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
        return boost::any();

    string name = Invokable::anyAsString(params[0]);
    SILOG(ogre,detailed,"Invoking the function " << name);


    if(name == "onReady")
        return setOnReady(params);
    else if(name == "evalInUI")
        return evalInUI(params);
    else if(name == "createWindow")
        return createWindow(params);
    else if(name == "createWindowFile")
        return createWindowFile(params);
    else if(name == "addModuleToUI")
        return addModuleToUI(params);
    else if(name == "addTextModuleToUI")
        return addTextModuleToUI(params);
    else if(name == "createWindowHTML")
        return createWindowHTML(params);
    else if(name == "addInputHandler")
        return addInputHandler(params);
    else if (name == "removeInputHandler")
        return removeInputHandler(params);
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
    else if (name == "axis")
        return axis(params);
    else if (name == "world2Screen")
        return world2Screen(params);
    else if (name == "newDrawing")
        return newDrawing(params);
    else if (name == "setMat")
        return setMat(params);
    else if (name == "setInheritOrient")
        return setInheritOrient(params);
    else if (name == "setVisible")
        return setVisible(params);
    else if (name == "shape")
        return shape(params);
    else if (name == "visible")
        return visible(params);
    else if (name == "camera")
        return getCamera(params);
    else if (name == "setCameraPosition")
        return setCameraPosition(params);
    else if (name == "setCameraOrientation")
        return setCameraOrientation(params);
    else if (name == "getAnimationList")
        return getAnimationList(params);
    else if (name == "startAnimation")
        return startAnimation(params);
    else if (name == "stopAnimation")
        return stopAnimation(params);
    else if (name == "setInheritScale")
        return setInheritScale(params);
    else if (name == "isReady")
        return isReady(params);
    else if (name == "setSkybox")
        return setSkybox(params);
    else
        return OgreRenderer::invoke(params);

    return boost::any();
}

boost::any OgreSystem::isReady(std::vector<boost::any>& params)
{
    return Invokable::asAny(mReady);
}

boost::any OgreSystem::setOnReady(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    // On ready cb
    if (!Invokable::anyIsInvokable(params[1])) return boost::any();
    // On reset ready cb
    if (params.size() > 2 && !Invokable::anyIsInvokable(params[2])) return boost::any();

    Invokable* handler = Invokable::anyAsInvokable(params[1]);
    mOnReadyCallback = handler;

    Invokable* reset_handler = Invokable::anyAsInvokable(params[2]);
    mOnResetReadyCallback = reset_handler;

    return boost::any();
}

boost::any OgreSystem::evalInUI(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();

    mMouseHandler->mUIWidgetView->evaluateJS(Invokable::anyAsString(params[1]));

    return boost::any();
}

boost::any OgreSystem::createWindow(const String& window_name, bool is_html, bool is_file, String content, uint32 width, uint32 height) {
    WebViewManager* wvManager = WebViewManager::getSingletonPtr();
    WebView* ui_wv = wvManager->getWebView(window_name);
    if(!ui_wv)
    {
        ui_wv = wvManager->createWebView(
            mContext, window_name, window_name,
            width, height, OverlayPosition(RP_TOPLEFT),
            simStrand);

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
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1]) || !Invokable::anyIsString(params[2])) return boost::any();

    String window_name = Invokable::anyAsString(params[1]);
    String html_url = Invokable::anyAsString(params[2]);
    uint32 width = (params.size() > 3 && Invokable::anyIsNumeric(params[3])) ? Invokable::anyAsNumeric(params[3]) : 300;
    uint32 height = (params.size() > 4 && Invokable::anyIsNumeric(params[4])) ? Invokable::anyAsNumeric(params[4]) : 300;
    return createWindow(window_name, false, false, html_url, width, height);
}

boost::any OgreSystem::createWindowFile(vector<boost::any>& params) {
    // Create a window using the specified url
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1]) || !Invokable::anyIsString(params[2])) return boost::any();

    String window_name = Invokable::anyAsString(params[1]);
    String html_url = Invokable::anyAsString(params[2]);
    uint32 width = (params.size() > 3 && Invokable::anyIsNumeric(params[3])) ? Invokable::anyAsNumeric(params[3]) : 300;
    uint32 height = (params.size() > 4 && Invokable::anyIsNumeric(params[4])) ? Invokable::anyAsNumeric(params[4]) : 300;

    return createWindow(window_name, false, true, html_url, width, height);
}

boost::any OgreSystem::addModuleToUI(std::vector<boost::any>& params) {
    if (params.size() != 3) return boost::any();
    if (!anyIsString(params[1]) || !anyIsString(params[2])) return boost::any();

    String window_name = anyAsString(params[1]);
    String html_url = anyAsString(params[2]);

    if (!mMouseHandler) return boost::any();

    // Note the ../, this is because that loadModule executes from within data/chrome
    mMouseHandler->mUIWidgetView->evaluateJS("loadModule('../" + html_url + "')");
    mMouseHandler->mUIWidgetView->defaultEvent(window_name + "-__ready");
    Invokable* inn = mMouseHandler->mUIWidgetView;
    return Invokable::asAny(inn);
}

boost::any OgreSystem::addTextModuleToUI(std::vector<boost::any>& params) {
    if (params.size() != 3) return boost::any();
    if (!anyIsString(params[1]) || !anyIsString(params[2])) return boost::any();

    String window_name = anyAsString(params[1]);
    String module_js = anyAsString(params[2]);

    if (!mMouseHandler) return boost::any();

    // Note that we assume escaped js
    mMouseHandler->mUIWidgetView->evaluateJS("loadModuleText(" + module_js + ")");
    mMouseHandler->mUIWidgetView->defaultEvent(window_name + "-__ready");
    Invokable* inn = mMouseHandler->mUIWidgetView;
    return Invokable::asAny(inn);
}

boost::any OgreSystem::createWindowHTML(vector<boost::any>& params) {
    // Create a window using the specified HTML content
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1]) || !Invokable::anyIsString(params[2])) return boost::any();

    String window_name = Invokable::anyAsString(params[1]);
    String html_script = Invokable::anyAsString(params[2]);
    uint32 width = (params.size() > 3 && Invokable::anyIsNumeric(params[3])) ? Invokable::anyAsNumeric(params[3]) : 300;
    uint32 height = (params.size() > 4 && Invokable::anyIsNumeric(params[4])) ? Invokable::anyAsNumeric(params[4]) : 300;

    return createWindow(window_name, true, false, html_script, width, height);
}

boost::any OgreSystem::addInputHandler(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsInvokable(params[1])) return boost::any();

    Invokable* handler = Invokable::anyAsInvokable(params[1]);
    mMouseHandler->addDelegate(handler);
    return boost::any();
}


boost::any OgreSystem::removeInputHandler(std::vector<boost::any>& params)
{
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsInvokable(params[1])) return boost::any();

    Invokable* handler = Invokable::anyAsInvokable(params[1]);
    mMouseHandler->removeDelegate(handler);
    return boost::any();
}


boost::any OgreSystem::pick(vector<boost::any>& params) {
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsNumeric(params[1])) return boost::any();
    if (!Invokable::anyIsNumeric(params[2])) return boost::any();

    float x = Invokable::anyAsNumeric(params[1]);
    float y = Invokable::anyAsNumeric(params[2]);
    // We support bool ignore_self or SpaceObjectReference as third
    SpaceObjectReference ignore = SpaceObjectReference::null();
    if (params.size() > 3) {
        if (Invokable::anyIsBoolean(params[3]) && Invokable::anyAsBoolean(params[3]))
            ignore = mPresenceID;
        else if (Invokable::anyIsObject(params[3]))
            ignore = Invokable::anyAsObject(params[3]);
    }
    Vector3f hitPoint;
    SpaceObjectReference result = mMouseHandler->pick(Vector2f(x,y), 1, ignore, &hitPoint);

    Invokable::Dict pick_result;
    pick_result["object"] = Invokable::asAny(result);
    Invokable::Dict pick_position;
    pick_position["x"] = Invokable::asAny(hitPoint.x);
    pick_position["y"] = Invokable::asAny(hitPoint.y);
    pick_position["z"] = Invokable::asAny(hitPoint.z);
    pick_result["position"] = Invokable::asAny(pick_position);

    return Invokable::asAny(pick_result);
}

boost::any OgreSystem::getAnimationList(vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsObject(params[1])) return boost::any();

    SpaceObjectReference objid = Invokable::anyAsObject(params[1]);

    if (mSceneEntities.find(objid.toString()) == mSceneEntities.end()) return boost::any();
    Entity* ent = mSceneEntities.find(objid.toString())->second;

    const std::vector<String> animationList = ent->getAnimationList();
    Invokable::Array arr;

    for (uint32 i = 0; i < animationList.size(); i++) {
      arr.push_back(Invokable::asAny(animationList[i]));
    }

    return Invokable::asAny(arr);
}

boost::any OgreSystem::startAnimation(std::vector<boost::any>& params) {
  if (params.size() < 3) return boost::any();
  if (!Invokable::anyIsObject(params[1])) return boost::any();
  if ( !anyIsString(params[2]) ) return boost::any();

  SpaceObjectReference objid = Invokable::anyAsObject(params[1]);
  String animation_name = Invokable::anyAsString(params[2]);

  if (mSceneEntities.find(objid.toString()) == mSceneEntities.end()) return boost::any();

  Entity* ent = mSceneEntities.find(objid.toString())->second;
  ent->setAnimation(animation_name);

  return boost::any();
}

boost::any OgreSystem::stopAnimation(std::vector<boost::any>& params) {
  if (params.size() < 2) return boost::any();
  if (!Invokable::anyIsObject(params[1])) return boost::any();

  SpaceObjectReference objid = Invokable::anyAsObject(params[1]);

  if (mSceneEntities.find(objid.toString()) == mSceneEntities.end()) return boost::any();

  Entity* ent = mSceneEntities.find(objid.toString())->second;
  ent->setAnimation("");

  return boost::any();
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

boost::any OgreSystem::visible(vector<boost::any>& params) {
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsObject(params[1])) return boost::any();
    if (!Invokable::anyIsBoolean(params[2])) return boost::any();

    SpaceObjectReference objid = Invokable::anyAsObject(params[1]);
    bool setting = Invokable::anyAsBoolean(params[2]);

    if (mSceneEntities.find(objid.toString()) == mSceneEntities.end()) return boost::any();
    Entity* ent = mSceneEntities.find(objid.toString())->second;
    ent->setVisible(setting);

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

    Invokable::Dict camera_orient;
    Quaternion orient = mPrimaryCamera->getOrientation();
    camera_orient["x"] = Invokable::asAny(orient.x);
    camera_orient["y"] = Invokable::asAny(orient.y);
    camera_orient["z"] = Invokable::asAny(orient.z);
    camera_orient["w"] = Invokable::asAny(orient.w);
    camera_info["orientation"] = Invokable::asAny(camera_orient);

    return Invokable::asAny(camera_info);
}

boost::any OgreSystem::setCameraPosition(vector<boost::any>& params) {
    if (mPrimaryCamera == NULL) return boost::any();
    if (params.size() < 4) return boost::any();
    if (!Invokable::anyIsNumeric(params[1]) || !Invokable::anyIsNumeric(params[2]) || !Invokable::anyIsNumeric(params[3])) return boost::any();

    double x = Invokable::anyAsNumeric(params[1]),
        y = Invokable::anyAsNumeric(params[2]),
        z = Invokable::anyAsNumeric(params[3]);

    mPrimaryCamera->setPosition(Vector3d(x, y, z));

    return boost::any();
}

boost::any OgreSystem::setCameraOrientation(vector<boost::any>& params) {
    if (mPrimaryCamera == NULL) return boost::any();
    if (params.size() < 5) return boost::any();
    if (!Invokable::anyIsNumeric(params[1]) || !Invokable::anyIsNumeric(params[2]) || !Invokable::anyIsNumeric(params[3]) || !Invokable::anyIsNumeric(params[4])) return boost::any();

    double x = Invokable::anyAsNumeric(params[1]),
        y = Invokable::anyAsNumeric(params[2]),
        z = Invokable::anyAsNumeric(params[3]),
        w = Invokable::anyAsNumeric(params[4]);

    mPrimaryCamera->setOrientation(Quaternion(x, y, z, w, Quaternion::XYZW()));

    return boost::any();
}

boost::any OgreSystem::world2Screen(vector<boost::any>& params) {
    if (mPrimaryCamera == NULL) return boost::any();
    if (params.size() < 4) return boost::any();
    if (!Invokable::anyIsNumeric(params[1]) || !Invokable::anyIsNumeric(params[2]) || !Invokable::anyIsNumeric(params[3])) return boost::any();

    double x = Invokable::anyAsNumeric(params[1]),
           y = Invokable::anyAsNumeric(params[2]),
           z = Invokable::anyAsNumeric(params[3]);

    Ogre::Camera *ogreCamera = mPrimaryCamera->getOgreCamera();
    Ogre::Vector3 hcsPos = ogreCamera->getProjectionMatrix() * (ogreCamera->getViewMatrix() * Ogre::Vector3(x, y, z));

    //if (fabs(hcsPos.x) > 1 || fabs(hcsPos.y) > 1) return boost::any();

    Invokable::Dict screenPos;
    screenPos["x"] = Invokable::asAny(hcsPos.x);
    screenPos["y"] = Invokable::asAny(hcsPos.y);

    return Invokable::asAny(screenPos);
}

boost::any OgreSystem::shape(vector<boost::any>& params) {
    if (mSceneManager == NULL) return boost::any();
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();
    if (!Invokable::anyIsBoolean(params[2]))return boost::any();

    String objName = Invokable::anyAsString(params[1]) + "-obj";
    if (!mSceneManager->hasManualObject(objName)) return boost::any();
    Ogre::ManualObject *obj = mSceneManager->getManualObject(objName);

    bool clear = Invokable::anyAsBoolean(params[2]);
    if (clear)
        obj->clear();

    if (params.size() < 10) return boost::any();
    if (!Invokable::anyIsNumeric(params[3])) return boost::any();
    int type = (int) floor(Invokable::anyAsNumeric(params[3]));
    Ogre::RenderOperation::OperationType opType;
    if (type == 2)
        opType = Ogre::RenderOperation::OT_LINE_LIST;
    else if (type == 3)
        opType = Ogre::RenderOperation::OT_LINE_STRIP;
    else
        return boost::any();


    obj->begin(currentMat, opType);
    for (vector<boost::any>::size_type i = 4; i < params.size() - 2; i += 3) {
        if (!Invokable::anyIsNumeric(params[i]) || !Invokable::anyIsNumeric(params[i+1]) || !Invokable::anyIsNumeric(params[i+2]))
            continue;

        double x = Invokable::anyAsNumeric(params[i]);
        double y = Invokable::anyAsNumeric(params[i+1]);
        double z = Invokable::anyAsNumeric(params[i+2]);

        obj->position(x, y, z);
    }
    obj->end();

    return boost::any();
}

boost::any OgreSystem::setVisible(vector<boost::any>& params) {
    if (mSceneManager == NULL) return boost::any();
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();
    if (!Invokable::anyIsBoolean(params[2])) return boost::any();

    String nodeName = Invokable::anyAsString(params[1]);
    if (!mSceneManager->hasSceneNode(nodeName)) return boost::any();
    Ogre::SceneNode *node = mSceneManager->getSceneNode(nodeName);

    bool visible = Invokable::anyAsBoolean(params[2]);
    node->setVisible(visible);
    return boost::any();
}

boost::any OgreSystem::setInheritScale(vector<boost::any>& params) {
    if (mSceneManager == NULL) return boost::any();
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();
    if (!Invokable::anyIsBoolean(params[2])) return boost::any();

    String nodeName = Invokable::anyAsString(params[1]);
    if (!mSceneManager->hasSceneNode(nodeName)) return boost::any();
    Ogre::SceneNode *node = mSceneManager->getSceneNode(nodeName);

    bool val = Invokable::anyAsBoolean(params[2]);
    node->setInheritOrientation(val);
    if (val) {
        const Ogre::Vector3& parentScale = node->getParentSceneNode()->getScale();
        node->setScale(1 / parentScale.x, 1 / parentScale.y, 1 / parentScale.z);
    } else {
        node->setScale(1, 1, 1);
    }

    return boost::any();
}

boost::any OgreSystem::setInheritOrient(vector<boost::any>& params) {
    if (mSceneManager == NULL) return boost::any();
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();
    if (!Invokable::anyIsBoolean(params[2])) return boost::any();

    String nodeName = Invokable::anyAsString(params[1]);
    if (!mSceneManager->hasSceneNode(nodeName)) return boost::any();
    Ogre::SceneNode *node = mSceneManager->getSceneNode(nodeName);

    bool val = Invokable::anyAsBoolean(params[2]);
    node->setInheritOrientation(val);
    return boost::any();
}

double OgreSystem::clamp(const double& val) {
    if (val > 255) return 255;
    if (val < 0) return 0;
    return val;
}

boost::any OgreSystem::setMat(vector<boost::any>& params) {
    uint32 er = 0, eg = 0, eb = 0;
    if (params.size() > 3) {
        if (Invokable::anyIsNumeric(params[1])
         && Invokable::anyIsNumeric(params[2])
         && Invokable::anyIsNumeric(params[3])) {
            er = (uint32) floor(clamp(Invokable::anyAsNumeric(params[1])));
            eg = (uint32) floor(clamp(Invokable::anyAsNumeric(params[2])));
            eb = (uint32) floor(clamp(Invokable::anyAsNumeric(params[3])));
        }
    }

    uint32 dr = 0, dg = 0, db = 0;
    if (params.size() > 6) {
        if (Invokable::anyIsNumeric(params[4])
         && Invokable::anyIsNumeric(params[5])
         && Invokable::anyIsNumeric(params[6])) {
            dr = (uint32) floor(clamp(Invokable::anyAsNumeric(params[4])));
            dg = (uint32) floor(clamp(Invokable::anyAsNumeric(params[5])));
            db = (uint32) floor(clamp(Invokable::anyAsNumeric(params[6])));
        }
    }

    uint32 sr = 0, sg = 0, sb = 0;
    if (params.size() > 9) {
        if (Invokable::anyIsNumeric(params[7])
         && Invokable::anyIsNumeric(params[8])
         && Invokable::anyIsNumeric(params[9])) {
            sr = (uint32) floor(clamp(Invokable::anyAsNumeric(params[7])));
            sg = (uint32) floor(clamp(Invokable::anyAsNumeric(params[8])));
            sb = (uint32) floor(clamp(Invokable::anyAsNumeric(params[9])));
        }
    }

    uint32 ar = 0, ag = 0, ab = 0;
    if (params.size() > 12) {
        if (Invokable::anyIsNumeric(params[10])
         && Invokable::anyIsNumeric(params[11])
         && Invokable::anyIsNumeric(params[12])) {
            ar = (uint32) floor(clamp(Invokable::anyAsNumeric(params[10])));
            ag = (uint32) floor(clamp(Invokable::anyAsNumeric(params[11])));
            ab = (uint32) floor(clamp(Invokable::anyAsNumeric(params[12])));
        }
    }

    char buf[64];
    int ret = sprintf(buf, "%02x-%02x-%02x:%02x-%02x-%02x:%02x-%02x-%02x:%02x-%02x-%02x",
            er, eg, eb, dr, dg, db, sr, sg, sb, ar, ag, ab);
    currentMat.clear();
    currentMat.append(buf);

    if (!Ogre::MaterialManager::getSingleton().resourceExists(currentMat)) {
         Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(currentMat, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mat->setReceiveShadows(false);
        mat->getTechnique(0)->setLightingEnabled(true);
        mat->getTechnique(0)->getPass(0)->setSpecular(sr/255.0, sg/255.0, sb/255.0, 1);
        mat->getTechnique(0)->getPass(0)->setDiffuse(dr/255.0, dg/255.0, db/255.0, 1);
        mat->getTechnique(0)->getPass(0)->setAmbient(ar/255.0, ag/255.0, ab/255.0);
        mat->getTechnique(0)->getPass(0)->setSelfIllumination(er/255.0, eg/255.0, eb/255.0);
    }

    return Invokable::asAny(currentMat);
}

boost::any OgreSystem::newDrawing(vector<boost::any>& params) {
    if (mSceneManager == NULL) return boost::any();
    if (params.size() < 2) return boost::any();
    if (!Invokable::anyIsString(params[1])) return boost::any();
    String nodeName = Invokable::anyAsString(params[1]);
    if (mSceneManager->hasSceneNode(nodeName)) return boost::any();
    Ogre::SceneNode *parent = mSceneManager->getRootSceneNode();
    if (params.size() > 2 && Invokable::anyIsObject(params[2])) {
        SpaceObjectReference objid = Invokable::anyAsObject(params[2]);
        if (mSceneEntities.find(objid.toString()) != mSceneEntities.end()) {
            Entity *ent = mSceneEntities.find(objid.toString())->second;
            parent = ent->getSceneNode();
        }
    }

    bool inheritOrient = true;
    if (params.size() > 3 && Invokable::anyIsBoolean(params[3]))
        inheritOrient = Invokable::anyAsBoolean(params[3]);

    bool inheritScale = true;
    if (params.size() > 4 && Invokable::anyIsBoolean(params[4]))
        inheritScale = Invokable::anyAsBoolean(params[4]);

    bool visible = true;
    if (params.size() > 5 && Invokable::anyIsBoolean(params[5]))
        visible = Invokable::anyAsBoolean(params[5]);

    Ogre::ManualObject *obj = mSceneManager->createManualObject(nodeName + "-obj");
    Ogre::SceneNode *node = parent->createChildSceneNode(nodeName);
    node->attachObject(obj);

    if (inheritScale) {
        node->setInheritScale(true);
        node->setScale(1 / parent->getScale().x, 1 / parent->getScale().y, 1 / parent->getScale().z);
    } else {
        node->setInheritScale(false);
    }
    node->setInheritOrientation(inheritOrient);
    node->setVisible(visible);

    return boost::any();
}

boost::any OgreSystem::axis(vector<boost::any>& params) {
    if (params.size() < 4) return boost::any();
    if (!Invokable::anyIsObject(params[1])) return boost::any();
    if (!Invokable::anyIsString(params[2])) return boost::any();
    if (!Invokable::anyIsBoolean(params[3])) return boost::any();

    SpaceObjectReference objid = Invokable::anyAsObject(params[1]);
    String axisName = Invokable::anyAsString(params[2]);
    bool visible = Invokable::anyAsBoolean(params[3]);

    if (axisName != "x" && axisName != "y" && axisName != "z")
        return boost::any();

    if (mSceneEntities.find(objid.toString()) == mSceneEntities.end())
        return boost::any();
    Entity *ent = mSceneEntities.find(objid.toString())->second;
    Ogre::SceneNode *parent = ent->getSceneNode();
    Ogre::SceneNode *child = NULL;
    Ogre::SceneNode::ChildNodeIterator it = parent->getChildIterator();
    String nodeName = axisName + objid.toString();
    while (it.hasMoreElements()) {
        Ogre::SceneNode *node = dynamic_cast<Ogre::SceneNode *>(it.getNext());
        if (node->getName() == nodeName) {
            child = node;
            break;
        }
    }

    if (visible) {
        if (!child) {
            Ogre::ManualObject *axisObj = mSceneManager->createManualObject(axisName + "-axis-" + objid.toString());
            if (axisName == "x") {
                axisObj->begin("x-axis-neg-mat", Ogre::RenderOperation::OT_LINE_LIST);
                axisObj->position(-7 * ent->bounds().radius(), 0, 0);
                axisObj->position(0, 0, 0);
                axisObj->end();
                axisObj->begin("x-axis-pos-mat", Ogre::RenderOperation::OT_LINE_LIST);
                axisObj->position(0, 0, 0);
                axisObj->position(7 * ent->bounds().radius(), 0, 0);
                axisObj->end();
            } else if (axisName == "y") {
                axisObj->begin("y-axis-neg-mat", Ogre::RenderOperation::OT_LINE_LIST);
                axisObj->position(0, -7 * ent->bounds().radius(), 0);
                axisObj->position(0, 0, 0);
                axisObj->end();
                axisObj->begin("y-axis-pos-mat", Ogre::RenderOperation::OT_LINE_LIST);
                axisObj->position(0, 0, 0);
                axisObj->position(0, 7 * ent->bounds().radius(), 0);
                axisObj->end();
            } else {
                axisObj->begin("z-axis-neg-mat", Ogre::RenderOperation::OT_LINE_LIST);
                axisObj->position(0, 0, -7 * ent->bounds().radius());
                axisObj->position(0, 0, 0);
                axisObj->end();
                axisObj->begin("z-axis-pos-mat", Ogre::RenderOperation::OT_LINE_LIST);
                axisObj->position(0, 0, 0);
                axisObj->position(0, 0, 7 * ent->bounds().radius());
                axisObj->end();
            }

            child = parent->createChildSceneNode(nodeName);
            child->attachObject(axisObj);
        }

        bool global = false;
        if (params.size() > 4) {
            if (Invokable::anyIsBoolean(params[4])) {
                global = Invokable::anyAsBoolean(params[4]);
            }
        }
        child->setInheritScale(false);
        child->setInheritOrientation(!global);
        child->setVisible(true);
    } else if (child) {
        child->setVisible(false);
    }

    return boost::any();
}

boost::any OgreSystem::setSkybox(std::vector<boost::any>& params) {
    if (params.size() < 2) return boost::any();
    // setSkybox shape image_url
    if (!Invokable::anyIsString(params[1])) return boost::any();

    // We can accept just one parameter if the user is trying to disable the
    // skybox
    String shape_str = Invokable::anyAsString(params[1]);
    if (shape_str == "disabled") {
        if (mSkybox && (*mSkybox) && initialized)
            mSkybox->unload();
        mSkybox = SkyboxPtr(new Skybox());
    }

    // Otherwise, we need all the params
    if (params.size() < 3) return boost::any();
    if (!Invokable::anyIsString(params[2]))
        return boost::any();

    if ((params.size() > 3 && !Invokable::anyIsNumeric(params[3])) ||
        (params.size() > 4 && !Invokable::anyIsNumeric(params[4])) ||
        (params.size() > 5 && !Invokable::anyIsNumeric(params[5])))
    {
        return boost::any();
    }

    // Orientation
    if (params.size() > 9 &&
        !(Invokable::anyIsNumeric(params[6]) &&
            Invokable::anyIsNumeric(params[7]) &&
            Invokable::anyIsNumeric(params[8]) &&
            Invokable::anyIsNumeric(params[9])))
    {
        return boost::any();
    }

    Skybox::SkyboxShape shape;
    if (shape_str == "cube")
        shape = Skybox::SKYBOX_CUBE;
    else if (shape_str == "dome")
        shape = Skybox::SKYBOX_DOME;
    else if (shape_str == "plane")
        shape = Skybox::SKYBOX_PLANE;
    else
        return boost::any();

    String image_url = Invokable::anyAsString(params[2]);

    if (mSkybox && (*mSkybox) && initialized)
        mSkybox->unload();

    mSkybox = SkyboxPtr(new Skybox(shape, image_url));
    if (params.size() > 3)
        mSkybox->distance = Invokable::anyAsNumeric(params[3]);
    if (params.size() > 4)
        mSkybox->tiling = Invokable::anyAsNumeric(params[4]);
    if (params.size() > 5)
        mSkybox->curvature = Invokable::anyAsNumeric(params[5]);

    if (params.size() > 9)
        mSkybox->orientation = Quaternion(
            Invokable::anyAsNumeric(params[6]),
            Invokable::anyAsNumeric(params[7]),
            Invokable::anyAsNumeric(params[8]),
            Invokable::anyAsNumeric(params[9])
        );

    if (mSkybox && (*mSkybox) && initialized)
        mSkybox->load(mSceneManager, mResourceLoader, mTransferPool);

    return Invokable::asAny(true);
}

}
}
