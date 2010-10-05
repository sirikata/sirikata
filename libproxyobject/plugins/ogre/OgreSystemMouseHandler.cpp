/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystemMouseHandler.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#include "OgreSystem.hpp"
#include "OgreMeshRaytrace.hpp"
#include "CameraEntity.hpp"
#include "LightEntity.hpp"
#include "Lights.hpp"
#include "MeshEntity.hpp"
#include "input/SDLInputManager.hpp"
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyWebViewObject.hpp>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyLightObject.hpp>
#include <sirikata/proxyobject/TimeOffsetManager.hpp>
#include "input/InputEvents.hpp"
#include "input/SDLInputDevice.hpp"
#include "DragActions.hpp"
#include "WebView.hpp"
#include "InputResponse.hpp"
#include "InputBinding.hpp"
#include <sirikata/core/task/Event.hpp>
#include <sirikata/core/task/Time.hpp>
#include <sirikata/core/task/EventManager.hpp>
#include <SDL_keysym.h>
#include <set>

#include "WebViewManager.hpp"
#include "CameraPath.hpp"
#include "Protocol_Sirikata.pbj.hpp"
#include <sirikata/core/util/RoutableMessageBody.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/util/KnownMessages.hpp>
#include <sirikata/core/util/KnownScriptTypes.hpp>

#include <boost/lexical_cast.hpp>

namespace Sirikata {
namespace Graphics {
using namespace Input;
using namespace Task;
using namespace std;

#define DEG2RAD 0.0174532925
#ifdef _WIN32
#undef SDL_SCANCODE_UP
#define SDL_SCANCODE_UP 0x60
#undef SDL_SCANCODE_RIGHT
#define SDL_SCANCODE_RIGHT 0x5e
#undef SDL_SCANCODE_DOWN
#define SDL_SCANCODE_DOWN 0x5a
#undef SDL_SCANCODE_LEFT
#define SDL_SCANCODE_LEFT 0x5c
#undef SDL_SCANCODE_PAGEUP
#define SDL_SCANCODE_PAGEUP 0x61
#undef SDL_SCANCODE_PAGEDOWN
#define SDL_SCANCODE_PAGEDOWN 0x5b
#endif

bool compareEntity (const Entity* one, const Entity* two) {

    ProxyObject *pp = one->getProxyPtr().get();

    ProxyCameraObject* camera1 = dynamic_cast<ProxyCameraObject*>(pp);
    ProxyLightObject* light1 = dynamic_cast<ProxyLightObject*>(pp);
    ProxyMeshObject* mesh1 = dynamic_cast<ProxyMeshObject*>(pp);
    Time now = one->getScene()->getLocalTimeOffset()->now(*pp);
    Location loc1 = pp->globalLocation(now);
    pp = two->getProxyPtr().get();
    Location loc2 = pp->globalLocation(now);
    ProxyCameraObject* camera2 = dynamic_cast<ProxyCameraObject*>(pp);
    ProxyLightObject* light2 = dynamic_cast<ProxyLightObject*>(pp);
    ProxyMeshObject* mesh2 = dynamic_cast<ProxyMeshObject*>(pp);
    if (camera1 && !camera2) return true;
    if (camera2 && !camera1) return false;
    if (camera1 && camera2) {
        return loc1.getPosition().x < loc2.getPosition().x;
    }
    if (light1 && mesh2) return true;
    if (mesh1 && light2) return false;
    if (light1 && light2) {
        return loc1.getPosition().x < loc2.getPosition().x;
    }
    if (mesh1 && mesh2) {
        return mesh1->getPhysical().name < mesh2->getPhysical().name;
    }
    return one<two;
}

// Defined in DragActions.cpp.

class OgreSystem::MouseHandler {
    OgreSystem *mParent;
    std::vector<SubscriptionId> mEvents;
    typedef std::multimap<InputDevice*, SubscriptionId> DeviceSubMap;
    DeviceSubMap mDeviceSubscriptions;

    uint32 mScreenshotID;
    bool mPeriodicScreenshot;
    Task::LocalTime mScreenshotStartTime;
    Task::LocalTime mLastScreenshotTime;

    SpaceObjectReference mCurrentGroup;
    typedef std::set<ProxyObjectWPtr> SelectedObjectSet;
    SelectedObjectSet mSelectedObjects;

    struct UIInfo {
        UIInfo()
         : scripting(NULL)
        {}

        WebView* scripting;
    };
    typedef std::map<SpaceObjectReference, UIInfo> ObjectUIMap;
    ObjectUIMap mObjectUIs;
    // Currently we don't have a good way to push the space object reference to
    // the webview because doing it too early causes it to fail since the JS in
    // the page hasn't been executed yet.  Instead, we maintain a map so we can
    // extract it from the webview ID.
    typedef std::map<WebView*, ProxyObjectWPtr> ScriptingUIObjectMap;
    ScriptingUIObjectMap mScriptingUIObjects;

    SpaceObjectReference mLastShiftSelected;
    IntersectResult mMouseDownTri;
    ProxyObjectWPtr mMouseDownObject;
    int mMouseDownSubEntity; // not dereferenced.
    int mLastHitCount;
    float mLastHitX;
    float mLastHitY;
    // map from mouse button to drag for that mouse button.
    /* as far as multiple cursors are concerned,
       each cursor should have its own MouseHandler instance */
    std::map<int, DragAction> mDragAction;
    std::map<int, ActiveDrag*> mActiveDrag;
    std::set<int> mWebViewActiveButtons;
    /*
        typedef EventResponse (MouseHandler::*ClickAction) (EventPtr evbase);
        std::map<int, ClickAction> mClickAction;
    */

    CameraPath mCameraPath;
    bool mRunningCameraPath;
    uint32 mCameraPathIndex;
    Task::DeltaTime mCameraPathTime;
    Task::LocalTime mLastCameraTime;
    Task::LocalTime mLastFpsTime;

    typedef std::map<String, InputResponse*> InputResponseMap;
    InputResponseMap mInputResponses;

    InputBinding mInputBinding;

    WebView* mUploadWebView;
    WebView* mFPSWidgetView;

    WebView* mQueryAngleWidgetView;
    // To avoid too many messages, update only after a timeout
    float mNewQueryAngle;
    Network::IOTimerPtr mQueryAngleTimer;


    class SubObjectIterator {
        typedef Entity* value_type;
        //typedef ssize_t difference_type;
        typedef size_t size_type;
        OgreSystem::SceneEntitiesMap::const_iterator mIter;
        Entity *mParentEntity;
        OgreSystem *mOgreSys;
        void findNext() {
            while (!end() && !((*mIter).second->getProxy().getParentProxy()->getObjectReference() == mParentEntity->id())) {
                ++mIter;
            }
        }
    public:
        SubObjectIterator(Entity *parent) :
                mParentEntity(parent),
                mOgreSys(parent->getScene()) {
            mIter = mOgreSys->mSceneEntities.begin();
            findNext();
        }
        SubObjectIterator &operator++() {
            ++mIter;
            findNext();
            return *this;
        }
        Entity *operator*() const {
            return (*mIter).second;
        }
        bool end() const {
            return (mIter == mOgreSys->mSceneEntities.end());
        }
    };


    /////////////////// HELPER FUNCTIONS ///////////////

    void mouseOverWebView(CameraEntity *cam, Time time, float xPixel, float yPixel, bool mousedown, bool mouseup) {
        Location location(cam->getProxy().globalLocation(time));
        Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
        Ogre::Ray traceFrom(toOgre(location.getPosition(), mParent->getOffset()), toOgre(dir));
        ProxyObjectPtr obj(mMouseDownObject.lock());
        Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        if (mMouseDownTri.intersected && ent) {
            MeshEntity *me = static_cast<MeshEntity*>(ent);
            IntersectResult res = mMouseDownTri;
            res.distance = 1.0e38;
/* fixme */
            Ogre::Node *node = ent->getSceneNode();
            const Ogre::Vector3 &position = node->_getDerivedPosition();
            const Ogre::Quaternion &orient = node->_getDerivedOrientation();
            const Ogre::Vector3 &scale = node->_getDerivedScale();
            Triangle newt = res.tri;
            newt.v1.coord = (orient * (newt.v1.coord * scale)) + position;
            newt.v2.coord = (orient * (newt.v2.coord * scale)) + position;
            newt.v3.coord = (orient * (newt.v3.coord * scale)) + position;
/* */
            OgreMesh::intersectTri(OgreMesh::transformRay(ent->getSceneNode(), traceFrom), res, &newt, true); // &res.tri
            WebView *wv = me->getWebView(mMouseDownSubEntity);
            if (wv) {
                unsigned short int wid=0,hei=0;
                wv->getExtents(wid,hei);
                int x = res.u * wid;
                int y = res.v * hei;
                if (mousedown) {
                    wv->injectMouseDown(x, y);
                }
                if (mouseup) {
                    wv->injectMouseUp(x, y);
                }
                if (!mousedown && !mouseup) {
                    wv->injectMouseMove(x, y);
                }
            }
        }
    }

    Entity *hoverEntity (CameraEntity *cam, Time time, float xPixel, float yPixel, bool mousedown, int *hitCount,int which=0) {
        Location location(cam->getProxy().globalLocation(time));
        Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
        SILOG(input,info,"X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<location.getPosition()<<"; dir = "<<dir);

        double dist;
        Vector3f normal;
        IntersectResult res;
        int subent=-1;
        Ogre::Ray traceFrom(toOgre(location.getPosition(), mParent->getOffset()), toOgre(dir));
        Entity *mouseOverEntity = mParent->internalRayTrace(traceFrom, false, *hitCount, dist, normal, subent, &res, mousedown, which);
        if (mousedown && mouseOverEntity) {
            MeshEntity *me = dynamic_cast<MeshEntity*>(mouseOverEntity);
            if (me) {
                mMouseDownTri = res;
                mMouseDownObject = me->getProxyPtr();
                mMouseDownSubEntity = subent;
                WebView *wv = me->getWebView(mMouseDownSubEntity);
                if (wv) {
                    //if (which==0) {*hitCount=-1;}
                }
            }
        }
        if (mouseOverEntity) {
            while (
                mouseOverEntity->getProxy().getParentProxy() &&
                !(mouseOverEntity->getProxy().getParentProxy()->getObjectReference() == mCurrentGroup)) {
                mouseOverEntity = mParent->getEntity(mouseOverEntity->getProxy().getParentProxy()->getObjectReference());
                if (mouseOverEntity == NULL) {
                    return NULL; // FIXME: should try again.
                }
            }
            return mouseOverEntity;
        }
        return NULL;
    }

    ///////////////////// CLICK HANDLERS /////////////////////
public:
    void clearSelection() {
        for (SelectedObjectSet::const_iterator selectIter = mSelectedObjects.begin();
                selectIter != mSelectedObjects.end(); ++selectIter) {
            ProxyObjectPtr obj(selectIter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (ent) {
                ent->setSelected(false);
            }
            // Fire deselected event.
        }
        mSelectedObjects.clear();
    }
private:

    static String fillZeroPrefix(const String& prefill, int32 nwide) {
        String retval = prefill;
        while((int)retval.size() < nwide)
            retval = String("0") + retval;
        return retval;
    }

    void screenshotAction() {
        String fname = String("screenshot-") + boost::lexical_cast<String>(mScreenshotID) + String(".png");
        mParent->screenshot(fname);
        mScreenshotID++;
    }

    void togglePeriodicScreenshotAction() {
        mPeriodicScreenshot = !mPeriodicScreenshot;
    }

    void timedScreenshotAction(const Task::LocalTime& t) {
        String fname = String("screenshot-") + fillZeroPrefix(boost::lexical_cast<String>((t - mScreenshotStartTime).toMilliseconds()), 8) + String(".png");
        mParent->screenshot(fname);
    }

    void quitAction() {
        mParent->quit();
    }

    bool recentMouseInRange(float x, float y, float *lastX, float *lastY) {
        float delx = x-*lastX;
        float dely = y-*lastY;

        if (delx<0) delx=-delx;
        if (dely<0) dely=-dely;
        if (delx>.03125||dely>.03125) {
            *lastX=x;
            *lastY=y;

            return false;
        }
        return true;
    }
    int mWhichRayObject;
    void selectObjectAction(Vector2f p, int direction) {
        if (!mParent||!mParent->mPrimaryCamera) return;
        CameraEntity *camera = mParent->mPrimaryCamera;
        Time time = Time::local(); // FIXME #116, #117
        if (!camera) {
            return;
        }
        if (mParent->mInputManager->isModifierDown(Input::MOD_SHIFT)) {
            // add object.
            int numObjectsUnderCursor=0;
            Entity *mouseOver = hoverEntity(camera, time, p.x, p.y, false, &numObjectsUnderCursor, mWhichRayObject);
            if (!mouseOver) {
                return;
            }
            if (mouseOver->id() == mLastShiftSelected && numObjectsUnderCursor==mLastHitCount ) {
                SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
                if (selectIter != mSelectedObjects.end()) {
                    ProxyObjectPtr obj(selectIter->lock());
                    Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
                    if (ent) {
                        ent->setSelected(false);
                    }
                    mSelectedObjects.erase(selectIter);
                }
                mWhichRayObject+=direction;
                mLastShiftSelected = SpaceObjectReference::null();
            }else {
                mWhichRayObject=0;
            }
            mouseOver = hoverEntity(camera, time, p.x, p.y, false, &mLastHitCount, mWhichRayObject);
            if (!mouseOver) {
                return;
            }

            SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
            if (selectIter == mSelectedObjects.end()) {
                SILOG(input,info,"Added selection " << mouseOver->id());
                mSelectedObjects.insert(mouseOver->getProxyPtr());
                mouseOver->setSelected(true);
                mLastShiftSelected = mouseOver->id();
                // Fire selected event.
            }
            else {
                ProxyObjectPtr obj(selectIter->lock());
                Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
                if (ent) {
                    SILOG(input,info,"Deselected " << ent->id());
                    ent->setSelected(false);
                }
                mSelectedObjects.erase(selectIter);
                // Fire deselected event.
            }
        }
        else if (mParent->mInputManager->isModifierDown(Input::MOD_CTRL)) {
            SILOG(input,info,"Cleared selection");
            clearSelection();
            mLastShiftSelected = SpaceObjectReference::null();
        }
        else {
            // reset selection.
            clearSelection();
            //mWhichRayObject+=direction;
            int numObjectsUnderCursor=0;
            Entity *mouseOver = hoverEntity(camera, time, p.x, p.y, true, &numObjectsUnderCursor, mWhichRayObject);
            if (recentMouseInRange(p.x, p.y, &mLastHitX, &mLastHitY)==false||numObjectsUnderCursor!=mLastHitCount){
                mouseOver = hoverEntity(camera, time, p.x, p.y, true, &mLastHitCount, mWhichRayObject=0);
            }
            if (mouseOver) {
                mSelectedObjects.insert(mouseOver->getProxyPtr());
                mouseOver->setSelected(true);
                SILOG(input,info,"Replaced selection with " << mouseOver->id());
                // Fire selected event.
            }
            mLastShiftSelected = SpaceObjectReference::null();
        }
        return;
    }

    ///////////////// KEYBOARD HANDLERS /////////////////

    void deleteObjectsAction() {
        Task::LocalTime now(Task::LocalTime::now());
        while (doUngroupObjects(now)) {
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            std::string serializedDestroy;

            RoutableMessageBody body;
            body.add_message("DestroyObject", serializedDestroy);
            std::string serialized;
            body.SerializeToString(&serialized);

            obj->sendMessage(
                Services::RPC,
                MemoryReference(serialized.data(),serialized.length())
            );
        }
        mSelectedObjects.clear();
    }

    Entity *doCloneObject(Entity *ent, const ProxyObjectPtr &parentPtr, Time now) {
/*
        SpaceObjectReference newId = SpaceObjectReference(ent->id().space(), ObjectReference(UUID::random()));
        Location loc = ent->getProxy().globalLocation(now);
        Location localLoc = ent->getProxy().extrapolateLocation(now);
        ProxyManager *proxyMgr = ent->getProxy().getProxyManager();

        std::tr1::shared_ptr<ProxyMeshObject> meshObj(
            std::tr1::dynamic_pointer_cast<ProxyMeshObject>(ent->getProxyPtr()));
        std::tr1::shared_ptr<ProxyLightObject> lightObj(
            std::tr1::dynamic_pointer_cast<ProxyLightObject>(ent->getProxyPtr()));
        ProxyObjectPtr newObj;
        if (meshObj) {
            std::tr1::shared_ptr<ProxyWebViewObject> webObj(
                std::tr1::dynamic_pointer_cast<ProxyWebViewObject>(ent->getProxyPtr()));
            std::tr1::shared_ptr<ProxyMeshObject> newMeshObject;
            if (webObj) {
                std::tr1::shared_ptr<ProxyWebViewObject> newWebObject(new ProxyWebViewObject(proxyMgr, newId, parentPtr->odp()));
                newWebObject->loadURL("http://www.google.com/");
                newMeshObject = newWebObject;
            } else {
                newMeshObject = std::tr1::shared_ptr<ProxyMeshObject>(new ProxyMeshObject(proxyMgr, newId, parentPtr->odp()));
            }
            newObj = newMeshObject;
            proxyMgr->createObject(newMeshObject,mParent->getPrimaryCamera()->getProxy().getQueryTracker());
            newMeshObject->setMesh(meshObj->getMesh());
            newMeshObject->setScale(meshObj->getScale());
        }
        else if (lightObj) {
            std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId, parentPtr->odp()));
            newObj = newLightObject;
            proxyMgr->createObject(newLightObject,mParent->getPrimaryCamera()->getProxy().getQueryTracker());
            newLightObject->update(lightObj->getLastLightInfo());
        }
        else {
            newObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newId, parentPtr->odp()));
            proxyMgr->createObject(newObj,mParent->getPrimaryCamera()->getProxy().getQueryTracker());
        }
        if (newObj) {
            if (parentPtr) {
                newObj->setParent(parentPtr, now, loc, localLoc);
                newObj->resetLocation(now, localLoc);
            }
            else {
                newObj->resetLocation(now, loc);
            }
        }
        {
            std::list<Entity*> toClone;
            for (SubObjectIterator subIter (ent); !subIter.end(); ++subIter) {
                toClone.push_back(*subIter);
            }
            for (std::list<Entity*>::const_iterator iter = toClone.begin(); iter != toClone.end(); ++iter) {
                doCloneObject(*iter, newObj, now);
            }
        }
        return mParent->getEntity(newId);
*/
        return NULL;
    }

    void cloneObjectsAction() {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::LocalTime now(Task::LocalTime::now());
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) {
                continue;
            }
            Time objnow=Time::convertFrom(now,mParent->getLocalTimeOffset()->offset(ent->getProxy()));
            Entity *newEnt = doCloneObject(ent, ent->getProxy().getParentProxy(), objnow);
            Location loc (ent->getProxy().extrapolateLocation(objnow));
            loc.setPosition(loc.getPosition() + Vector3d(WORLD_SCALE/2.,0,0));
            //newEnt->getProxy().resetLocation(objnow, loc);
            newSelectedObjects.insert(newEnt->getProxyPtr());
            newEnt->setSelected(true);
            ent->setSelected(false);
        }
        mSelectedObjects.swap(newSelectedObjects);
    }

    void groupObjectsAction() {
        if (mSelectedObjects.size()<2) {
            return;
        }
        if (!mParent->mPrimaryCamera) {
            return;
        }
        SpaceObjectReference parentId = mCurrentGroup;

        ProxyManager *proxyMgr = mParent->mPrimaryCamera->getProxy().getProxyManager();
        Time now(mParent->getLocalTimeOffset()->now(mParent->mPrimaryCamera->getProxy()));
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) continue;
            if (ent->getProxy().getProxyManager() != proxyMgr) {
                SILOG(input,error,"Attempting to group objects owned by different proxy manager!");
                return;
            }
            if (!(ent->getProxy().getParentProxy()->getObjectReference() == parentId)) {
                SILOG(input,error,"Multiple select "<< ent->id() <<
                    " has parent  "<<ent->getProxy().getParentProxy()->getObjectReference() << " instead of " << mCurrentGroup);
                return;
            }
        }
        Vector3d totalPosition (averageSelectedPosition(now, mSelectedObjects.begin(), mSelectedObjects.end()));
        Location totalLocation (totalPosition,Quaternion::identity(),Vector3f(0,0,0),Vector3f(0,0,0),0);
        Entity *parentEntity = mParent->getEntity(parentId);
        if (parentEntity) {
            totalLocation = parentEntity->getProxy().globalLocation(now);
            totalLocation.setPosition(totalPosition);
        }

        SpaceObjectReference newParentId = SpaceObjectReference(mCurrentGroup.space(), ObjectReference(UUID::random()));
        //proxyMgr->createObject(ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newParentId, mParent->getPrimaryCamera()->getProxy().odp())),mParent->getPrimaryCamera()->getProxy().getQueryTracker());
        Entity *newParentEntity = mParent->getEntity(newParentId);
        //newParentEntity->getProxy().resetLocation(now, totalLocation);

        if (parentEntity) {
            //newParentEntity->getProxy().setParent(parentEntity->getProxyPtr(), now);
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) continue;
            //ent->getProxy().setParent(newParentEntity->getProxyPtr(), now);
            ent->setSelected(false);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(newParentEntity->getProxyPtr());
        newParentEntity->setSelected(true);
    }

    bool doUngroupObjects(Task::LocalTime now) {
        int numUngrouped = 0;
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *parentEnt = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!parentEnt) {
                continue;
            }
            ProxyManager *proxyMgr = parentEnt->getProxy().getProxyManager();
            ProxyObjectPtr parentParent (parentEnt->getProxy().getParentProxy());
            mCurrentGroup = parentEnt->getProxy().getParentProxy()->getObjectReference(); // parentParent may be NULL.
            bool hasSubObjects = false;
            for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
                hasSubObjects = true;
                Entity *ent = *subIter;
                //ent->getProxy().setParent(parentParent, Time::convertFrom(now,mParent->getLocalTimeOffset()->offset(ent->getProxy())));
                newSelectedObjects.insert(ent->getProxyPtr());
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                parentEnt->setSelected(false);
                proxyMgr->destroyObject(parentEnt->getProxyPtr(),mParent->getPrimaryCamera()->getProxy().getQueryTracker());
                parentEnt = NULL; // dies.
                numUngrouped++;
            }
            else {
                newSelectedObjects.insert(parentEnt->getProxyPtr());
            }
        }
        mSelectedObjects.swap(newSelectedObjects);
        return (numUngrouped>0);
    }

    void ungroupObjectsAction() {
        Task::LocalTime now(Task::LocalTime::now());
        doUngroupObjects(now);
    }

    void enterObjectAction() {
        Task::LocalTime now(Task::LocalTime::now());
        if (mSelectedObjects.size() != 1) {
            return;
        }
        Entity *parentEnt = NULL;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            parentEnt = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        }
        if (parentEnt) {
            SelectedObjectSet newSelectedObjects;
            bool hasSubObjects = false;
            for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
                hasSubObjects = true;
                Entity *ent = *subIter;
                newSelectedObjects.insert(ent->getProxyPtr());
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                mSelectedObjects.swap(newSelectedObjects);
                mCurrentGroup = parentEnt->id();
            }
        }
    }

    void leaveObjectAction() {
        Task::LocalTime now(Task::LocalTime::now());
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *selent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (selent) {
                selent->setSelected(false);
            }
        }
        mSelectedObjects.clear();
        Entity *ent = mParent->getEntity(mCurrentGroup);
        if (ent) {
            mCurrentGroup = ent->getProxy().getParentProxy()->getObjectReference();
            Entity *parentEnt = mParent->getEntity(mCurrentGroup);
            if (parentEnt) {
                mSelectedObjects.insert(parentEnt->getProxyPtr());
            }
        }
        else {
            mCurrentGroup = SpaceObjectReference::null();
        }
    }

    /** Create a UI element using a web view. */
    void createUIAction(const String& ui_page) {
        WebView* ui_wv = WebViewManager::getSingleton().createWebView("__object", "__object", 300, 300, OverlayPosition(RP_BOTTOMCENTER));
        ui_wv->loadFile(ui_page);
    }

    /** Create a UI element for interactively scripting an object.
        Sends a message on KnownServices port LISTEN_FOR_SCRIPT_BEGIN to the
        HostedObject. 
     */
    void createScriptingUIAction() {

        static bool onceInitialized = false;

        // Ask all the objects to initialize scripting
        initScriptOnSelectedObjects();

        // Then bring up windows for each of them
        for(SelectedObjectSet::iterator sel_it = mSelectedObjects.begin(); sel_it != mSelectedObjects.end(); sel_it++) {
            ProxyObjectPtr obj(sel_it->lock());
            if (!obj) continue;

            SpaceObjectReference objid = obj->getObjectReference();

            ObjectUIMap::iterator ui_it = mObjectUIs.find(objid);
            if (ui_it == mObjectUIs.end()) {
                mObjectUIs.insert( ObjectUIMap::value_type(objid, UIInfo()) );
                ui_it = mObjectUIs.find(objid);
            }
            UIInfo& ui_info = ui_it->second;

            if (ui_info.scripting != NULL) {
                // Already there, just make sure its showing
                ui_info.scripting->show();
            }
            else {
                if (onceInitialized)
                {
                    WebView* new_scripting_ui =
                        WebViewManager::getSingleton().createWebView(
                            String("__scripting") + objid.toString(), "__scripting", 300, 300,
                            OverlayPosition(RP_BOTTOMCENTER)
                        );
                    new_scripting_ui->loadFile("scripting/prompt.html");

                    ui_info.scripting = new_scripting_ui;
                    mScriptingUIObjects[new_scripting_ui] = obj;
                    return;
                }


                //name it something else, and put it in a different place
                WebView* new_scripting_ui =
                    WebViewManager::getSingleton().createWebView(
                        String("__scripting") + objid.toString(), "__scripting", 300, 300,
                        OverlayPosition(RP_TOPLEFT)
                    );
                new_scripting_ui->loadFile("scripting/prompt.html");

                ui_info.scripting = new_scripting_ui;
                mScriptingUIObjects[new_scripting_ui] = obj;
                onceInitialized = true;

            }
        }
    }

    void LOCAL_createWebviewAction() {
/*
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return;
        SpaceObjectReference newId = SpaceObjectReference(camera->id().space(), ObjectReference(UUID::random()));
        ProxyManager *proxyMgr = camera->getProxy().getProxyManager();
        Time now(mParent->getLocalTimeOffset()->now(camera->getProxy()));
        Location loc (camera->getProxy().globalLocation(now));
        loc.setPosition(loc.getPosition() + Vector3d(direction(loc.getOrientation()))*WORLD_SCALE/3);
        loc.setOrientation(loc.getOrientation());

        std::tr1::shared_ptr<ProxyWebViewObject> newWebObject (new ProxyWebViewObject(proxyMgr, newId, camera->getProxy().odp()));
        proxyMgr->createObject(newWebObject,mParent->getPrimaryCamera()->getProxy().getQueryTracker());
        {
            newWebObject->setMesh(URI("meru:///webview.mesh"));
            newWebObject->loadURL("http://www.yahoo.com/");
        }

        Entity *parentent = mParent->getEntity(mCurrentGroup);
        if (parentent) {
            Location localLoc = loc.toLocal(parentent->getProxy().globalLocation(now));
            newWebObject->setParent(parentent->getProxyPtr(), now, loc, localLoc);
            newWebObject->resetLocation(now, localLoc);
        }
        else {
            newWebObject->resetLocation(now, loc);
        }
        createLight(now)->setParent(newWebObject, now);
        mSelectedObjects.clear();
        mSelectedObjects.insert(newWebObject);
        Entity *ent = mParent->getEntity(newId);
        if (ent) {
            ent->setSelected(true);
        }
*/
    }

    void createWebviewAction() {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return;
        Time now(mParent->getLocalTimeOffset()->now(camera->getProxy()));
        Location curLoc (camera->getProxy().globalLocation(now));
        Protocol::CreateObject creator;
        Protocol::IConnectToSpace space = creator.add_space_properties();
        Protocol::IObjLoc loc = space.mutable_requested_object_loc();
        loc.set_position(curLoc.getPosition() + Vector3d(direction(curLoc.getOrientation()))*WORLD_SCALE/3);
        loc.set_orientation(curLoc.getOrientation());
        loc.set_velocity(Vector3f(0,0,0));
        loc.set_angular_speed(0);
        loc.set_rotational_axis(Vector3f(1,0,0));

        creator.set_mesh("meru:///webview.mesh");
        creator.set_weburl("http://www.yahoo.com/");
        creator.set_scale(Vector3f(2,2,2));

        std::string serializedCreate;
        creator.SerializeToString(&serializedCreate);

        RoutableMessageBody body;
        body.add_message("CreateObject", serializedCreate);
        std::string serialized;
        body.SerializeToString(&serialized);
        camera->getProxy().sendMessage(
            Services::RPC,
            MemoryReference(serialized.data(), serialized.length())
        );
    }

    void onUploadObjectEvent(WebView* webview, const JSArguments& args) {
        /*
        if (args.size() < 1) {
            SILOG(ogre,error,"event() must be called with at least one argument.  It should take the form event(name, other, args, follow)");
            return;
        }

        // We've passed all the checks, just convert everything and we're good to go
    //    String name = args[0].toString();
        String name((char*)args[0].data(), args[0].size());
        JSArguments event_args;
        event_args.insert(event_args.begin(), args.begin() + 1, args.end());

        mInputManager->fire(Task::EventPtr( new WebViewEvent(webview->getName(), args) ));
        */
        printf("upload object event fired arg length = %d\n", (int)args.size());
        if (args.size() != 3) {
            printf("expected 3 arguments, returning.\n");
            return;
        }

        String file_path(args[0].data());
        String title(args[1].data());
        String description(args[2].data());

        printf("Upload request. path = '%s' , title = '%s' , desc = '%s' .\n", file_path.c_str(), title.c_str(), description.c_str());
        WebViewManager::getSingleton().destroyWebView(mUploadWebView);
        mUploadWebView = NULL;
    }

    void startUploadObject() {
        if(mUploadWebView) {
            printf("startUploadObject called. Focusing existing.\n");
            mUploadWebView->focus();
        } else {
            printf("startUploadObject called. Opening upload UI.\n");
            mUploadWebView = WebViewManager::getSingleton().createWebView("upload_tool", "upload_tool",404, 227,
                    OverlayPosition(RP_CENTER), false, 70, TIER_FRONT);
            mUploadWebView->bind("event", std::tr1::bind(&MouseHandler::onUploadObjectEvent, this, _1, _2));
            mUploadWebView->loadFile("chrome/upload.html");
        }
    }

    void handleFPSWidget() {
        if(mFPSWidgetView) {
            printf("closing fps widget\n");
            WebViewManager::getSingleton().destroyWebView(mFPSWidgetView);
            mFPSWidgetView = NULL;
        } else {
            printf("creating fps widget\n");
            mFPSWidgetView = WebViewManager::getSingleton().createWebView("fps_widget", "fps_widget",114, 45,
                    OverlayPosition(RP_BOTTOMRIGHT), false, 70, TIER_FRONT);
            mFPSWidgetView->loadFile("chrome/fps.html");
            mFPSWidgetView->setTransparent(true);
        }
    }


    void handleQueryAngleWidget() {
        if(mQueryAngleWidgetView) {
            WebViewManager::getSingleton().destroyWebView(mQueryAngleWidgetView);
            mQueryAngleWidgetView = NULL;
        } else {
            mQueryAngleWidgetView = WebViewManager::getSingleton().createWebView("query_angle_widget", "query_angle_widget",300, 100,
                    OverlayPosition(RP_BOTTOMRIGHT), false, 70, TIER_FRONT);
            mQueryAngleWidgetView->bind("set_query_angle", std::tr1::bind(&MouseHandler::handleSetQueryAngle, this, _1, _2));
            mQueryAngleWidgetView->loadFile("debug/query_angle.html");
        }
    }

    void handleSetQueryAngle(WebView* webview, const JSArguments& args) {
        assert(args.size() == 1);
        mNewQueryAngle = boost::lexical_cast<float>(args[0].data());
        mQueryAngleTimer->cancel();
        mQueryAngleTimer->wait(Duration::seconds(1.f));
    }

    void handleSetQueryAngleTimeout() {
        printf("New query angle: %f\n", mNewQueryAngle);
    }


    void createScriptedObjectAction(const std::tr1::unordered_map<String, String>& args) {
        typedef std::tr1::unordered_map<String, String> StringMap;
        printf("createScriptedObjectAction: %d\n", (int)args.size());
        // Filter out the script type from rest of args
        String script_type = "";
        StringMap filtered_args = args;
        {
            StringMap::iterator type_it = filtered_args.find("ScriptType");
            if (type_it == filtered_args.end())
                return;
            script_type = type_it->second;
            filtered_args.erase(type_it);
        }

        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return;
        Time now = Time::local(); // FIXME #116, #117
        Location curLoc (camera->getProxy().globalLocation(now));
        Protocol::CreateObject creator;
        Protocol::IConnectToSpace space = creator.add_space_properties();
        Protocol::IObjLoc loc = space.mutable_requested_object_loc();
        loc.set_position(curLoc.getPosition() + Vector3d(direction(curLoc.getOrientation()))*WORLD_SCALE/3);
        loc.set_orientation(curLoc.getOrientation());
        loc.set_velocity(Vector3f(0,0,0));
        loc.set_angular_speed(0);
        loc.set_rotational_axis(Vector3f(1,0,0));

        creator.set_mesh("http://www.sirikata.com/content/assets/cube.dae");
        creator.set_scale(Vector3f(1,1,1));

        creator.set_script(script_type);
        Protocol::IStringMapProperty script_args = creator.mutable_script_args();
        for(StringMap::const_iterator arg_it = filtered_args.begin(); arg_it != filtered_args.end(); arg_it++) {
            script_args.add_keys(arg_it->first); script_args.add_values(arg_it->second);
        }

        std::string serializedCreate;
        creator.SerializeToString(&serializedCreate);

        RoutableMessageBody body;
        body.add_message("CreateObject", serializedCreate);
        std::string serialized;
        body.SerializeToString(&serialized);
        camera->getProxy().sendMessage(
            Services::RPC,
            MemoryReference(serialized.data(), serialized.length())
        );
    }


    /**
       This function sends out a message on KnownServices port
       LISTEN_FOR_SCRIPT_BEGIN to the HostedObject.  Presumably, the hosted
       object receives the message and attaches a JSObjectScript to the HostedObject.
     */
    void initScriptOnSelectedObjects() {
        for (SelectedObjectSet::const_iterator selectIter = mSelectedObjects.begin();
             selectIter != mSelectedObjects.end(); ++selectIter) {
            ProxyObjectPtr obj(selectIter->lock());

            Protocol::ScriptingInit init_script;

            // Filter out the script type from rest of args
            //String script_type = "js"; // FIXME how to decide this?
            init_script.set_script(ScriptTypes::JS_SCRIPT_TYPE);

            std::string serializedInitScript;
            init_script.SerializeToString(&serializedInitScript);

            // RoutableMessageBody body;
            // body.add_message("InitScript", serializedInitScript);
            // std::string serialized;
            // body.SerializeToString(&serialized);
            
            // obj->sendMessage(
            //     Services::RPC,
            //     MemoryReference(serialized.data(), serialized.length())
            // );


            RoutableMessageBody body;
            //body.add_message("InitScript", serializedInitScript);
            body.add_message(KnownMessages::INIT_SCRIPT, serializedInitScript);
            std::string serialized;
            body.SerializeToString(&serialized);
            
            obj->sendMessage(
                Services::LISTEN_FOR_SCRIPT_BEGIN,
                MemoryReference(serialized.data(), serialized.length())
            );

            
        }
    }

    /** Executes a script chunk in an object.
     *  Implicit parameters in args:
     *   Command: (string) command(s) to execute.
     *  The target of the command is determined implicitly based on
     *  the webview this is coming from.
     */
    void executeScript(WebView* wv, const std::tr1::unordered_map<String, String>& args)
    {
        typedef std::tr1::unordered_map<String, String> StringMap;
        
        ScriptingUIObjectMap::iterator objit = mScriptingUIObjects.find(wv);
        if (objit == mScriptingUIObjects.end())
            return;
        ProxyObjectPtr target_obj(objit->second.lock());

        if (!target_obj) return;

        StringMap::const_iterator command_it = args.find("Command");
        assert(command_it != args.end());
        
        //Get Proxy
        Protocol::ScriptingMessage scripting_msg;
        Protocol::IScriptingRequest scripting_req = scripting_msg.add_requests();
        scripting_req.set_id(0);
        scripting_req.set_body(command_it->second);
        std::string serialized_scripting_request;
        scripting_msg.SerializeToString(&serialized_scripting_request);
        target_obj->sendMessage(
            Services::SCRIPTING,
            MemoryReference(serialized_scripting_request)
        );
    }

    /** Closes a webview. */
    void closeWebView(WebView* wv, const std::tr1::unordered_map<String, String>& args) {
        ScriptingUIObjectMap::iterator scriptui_it = mScriptingUIObjects.find(wv);
        if (scriptui_it != mScriptingUIObjects.end()) {
            ProxyObjectPtr target_obj(scriptui_it->second.lock());

            if (target_obj) {
                ObjectUIMap::iterator ui_it = mObjectUIs.find(target_obj->getObjectReference());
                if (ui_it != mObjectUIs.end())
                    mObjectUIs.erase(ui_it);
            }

            mScriptingUIObjects.erase(scriptui_it);
        }

        WebViewManager::getSingleton().destroyWebView(wv);
    }

    std::tr1::shared_ptr<ProxyLightObject> createLight(Time now) {
/*
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return std::tr1::shared_ptr<ProxyLightObject>();
        SpaceObjectReference newId = SpaceObjectReference(camera->id().space(), ObjectReference(UUID::random()));
        ProxyManager *proxyMgr = camera->getProxy().getProxyManager();
        Location loc (camera->getProxy().globalLocation(now));
        loc.setPosition(loc.getPosition());
        loc.setOrientation(Quaternion(0.886995, 0.000000, -0.461779, 0.000000, Quaternion::WXYZ()));

        std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId, camera->getProxy().odp()));
        proxyMgr->createObject(newLightObject,camera->getProxy().getQueryTracker());
        {
            LightInfo li;
            li.setLightDiffuseColor(Color(0.976471, 0.992157, 0.733333));
            li.setLightAmbientColor(Color(.24,.25,.18));
            li.setLightSpecularColor(Color(0,0,0));
            li.setLightShadowColor(Color(0,0,0));
            li.setLightPower(1.0);
            li.setLightRange(75);
            li.setLightFalloff(1,0,0.03);
            li.setLightSpotlightCone(30,40,1);
            li.setCastsShadow(true);
            // set li according to some sample light in the scene file!
            newLightObject->update(li);
        }

        Entity *parentent = mParent->getEntity(mCurrentGroup);
        if (parentent) {
            Location localLoc = loc.toLocal(parentent->getProxy().globalLocation(now));
            newLightObject->setParent(parentent->getProxyPtr(), now, loc, localLoc);
            newLightObject->resetLocation(now, localLoc);
        }
        else {
            newLightObject->resetLocation(now, loc);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(newLightObject);
        Entity *ent = mParent->getEntity(newId);
        if (ent) {
            ent->setSelected(true);
        }
        return newLightObject;
    */
        return std::tr1::shared_ptr<ProxyLightObject>();
    }
    void createLightAction() {
        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return;
        createLight(mParent->getLocalTimeOffset()->now(camera->getProxy()));
    }

	ProxyObjectPtr getTopLevelParent(ProxyObjectPtr camProxy) {
		ProxyObjectPtr parentProxy;
		while ((parentProxy=camProxy->getParentProxy())) {
			camProxy=parentProxy;
		}
		return camProxy;
	}

    void moveAction(Vector3f dir, float amount) {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        if (!mParent||!mParent->mPrimaryCamera) return;
        ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
        if (!cam) return;

        SpaceID space = cam->getObjectReference().space();

        // Make sure the thing we're trying to move really is the thing
        // connected to the world.
        // FIXME We should have a real "owner" VWObject, even if it is possible
        // for it to change over time.
        VWObjectPtr cam_vwobj = cam->getOwner();
        if (cam_vwobj->id(space) != cam->getObjectReference()) return;

        // Get the updated position
        Time now = Time::local(); // FIXME #116, #117
        Location loc = cam->extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        // Request updates from spcae
        TimedMotionVector3f newloc(now, MotionVector3f(Vector3f(loc.getPosition()), (orient * dir) * amount * WORLD_SCALE * .5) );
        SILOG(ogre,fatal,"Req loc: " << loc.getPosition() << loc.getVelocity());
        cam_vwobj->requestLocationUpdate(space, newloc);
        // And update our local Proxy's information, assuming the move will be successful
        cam->setLocation(newloc);
    }

    void rotateAction(Vector3f about, float amount) {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        if (!mParent||!mParent->mPrimaryCamera) return;
        ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
        if (!cam) return;

        SpaceID space = cam->getObjectReference().space();

        // Make sure the thing we're trying to move really is the thing
        // connected to the world.
        // FIXME We should have a real "owner" VWObject, even if it is possible
        // for it to change over time.
        VWObjectPtr cam_vwobj = cam->getOwner();
        if (cam_vwobj->id(space) != cam->getObjectReference()) return;

        // Get the updated position
        Time now = Time::local(); // FIXME #116, #117
        Location loc = cam->extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        // Request updates from spcae
        TimedMotionQuaternion neworient(now, MotionQuaternion(loc.getOrientation(), Quaternion(about, amount)));
        cam_vwobj->requestOrientationUpdate(space, neworient);
        // And update our local Proxy's information, assuming the move will be successful
        cam->setOrientation(neworient);
    }

    void stableRotateAction(float dir, float amount) {

        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        if (!mParent||!mParent->mPrimaryCamera) return;
        ProxyObjectPtr cam = mParent->mPrimaryCamera->getProxyPtr();
        if (!cam) return;

        SpaceID space = cam->getObjectReference().space();

        // Make sure the thing we're trying to move really is the thing
        // connected to the world.
        // FIXME We should have a real "owner" VWObject, even if it is possible
        // for it to change over time.
        VWObjectPtr cam_vwobj = cam->getOwner();
        if (cam_vwobj->id(space) != cam->getObjectReference()) return;

        // Get the updated position
        Time now = Time::local(); // FIXME #116, #117
        Location loc = cam->extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        double p, r, y;
        quat2Euler(orient, p, r, y);
        Vector3f raxis;
        raxis.x = 0;
        raxis.y = std::cos(p*DEG2RAD);
        raxis.z = -std::sin(p*DEG2RAD);

        // Request updates from spcae
        TimedMotionQuaternion neworient(now, MotionQuaternion(loc.getOrientation(), Quaternion(raxis, dir*amount)));
        cam_vwobj->requestOrientationUpdate(space, neworient);
        // And update our local Proxy's information, assuming the move will be successful
        cam->setOrientation(neworient);
    }

    void setDragModeAction(const String& modename) {
        if (modename == "")
            mDragAction[1] = 0;

        mDragAction[1] = DragActionRegistry::get(modename);
    }

    void importAction() {
        std::cout << "input path name for import: " << std::endl;
        std::string filename;
        // a bit of a cludge right now, type name into console.
        std::vector<std::string> files;
        fflush(stdin);
        while (!feof(stdin)) {
            int c = fgetc(stdin);
            if (c == '\r') {
                c = fgetc(stdin);
            }
            if (c==' ') {
                files.push_back(filename);
                filename="";
                std::cout << '\n';
                continue;
            }
            if (c=='\n') {
                files.push_back(filename);
                break;
            }
            if (c=='\033' || c <= 0) {
                std::cout << "<escape>\n";
                return;
            }
            std::cout << (unsigned char)c;
            filename += (unsigned char)c;
        }
        std::cout << '\n';
        mParent->mInputManager->filesDropped(files);
    }

    void saveSceneAction() {
        std::set<std::string> saveSceneNames;
        std::cout << "saving new scene as scene_new.csv: " << std::endl;
        FILE *output = fopen("scene_new.csv", "wt");
        if (!output) {
            perror("Failed to open scene_new.csv");
            return;
        }
        fprintf(output, "objtype,subtype,name,parent,script,scriptparams,");
        fprintf(output, "pos_x,pos_y,pos_z,orient_x,orient_y,orient_z,orient_w,");
        fprintf(output, "vel_x,vel_y,vel_z,rot_axis_x,rot_axis_y,rot_axis_z,rot_speed,");
        fprintf(output, "scale_x,scale_y,scale_z,hull_x,hull_y,hull_z,");
        fprintf(output, "density,friction,bounce,colMask,colMsg,meshURI,diffuse_x,diffuse_y,diffuse_z,ambient,");
        fprintf(output, "specular_x,specular_y,specular_z,shadowpower,");
        fprintf(output, "range,constantfall,linearfall,quadfall,cone_in,cone_out,power,cone_fall,shadow\n");
        OgreSystem::SceneEntitiesMap::const_iterator iter;
        vector<Entity*> entlist;
        entlist.clear();
        for (iter = mParent->mSceneEntities.begin(); iter != mParent->mSceneEntities.end(); ++iter) {
            entlist.push_back(iter->second);
        }
        std::sort(entlist.begin(), entlist.end(), compareEntity);
        for (unsigned int i=0; i<entlist.size(); i++)
            dumpObject(output, entlist[i], saveSceneNames);
        fclose(output);
    }

    bool quat2Euler(Quaternion q, double& pitch, double& roll, double& yaw) {
        /// note that in the 'gymbal lock' situation, we will get nan's for pitch.
        /// for now, in that case we should revert to quaternion
        double q1,q2,q3,q0;
        q2=q.x;
        q3=q.y;
        q1=q.z;
        q0=q.w;
        roll = std::atan2((2*((q0*q1)+(q2*q3))), (1-(2*(std::pow(q1,2.0)+std::pow(q2,2.0)))));
        pitch = std::asin((2*((q0*q2)-(q3*q1))));
        yaw = std::atan2((2*((q0*q3)+(q1*q2))), (1-(2*(std::pow(q2,2.0)+std::pow(q3,2.0)))));
        pitch /= DEG2RAD;
        roll /= DEG2RAD;
        yaw /= DEG2RAD;
        if (std::abs(pitch) > 89.0) {
            return false;
        }
        return true;
    }

    string physicalName(ProxyMeshObject *obj, std::set<std::string> &saveSceneNames) {
        std::string name = obj->getPhysical().name;
        if (name.empty()) {
            name = obj->getMesh().filename();
            name.resize(name.size()-5);
            //name += ".0";
        }
//        if (name.find(".") < name.size()) {             /// remove any enumeration
//            name.resize(name.find("."));
//        }
        int basesize = name.size();
        int count = 1;
        while (saveSceneNames.count(name)) {
            name.resize(basesize);
            std::ostringstream os;
            os << name << "." << count;
            name = os.str();
            count++;
        }
        saveSceneNames.insert(name);
        return name;
    }
    void dumpObject(FILE* fp, Entity* e, std::set<std::string> &saveSceneNames) {
        ProxyObject *pp = e->getProxyPtr().get();
        Time now(mParent->getLocalTimeOffset()->now(*pp));
        Location loc = pp->globalLocation(now);
        ProxyCameraObject* camera = dynamic_cast<ProxyCameraObject*>(pp);
        ProxyLightObject* light = dynamic_cast<ProxyLightObject*>(pp);
        ProxyMeshObject* mesh = dynamic_cast<ProxyMeshObject*>(pp);

        double x,y,z;
        std::string w("");
        /// if feasible, use Eulers: (not feasible == potential gymbal confusion)
        if (!quat2Euler(loc.getOrientation(), x, z, y)) {
            x=loc.getOrientation().x;
            y=loc.getOrientation().y;
            z=loc.getOrientation().z;
            std::stringstream temp;
            temp << loc.getOrientation().w;
            w = temp.str();
        }

        Vector3f angAxis(loc.getAxisOfRotation());
        float angSpeed(loc.getAngularSpeed());

        string parent;
        ProxyObjectPtr parentObj = pp->getParentProxy();
        if (parentObj) {
            ProxyMeshObject *parentMesh = dynamic_cast<ProxyMeshObject*>(parentObj.get());
            if (parentMesh) {
                parent = physicalName(parentMesh, saveSceneNames);
            }
        }
        if (light) {
            const char *typestr = "directional";
            const LightInfo &linfo = light->getLastLightInfo();
            if (linfo.mType == LightInfo::POINT) {
                typestr = "point";
            }
            if (linfo.mType == LightInfo::SPOTLIGHT) {
                typestr = "spotlight";
            }
            float32 ambientPower, shadowPower;
            ambientPower = computeClosestPower(linfo.mDiffuseColor, linfo.mAmbientColor, linfo.mPower);
            shadowPower = computeClosestPower(linfo.mSpecularColor, linfo.mShadowColor,  linfo.mPower);
            fprintf(fp, "light,%s,,%s,,,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f,,,,,,,,,,,,,",typestr,parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,x,y,z,w.c_str(),
                    loc.getVelocity().x, loc.getVelocity().y, loc.getVelocity().z, angAxis.x, angAxis.y, angAxis.z, angSpeed);

            fprintf(fp, "%f,%f,%f,%f,%f,%f,%f,%f,%lf,%f,%f,%f,%f,%f,%f,%f,%d\n",
                    linfo.mDiffuseColor.x,linfo.mDiffuseColor.y,linfo.mDiffuseColor.z,ambientPower,
                    linfo.mSpecularColor.x,linfo.mSpecularColor.y,linfo.mSpecularColor.z,shadowPower,
                    linfo.mLightRange,linfo.mConstantFalloff,linfo.mLinearFalloff,linfo.mQuadraticFalloff,
                    linfo.mConeInnerRadians,linfo.mConeOuterRadians,linfo.mPower,linfo.mConeFalloff,
                    (int)linfo.mCastsShadow);
        }
        else if (mesh) {
            URI uri = mesh->getMesh();
            std::string uristr = uri.toString();
            if (uri.proto().empty()) {
                uristr = "";
            }
            const PhysicalParameters &phys = mesh->getPhysical();
            std::string subtype;
            switch (phys.mode) {
            case PhysicalParameters::Disabled:
                subtype="graphiconly";
                break;
            case PhysicalParameters::Static:
                subtype="staticmesh";
                break;
            case PhysicalParameters::DynamicBox:
                subtype="dynamicbox";
                break;
            case PhysicalParameters::DynamicSphere:
                subtype="dynamicsphere";
                break;
            case PhysicalParameters::DynamicCylinder:
                subtype="dynamiccylinder";
                break;
            case PhysicalParameters::Character:
                subtype="character";
                break;
            default:
                std::cout << "unknown physical mode! " << (int)phys.mode << std::endl;
            }
            std::string name = physicalName(mesh, saveSceneNames);
            fprintf(fp, "mesh,%s,%s,%s,,,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f,",subtype.c_str(),name.c_str(),parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,x,y,z,w.c_str(),
                    loc.getVelocity().x, loc.getVelocity().y, loc.getVelocity().z, angAxis.x, angAxis.y, angAxis.z, angSpeed);

            fprintf(fp, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%s\n",
                    mesh->getScale().x,mesh->getScale().y,mesh->getScale().z,
                    phys.hull.x, phys.hull.y, phys.hull.z,
                    phys.density, phys.friction, phys.bounce, phys.colMask, phys.colMsg, uristr.c_str());
        }
        else if (camera) {
            fprintf(fp, "camera,,,%s,,,%f,%f,%f,%f,%f,%f,%s,%f,%f,%f,%f,%f,%f,%f\n",parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,x,y,z,w.c_str(),
                    loc.getVelocity().x, loc.getVelocity().y, loc.getVelocity().z, angAxis.x, angAxis.y, angAxis.z, angSpeed);
        }
        else {
            fprintf(fp, "#unknown object type in dumpObject\n");
        }
    }

    void zoomAction(float value, Vector2f axes) {
        if (!mParent||!mParent->mPrimaryCamera) return;
        zoomInOut(value, axes, mParent->mPrimaryCamera, mSelectedObjects, mParent);
    }

    ///// Top Level Input Event Handlers //////

    EventResponse keyHandler(EventPtr ev) {
        std::tr1::shared_ptr<ButtonEvent> buttonev (
            std::tr1::dynamic_pointer_cast<ButtonEvent>(ev));
        if (!buttonev) {
            return EventResponse::nop();
        }

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onButton(buttonev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }

    EventResponse axisHandler(EventPtr ev) {
        std::tr1::shared_ptr<AxisEvent> axisev (
            std::tr1::dynamic_pointer_cast<AxisEvent>(ev));
        if (!axisev)
            return EventResponse::nop();

        float multiplier = mParent->mInputManager->mWheelToAxis->as<float>();

        if (axisev->mAxis == SDLMouse::WHEELY) {
            bool used = WebViewManager::getSingleton().injectMouseWheel(WebViewCoord(0, axisev->mValue.getCentered()/multiplier));
            if (used)
                return EventResponse::cancel();
        }
        if (axisev->mAxis == SDLMouse::WHEELX) {
            bool used = WebViewManager::getSingleton().injectMouseWheel(WebViewCoord(axisev->mValue.getCentered()/multiplier, 0));
            if (used)
                return EventResponse::cancel();
        }

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::cancel();
    }

    EventResponse textInputHandler(EventPtr ev) {
        std::tr1::shared_ptr<TextInputEvent> textev (
            std::tr1::dynamic_pointer_cast<TextInputEvent>(ev));
        if (!textev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onKeyTextInput(textev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }

    EventResponse mouseHoverHandler(EventPtr ev) {
        std::tr1::shared_ptr<MouseHoverEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseHoverEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseMove(mouseev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        if (mParent->mPrimaryCamera) {
            CameraEntity *camera = mParent->mPrimaryCamera;
            Time time = Time::local(); // FIXME #116, #117
            int lhc=mLastHitCount;
            mouseOverWebView(camera, time, mouseev->mX, mouseev->mY, false, false);
        }

        return EventResponse::nop();
    }

    EventResponse mousePressedHandler(EventPtr ev) {
        std::tr1::shared_ptr<MousePressedEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MousePressedEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMousePressed(mouseev);
        if (browser_resp == EventResponse::cancel()) {
            mWebViewActiveButtons.insert(mouseev->mButton);
            return EventResponse::cancel();
        }

        if (mParent->mPrimaryCamera) {
            CameraEntity *camera = mParent->mPrimaryCamera;
            Time time = Time::local(); // FIXME #116, #117
            int lhc=mLastHitCount;
            hoverEntity(camera, time, mouseev->mXStart, mouseev->mYStart, true, &lhc, mWhichRayObject);
            mouseOverWebView(camera, time, mouseev->mXStart, mouseev->mYStart, true, false);
        }
        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }

    EventResponse mouseClickHandler(EventPtr ev) {
        std::tr1::shared_ptr<MouseClickEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseClick(mouseev);
        if (mWebViewActiveButtons.find(mouseev->mButton) != mWebViewActiveButtons.end()) {
            mWebViewActiveButtons.erase(mouseev->mButton);
            return EventResponse::cancel();
        }
        if (browser_resp == EventResponse::cancel()) {
            return EventResponse::cancel();
        }
        if (mParent->mPrimaryCamera) {
            CameraEntity *camera = mParent->mPrimaryCamera;
            Time time = Time::local(); // FIXME #116, #117
            int lhc=mLastHitCount;
            mouseOverWebView(camera, time, mouseev->mX, mouseev->mY, false, true);
        }
        mMouseDownObject.reset();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }

    EventResponse mouseDragHandler(EventPtr evbase) {
        MouseDragEventPtr ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (!mParent||!mParent->mPrimaryCamera) return EventResponse::nop();
        std::set<int>::iterator iter = mWebViewActiveButtons.find(ev->mButton);
        if (iter != mWebViewActiveButtons.end()) {
            // Give the browser a chance to use this input
            EventResponse browser_resp = WebViewManager::getSingleton().onMouseDrag(ev);

            if (ev->mType == Input::DRAG_END) {
                mWebViewActiveButtons.erase(iter);
            }

            if (browser_resp == EventResponse::cancel()) {
                return EventResponse::cancel();
            }
        }

        if (mParent->mPrimaryCamera) {
            CameraEntity *camera = mParent->mPrimaryCamera;
            Time time = Time::local(); // FIXME #116, #117
            int lhc=mLastHitCount;
            mouseOverWebView(camera, time, ev->mX, ev->mY, false, ev->mType == Input::DRAG_END);
        }
        if (ev->mType == Input::DRAG_END) {
            mMouseDownObject.reset();
        }

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(evbase));
        mInputBinding.handle(inputev);

        ActiveDrag * &drag = mActiveDrag[ev->mButton];
        if (ev->mType == Input::DRAG_START) {
            if (drag) {
                delete drag;
            }
            DragStartInfo info = {
                /*.sys = */ mParent,
                /*.camera = */ mParent->mPrimaryCamera, // for now...
                /*.objects = */ mSelectedObjects,
                /*.ev = */ ev
            };
            if (mDragAction[ev->mButton]) {
                drag = mDragAction[ev->mButton](info);
            }
        }
        if (drag) {
            drag->mouseMoved(ev);

            if (ev->mType == Input::DRAG_END) {
                delete drag;
                drag = 0;
            }
        }
        return EventResponse::nop();
    }

    EventResponse webviewHandler(EventPtr ev) {
        WebViewEventPtr webview_ev (std::tr1::dynamic_pointer_cast<WebViewEvent>(ev));
        if (!webview_ev)
            return EventResponse::nop();

        // For everything else we let the browser go first, but in this case it should have
        // had its chance, so we just let it go
        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }


    /// Camera Path Utilities
    void cameraPathSetCamera(const Vector3d& pos, const Quaternion& orient) {
        if (!mParent||!mParent->mPrimaryCamera) return;
        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;
        Time now(mParent->getLocalTimeOffset()->now(*cam));
        Location loc = cam->extrapolateLocation(now);

        loc.setPosition( pos );
        loc.setOrientation( orient );
        loc.setVelocity(Vector3f(0,0,0));
        loc.setAngularSpeed(0);
        VWObjectPtr cam_vwobj = cam->getOwner();
        SpaceID space = cam->getObjectReference().space();
        if (cam_vwobj->id(space) != cam->getObjectReference()) return;
        Location oldloc = cam->extrapolateLocation(now);
        cam->setOrientation(TimedMotionQuaternion(now,MotionQuaternion(loc.getOrientation(), Quaternion(Vector3f(1,0,0),0))));
        TimedMotionVector3f newplace(now,MotionVector3f(Vector3f(oldloc.getPosition()),Vector3f(pos-oldloc.getPosition())));
        cam->setLocation(newplace);
        cam_vwobj->requestLocationUpdate(space, newplace);
    }

    void cameraPathSetToKeyFrame(uint32 idx) {
        if (idx < 0 || idx >= mCameraPath.numPoints()) return;

        CameraPoint keypoint = mCameraPath[idx];
        cameraPathSetCamera(mCameraPath[idx].position, mCameraPath[idx].orientation);
        mCameraPathTime = mCameraPath[idx].time;
    }

#define CAMERA_PATH_FILE "camera_path.txt"

    void cameraPathLoad() {
        mCameraPath.load(CAMERA_PATH_FILE);
    }

    void cameraPathSave() {
        mCameraPath.save(CAMERA_PATH_FILE);
    }

    void cameraPathNext() {
        mCameraPathIndex = mCameraPath.clampKeyIndex(mCameraPathIndex+1);
        cameraPathSetToKeyFrame(mCameraPathIndex);
    }

    void cameraPathPrevious() {
        mCameraPathIndex = mCameraPath.clampKeyIndex(mCameraPathIndex-1);
        cameraPathSetToKeyFrame(mCameraPathIndex);
    }

    void cameraPathInsert() {
        if (!mParent||!mParent->mPrimaryCamera) return;
        ProxyObjectPtr cam = getTopLevelParent(mParent->mPrimaryCamera->getProxyPtr());
        if (!cam) return;
        Time now(mParent->getLocalTimeOffset()->now(*cam));
        Location loc = cam->extrapolateLocation(now);

        mCameraPathIndex = mCameraPath.insert(mCameraPathIndex, loc.getPosition(), loc.getOrientation(), Task::DeltaTime::seconds(1.0));
        mCameraPathTime = mCameraPath.keyFrameTime(mCameraPathIndex);
    }

    void cameraPathDelete() {
        mCameraPathIndex = mCameraPath.remove(mCameraPathIndex);
        mCameraPathTime = mCameraPath.keyFrameTime(mCameraPathIndex);
    }

    void cameraPathRun() {
        mRunningCameraPath = !mRunningCameraPath;
        if (mRunningCameraPath)
            mCameraPathTime = Task::DeltaTime::zero();
    }

    void cameraPathChangeSpeed(float factor) {
        mCameraPath.changeTimeDelta(mCameraPathIndex, Task::DeltaTime::seconds(factor));
    }

    void cameraPathTick(const Task::LocalTime& t) {
        Task::DeltaTime dt = t - mLastCameraTime;
        mLastCameraTime = t;

        if (mRunningCameraPath) {
            mCameraPathTime += dt;

            Vector3d pos;
            Quaternion orient;
            bool success = mCameraPath.evaluate(mCameraPathTime, &pos, &orient);

            if (!success) {
                mRunningCameraPath = false;
                return;
            }

            cameraPathSetCamera(pos, orient);
        }
    }

    void fpsUpdateTick(const Task::LocalTime& t) {
        if(mFPSWidgetView) {
            Task::DeltaTime dt = t - mLastFpsTime;
            if(dt.toSeconds() > 1) {
                mLastFpsTime = t;
                Ogre::RenderTarget::FrameStats stats = mParent->getRenderTarget()->getStatistics();
                ostringstream os;
                os << stats.avgFPS;
                mFPSWidgetView->evaluateJS("update_fps(" + os.str() + ")");
            }
        }
    }

    void screenshotTick(const Task::LocalTime& t) {
        if (mPeriodicScreenshot && (t-mLastScreenshotTime > Task::DeltaTime::seconds(1.0))) {
            timedScreenshotAction(t);
            mLastScreenshotTime = t;
        }
    }

    /// WebView Actions
    void webViewNavigateAction(WebViewManager::NavigationAction action) {
        WebViewManager::getSingleton().navigate(action);
    }

    void webViewNavigateStringAction(WebViewManager::NavigationAction action, const String& arg) {
        WebViewManager::getSingleton().navigate(action, arg);
    }

    ///////////////// DEVICE FUNCTIONS ////////////////

    EventResponse deviceListener(EventPtr evbase) {
        std::tr1::shared_ptr<InputDeviceEvent> ev (std::tr1::dynamic_pointer_cast<InputDeviceEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        switch (ev->mType) {
        case InputDeviceEvent::ADDED:
            if (!!(std::tr1::dynamic_pointer_cast<SDLMouse>(ev->mDevice))) {
                SubscriptionId subId = mParent->mInputManager->subscribeId(
                    AxisEvent::getEventId(),
                    std::tr1::bind(&MouseHandler::axisHandler, this, _1));
                mEvents.push_back(subId);
                mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
            }
            if (!!(std::tr1::dynamic_pointer_cast<SDLKeyboard>(ev->mDevice))) {
                // Key Pressed
                {
                    SubscriptionId subId = mParent->mInputManager->subscribeId(
                        ButtonPressed::getEventId(),
                        std::tr1::bind(&MouseHandler::keyHandler, this, _1)
                    );
                    mEvents.push_back(subId);
                    mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
                }
                // Key Released
                {
                    SubscriptionId subId = mParent->mInputManager->subscribeId(
                        ButtonReleased::getEventId(),
                        std::tr1::bind(&MouseHandler::keyHandler, this, _1)
                    );
                    mEvents.push_back(subId);
                    mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
                }
            }
            break;
        case InputDeviceEvent::REMOVED: {
            DeviceSubMap::iterator iter;
            while ((iter = mDeviceSubscriptions.find(&*ev->mDevice))!=mDeviceSubscriptions.end()) {
                mParent->mInputManager->unsubscribe((*iter).second);
                mDeviceSubscriptions.erase(iter);
            }
        }
        break;
        }
        return EventResponse::nop();
    }

public:
    MouseHandler(OgreSystem *parent)
     : mParent(parent),
       mScreenshotID(0),
       mPeriodicScreenshot(false),
       mScreenshotStartTime(Task::LocalTime::now()),
       mLastScreenshotTime(Task::LocalTime::now()),
       mCurrentGroup(SpaceObjectReference::null()),
       mLastCameraTime(Task::LocalTime::now()),
       mLastFpsTime(Task::LocalTime::now()),
       mUploadWebView(NULL),
       mFPSWidgetView(NULL),
       mQueryAngleWidgetView(NULL),
       mNewQueryAngle(0.f),
       mQueryAngleTimer( Network::IOTimer::create(parent->mContext->ioService, std::tr1::bind(&MouseHandler::handleSetQueryAngleTimeout, this)) ),
       mWhichRayObject(0)
    {
        mLastHitCount=0;
        mLastHitX=0;
        mLastHitY=0;

        cameraPathLoad();
        mRunningCameraPath = false;
        mCameraPathIndex = 0;
        mCameraPathTime = mCameraPath.keyFrameTime(0);

        mEvents.push_back(mParent->mInputManager->registerDeviceListener(
                              std::tr1::bind(&MouseHandler::deviceListener, this, _1)));

        mDragAction[1] = 0;
        mDragAction[2] = DragActionRegistry::get("zoomCamera");
        mDragAction[3] = DragActionRegistry::get("panCamera");
        mDragAction[4] = DragActionRegistry::get("rotateCamera");

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseHoverEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseHoverHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MousePressedEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mousePressedHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseDragEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseDragHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseClickEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseClickHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                TextInputEvent::getEventId(),
                std::tr1::bind(&MouseHandler::textInputHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                WebViewEvent::Id,
                std::tr1::bind(&MouseHandler::webviewHandler, this, _1)));

        mInputResponses["screenshot"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::screenshotAction, this));
        mInputResponses["togglePeriodicScreenshot"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::togglePeriodicScreenshotAction, this));
        mInputResponses["quit"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::quitAction, this));

        mInputResponses["moveForward"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(0, 0, -1), _1), 1, 0);
        mInputResponses["moveBackward"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(0, 0, 1), _1), 1, 0);
        mInputResponses["moveLeft"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(-1, 0, 0), _1), 1, 0);
        mInputResponses["moveRight"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(1, 0, 0), _1), 1, 0);
        mInputResponses["moveDown"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(0, -1, 0), _1), 1, 0);
        mInputResponses["moveUp"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::moveAction, this, Vector3f(0, 1, 0), _1), 1, 0);

        mInputResponses["rotateXPos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(1, 0, 0), _1), 1, 0);
        mInputResponses["rotateXNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(-1, 0, 0), _1), 1, 0);
        mInputResponses["rotateYPos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(0, 1, 0), _1), 1, 0);
        mInputResponses["rotateYNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(0, -1, 0), _1), 1, 0);
        mInputResponses["rotateZPos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(0, 0, 1), _1), 1, 0);
        mInputResponses["rotateZNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::rotateAction, this, Vector3f(0, 0, -1), _1), 1, 0);

        mInputResponses["stableRotatePos"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::stableRotateAction, this, 1.f, _1), 1, 0);
        mInputResponses["stableRotateNeg"] = new FloatToggleInputResponse(std::tr1::bind(&MouseHandler::stableRotateAction, this, -1.f, _1), 1, 0);

        mInputResponses["createWebview"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::createWebviewAction, this));


        mInputResponses["openScriptingUI"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::createScriptingUIAction, this));
        mInputResponses["openObjectUI"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::createUIAction, this, "object/object.html"));


        mInputResponses["createScriptedObject"] = new StringMapInputResponse(std::tr1::bind(&MouseHandler::createScriptedObjectAction, this, _1));
        mInputResponses["executeScript"] = new WebViewStringMapInputResponse(std::tr1::bind(&MouseHandler::executeScript, this, _1, _2));

        mInputResponses["closeWebView"] = new WebViewStringMapInputResponse(std::tr1::bind(&MouseHandler::closeWebView, this, _1, _2));

        mInputResponses["createLight"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::createLightAction, this));
        mInputResponses["enterObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::enterObjectAction, this));
        mInputResponses["leaveObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::leaveObjectAction, this));
        mInputResponses["groupObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::groupObjectsAction, this));
        mInputResponses["ungroupObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::ungroupObjectsAction, this));
        mInputResponses["deleteObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::deleteObjectsAction, this));
        mInputResponses["cloneObjects"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cloneObjectsAction, this));
        mInputResponses["import"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::importAction, this));
        mInputResponses["saveScene"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::saveSceneAction, this));

        mInputResponses["selectObject"] = new Vector2fInputResponse(std::tr1::bind(&MouseHandler::selectObjectAction, this, _1, 1));
        mInputResponses["selectObjectReverse"] = new Vector2fInputResponse(std::tr1::bind(&MouseHandler::selectObjectAction, this, _1, -1));

        mInputResponses["zoom"] = new AxisInputResponse(std::tr1::bind(&MouseHandler::zoomAction, this, _1, _2));

        mInputResponses["setDragModeNone"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, ""));
        mInputResponses["setDragModeMoveObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "moveObject"));
        mInputResponses["setDragModeRotateObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "rotateObject"));
        mInputResponses["setDragModeScaleObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "scaleObject"));
        mInputResponses["setDragModeRotateCamera"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "rotateCamera"));
        mInputResponses["setDragModePanCamera"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::setDragModeAction, this, "panCamera"));

        mInputResponses["cameraPathLoad"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathLoad, this));
        mInputResponses["cameraPathSave"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathSave, this));
        mInputResponses["cameraPathNextKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathNext, this));
        mInputResponses["cameraPathPreviousKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathPrevious, this));
        mInputResponses["cameraPathInsertKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathInsert, this));
        mInputResponses["cameraPathDeleteKeyFrame"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathDelete, this));
        mInputResponses["cameraPathRun"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathRun, this));
        mInputResponses["cameraPathSpeedUp"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathChangeSpeed, this, -0.1f));
        mInputResponses["cameraPathSlowDown"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::cameraPathChangeSpeed, this, 0.1f));

        mInputResponses["webNewTab"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateNewTab));
        mInputResponses["webBack"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateBack));
        mInputResponses["webForward"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateForward));
        mInputResponses["webRefresh"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateRefresh));
        mInputResponses["webHome"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateHome));
        mInputResponses["webGo"] = new StringInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateStringAction, this, WebViewManager::NavigateGo, _1));
        mInputResponses["webDelete"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateAction, this, WebViewManager::NavigateDelete));

        mInputResponses["webCommand"] = new StringInputResponse(std::tr1::bind(&MouseHandler::webViewNavigateStringAction, this, WebViewManager::NavigateCommand, _1));


        mInputResponses["startUploadObject"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::startUploadObject, this));
        mInputResponses["handleFPSWidget"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::handleFPSWidget, this));
        mInputResponses["handleQueryAngleWidget"] = new SimpleInputResponse(std::tr1::bind(&MouseHandler::handleQueryAngleWidget, this));


        // Session
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_ESCAPE), mInputResponses["quit"]);

        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_I), mInputResponses["screenshot"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_I, Input::MOD_CTRL), mInputResponses["togglePeriodicScreenshot"]);

        // Movement
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W), mInputResponses["moveForward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S), mInputResponses["moveBackward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_D), mInputResponses["moveRight"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_A), mInputResponses["moveLeft"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_UP, Input::MOD_SHIFT), mInputResponses["rotateXPos"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DOWN, Input::MOD_SHIFT), mInputResponses["rotateXNeg"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_UP), mInputResponses["moveForward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DOWN), mInputResponses["moveBackward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Q), mInputResponses["moveUp"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Z), mInputResponses["moveDown"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_LEFT), mInputResponses["stableRotatePos"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_RIGHT), mInputResponses["stableRotateNeg"]);

        // Various other actions
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_N, Input::MOD_CTRL), mInputResponses["createWebview"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_N, Input::MOD_ALT), mInputResponses["openObjectUI"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S, Input::MOD_ALT), mInputResponses["openScriptingUI"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_B), mInputResponses["createLight"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_ENTER), mInputResponses["enterObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_RETURN), mInputResponses["enterObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_0), mInputResponses["leaveObject"]);
        //mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_ESCAPE), mInputResponses["leaveObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_G), mInputResponses["groupObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_G, Input::MOD_ALT), mInputResponses["ungroupObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DELETE), mInputResponses["deleteObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_V, Input::MOD_CTRL), mInputResponses["cloneObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_O, Input::MOD_CTRL), mInputResponses["import"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S, Input::MOD_CTRL), mInputResponses["saveScene"]);

        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_U, Input::MOD_CTRL), mInputResponses["startUploadObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_F, Input::MOD_CTRL), mInputResponses["handleFPSWidget"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_A, Input::MOD_CTRL), mInputResponses["handleQueryAngleWidget"]);


        // Drag modes
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Q, Input::MOD_CTRL), mInputResponses["setDragModeNone"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W, Input::MOD_CTRL), mInputResponses["setDragModeMoveObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_E, Input::MOD_CTRL), mInputResponses["setDragModeRotateObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_R, Input::MOD_CTRL), mInputResponses["setDragModeScaleObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_T, Input::MOD_CTRL), mInputResponses["setDragModeRotateCamera"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Y, Input::MOD_CTRL), mInputResponses["setDragModePanCamera"]);

        // Mouse Zooming
        mInputBinding.add(InputBindingEvent::Axis(SDLMouse::WHEELY), mInputResponses["zoom"]);

        // Selection
        mInputBinding.add(InputBindingEvent::MouseClick(1), mInputResponses["selectObject"]);
        mInputBinding.add(InputBindingEvent::MouseClick(3), mInputResponses["selectObjectReverse"]);

        // Camera Path
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_1), mInputResponses["cameraPathLoad"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_2), mInputResponses["cameraPathSave"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_3), mInputResponses["cameraPathNextKeyFrame"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_4), mInputResponses["cameraPathPreviousKeyFrame"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_5), mInputResponses["cameraPathInsertKeyFrame"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_6), mInputResponses["cameraPathDeleteKeyFrame"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_7), mInputResponses["cameraPathRun"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_8), mInputResponses["cameraPathSpeedUp"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_9), mInputResponses["cameraPathSlowDown"]);

        // WebView Chrome
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navnewtab"), mInputResponses["webNewTab"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navback"), mInputResponses["webBack"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navforward"), mInputResponses["webForward"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navrefresh"), mInputResponses["webRefresh"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navhome"), mInputResponses["webHome"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navgo"), mInputResponses["webGo"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navdel"), mInputResponses["webDelete"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navmoveforward"), mInputResponses["moveForward"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navturnleft"), mInputResponses["rotateYPos"]);
        mInputBinding.add(InputBindingEvent::Web("__chrome", "navturnright"), mInputResponses["rotateYNeg"]);

        mInputBinding.add(InputBindingEvent::Web("__chrome", "navcommand"), mInputResponses["webCommand"]);

        mInputBinding.add(InputBindingEvent::Web("__object", "CreateScriptedObject"), mInputResponses["createScriptedObject"]);

        mInputBinding.add(InputBindingEvent::Web("__scripting", "ExecScript"), mInputResponses["executeScript"]);
        mInputBinding.add(InputBindingEvent::Web("__scripting", "Close"), mInputResponses["closeWebView"]);
    }

    ~MouseHandler() {
        for (std::vector<SubscriptionId>::const_iterator iter = mEvents.begin();
             iter != mEvents.end();
             ++iter) {
            mParent->mInputManager->unsubscribe(*iter);
        }
        for (InputResponseMap::iterator iter=mInputResponses.begin(),iterend=mInputResponses.end();iter!=iterend;++iter) {
            delete iter->second;
        }
        for (std::map<int, ActiveDrag*>::iterator iter=mActiveDrag.begin(),iterend=mActiveDrag.end();iter!=iterend;++iter) {
            if(iter->second!=NULL)
                delete iter->second;
        }
    }
    void setParentGroupAndClear(const SpaceObjectReference &id) {
        clearSelection();
        mCurrentGroup = id;
    }
    const SpaceObjectReference &getParentGroup() const {
        return mCurrentGroup;
    }
    void addToSelection(const ProxyObjectPtr &obj) {
        mSelectedObjects.insert(obj);
    }

    void tick(const Task::LocalTime& t) {
        cameraPathTick(t);
        fpsUpdateTick(t);
        screenshotTick(t);
    }
};

void OgreSystem::allocMouseHandler() {
    mMouseHandler = new MouseHandler(this);
}
void OgreSystem::destroyMouseHandler() {
    if (mMouseHandler) {
        delete mMouseHandler;
    }
}

void OgreSystem::selectObject(Entity *obj, bool replace) {
    if (replace) {
        mMouseHandler->setParentGroupAndClear(obj->getProxy().getParentProxy()->getObjectReference());
    }
    if (mMouseHandler->getParentGroup() == obj->getProxy().getParentProxy()->getObjectReference()) {
        mMouseHandler->addToSelection(obj->getProxyPtr());
        obj->setSelected(true);
    }
}

void OgreSystem::tickInputHandler(const Task::LocalTime& t) const {
    if (mMouseHandler != NULL)
        mMouseHandler->tick(t);
}

}
}
