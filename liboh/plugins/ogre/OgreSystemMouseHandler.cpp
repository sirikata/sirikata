/*  Sirikata liboh -- Ogre Graphics Plugin
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

#include <util/Standard.hh>
#include <oh/Platform.hpp>
#include "OgreSystem.hpp"
#include "CameraEntity.hpp"
#include "LightEntity.hpp"
#include "MeshEntity.hpp"
#include "input/SDLInputManager.hpp"
#include <oh/ProxyManager.hpp>
#include <oh/ProxyObject.hpp>
#include <oh/ProxyMeshObject.hpp>
#include <oh/ProxyLightObject.hpp>
#include "input/InputEvents.hpp"
#include "input/SDLInputDevice.hpp"
#include "DragActions.hpp"
#include "InputResponse.hpp"
#include "InputBinding.hpp"
#include <task/Event.hpp>
#include <task/Time.hpp>
#include <task/EventManager.hpp>
#include <SDL_keysym.h>
#include <set>

#include "WebViewManager.hpp"

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
    Task::AbsTime now = Task::AbsTime::now();
    ProxyObject *pp = one->getProxyPtr().get();
    Location loc1 = pp->globalLocation(now);
    ProxyCameraObject* camera1 = dynamic_cast<ProxyCameraObject*>(pp);
    ProxyLightObject* light1 = dynamic_cast<ProxyLightObject*>(pp);
    ProxyMeshObject* mesh1 = dynamic_cast<ProxyMeshObject*>(pp);
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

    SpaceObjectReference mCurrentGroup;
    typedef std::set<ProxyObjectWPtr> SelectedObjectSet;
    SelectedObjectSet mSelectedObjects;
    SpaceObjectReference mLastShiftSelected;
    int mLastHitCount;
    float mLastHitX;
    float mLastHitY;
    // map from mouse button to drag for that mouse button.
    /* as far as multiple cursors are concerned,
       each cursor should have its own MouseHandler instance */
    std::map<int, DragAction> mDragAction;
    std::map<int, ActiveDrag*> mActiveDrag;
    /*
        typedef EventResponse (MouseHandler::*ClickAction) (EventPtr evbase);
        std::map<int, ClickAction> mClickAction;
    */

    typedef std::map<String, InputResponse*> InputResponseMap;
    InputResponseMap mInputResponses;

    InputBinding mInputBinding;

    class SubObjectIterator {
        typedef Entity* value_type;
        //typedef ssize_t difference_type;
        typedef size_t size_type;
        OgreSystem::SceneEntitiesMap::const_iterator mIter;
        Entity *mParentEntity;
        OgreSystem *mOgreSys;
        void findNext() {
            while (!end() && !((*mIter).second->getProxy().getParent() == mParentEntity->id())) {
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

    Entity *hoverEntity (CameraEntity *cam, Task::AbsTime time, float xPixel, float yPixel, int *hitCount,int which=0) {
        Location location(cam->getProxy().globalLocation(time));
        Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
        SILOG(input,info,"X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<location.getPosition()<<"; dir = "<<dir);

        double dist;
        Vector3f normal;
        Entity *mouseOverEntity = mParent->rayTrace(location.getPosition(), dir, *hitCount, dist, normal, which);
        if (mouseOverEntity) {
            while (!(mouseOverEntity->getProxy().getParent() == mCurrentGroup)) {
                mouseOverEntity = mParent->getEntity(mouseOverEntity->getProxy().getParent());
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
        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) {
            return;
        }
        if (mParent->mInputManager->isModifierDown(Input::MOD_SHIFT)) {
            // add object.
            int numObjectsUnderCursor=0;
            Entity *mouseOver = hoverEntity(camera, Task::AbsTime::now(), p.x, p.y, &numObjectsUnderCursor, mWhichRayObject);
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
            mouseOver = hoverEntity(camera, Task::AbsTime::now(), p.x, p.y, &mLastHitCount, mWhichRayObject);
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
            mWhichRayObject+=direction;
            int numObjectsUnderCursor=0;
            Entity *mouseOver = hoverEntity(camera, Task::AbsTime::now(), p.x, p.y, &numObjectsUnderCursor, mWhichRayObject);
            if (recentMouseInRange(p.x, p.y, &mLastHitX, &mLastHitY)==false||numObjectsUnderCursor!=mLastHitCount){
                mouseOver = hoverEntity(camera, Task::AbsTime::now(), p.x, p.y, &mLastHitCount, mWhichRayObject=0);
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
        Task::AbsTime now(Task::AbsTime::now());
        while (doUngroupObjects(now)) {
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (ent) {
                ent->getProxy().getProxyManager()->destroyObject(ent->getProxyPtr());
            }
        }
        mSelectedObjects.clear();
    }

    Entity *doCloneObject(Entity *ent, const ProxyObjectPtr &parentPtr, Task::AbsTime now) {
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
            std::tr1::shared_ptr<ProxyMeshObject> newMeshObject (new ProxyMeshObject(proxyMgr, newId));
            newObj = newMeshObject;
            proxyMgr->createObject(newMeshObject);
            newMeshObject->setMesh(meshObj->getMesh());
            newMeshObject->setScale(meshObj->getScale());
        }
        else if (lightObj) {
            std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId));
            newObj = newLightObject;
            proxyMgr->createObject(newLightObject);
            newLightObject->update(lightObj->getLastLightInfo());
        }
        else {
            newObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newId));
            proxyMgr->createObject(newObj);
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
    }

    void cloneObjectsAction() {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::AbsTime now(Task::AbsTime::now());
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) {
                continue;
            }
            Entity *newEnt = doCloneObject(ent, ent->getProxy().getParentProxy(), now);
            Location loc (ent->getProxy().extrapolateLocation(now));
            loc.setPosition(loc.getPosition() + Vector3d(WORLD_SCALE/2.,0,0));
            newEnt->getProxy().resetLocation(now, loc);
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
        Task::AbsTime now(Task::AbsTime::now());
        ProxyManager *proxyMgr = mParent->mPrimaryCamera->getProxy().getProxyManager();
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) continue;
            if (ent->getProxy().getProxyManager() != proxyMgr) {
                SILOG(input,error,"Attempting to group objects owned by different proxy manager!");
                return;
            }
            if (!(ent->getProxy().getParent() == parentId)) {
                SILOG(input,error,"Multiple select "<< ent->id() <<
                      " has parent  "<<ent->getProxy().getParent() << " instead of " << mCurrentGroup);
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
        proxyMgr->createObject(ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newParentId)));
        Entity *newParentEntity = mParent->getEntity(newParentId);
        newParentEntity->getProxy().resetLocation(now, totalLocation);

        if (parentEntity) {
            newParentEntity->getProxy().setParent(parentEntity->getProxyPtr(), now);
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
                iter != mSelectedObjects.end(); ++iter) {
            ProxyObjectPtr obj(iter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (!ent) continue;
            ent->getProxy().setParent(newParentEntity->getProxyPtr(), now);
            ent->setSelected(false);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(newParentEntity->getProxyPtr());
        newParentEntity->setSelected(true);
    }

    bool doUngroupObjects(Task::AbsTime now) {
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
            mCurrentGroup = parentEnt->getProxy().getParent(); // parentParent may be NULL.
            bool hasSubObjects = false;
            for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
                hasSubObjects = true;
                Entity *ent = *subIter;
                ent->getProxy().setParent(parentParent, now);
                newSelectedObjects.insert(ent->getProxyPtr());
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                parentEnt->setSelected(false);
                proxyMgr->destroyObject(parentEnt->getProxyPtr());
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
        Task::AbsTime now(Task::AbsTime::now());
        doUngroupObjects(now);
    }

    void enterObjectAction() {
        Task::AbsTime now(Task::AbsTime::now());
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
        Task::AbsTime now(Task::AbsTime::now());
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
            mCurrentGroup = ent->getProxy().getParent();
            Entity *parentEnt = mParent->getEntity(mCurrentGroup);
            if (parentEnt) {
                mSelectedObjects.insert(parentEnt->getProxyPtr());
            }
        }
        else {
            mCurrentGroup = SpaceObjectReference::null();
        }
    }

    void createLightAction() {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::AbsTime now(Task::AbsTime::now());
        CameraEntity *camera = mParent->mPrimaryCamera;
        if (!camera) return;
        SpaceObjectReference newId = SpaceObjectReference(camera->id().space(), ObjectReference(UUID::random()));
        ProxyManager *proxyMgr = camera->getProxy().getProxyManager();
        Location loc (camera->getProxy().globalLocation(now));
        loc.setPosition(loc.getPosition() + Vector3d(direction(loc.getOrientation()))*WORLD_SCALE);
        loc.setOrientation(Quaternion(0.886995, 0.000000, -0.461779, 0.000000, Quaternion::WXYZ()));

        std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId));
        proxyMgr->createObject(newLightObject);
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
            /* set li according to some sample light in the scene file! */
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
    }

    void moveAction(Vector3f dir, float amount) {
        Task::AbsTime now(Task::AbsTime::now());
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *cam = mParent->mPrimaryCamera;
        if (!cam) return;
        Location loc = cam->getProxy().extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        loc.setVelocity((orient * dir) * amount * WORLD_SCALE);
        loc.setAngularSpeed(0);

        cam->getProxy().setLocation(now, loc);
    }

    void rotateAction(Vector3f about, float amount) {
        Task::AbsTime now(Task::AbsTime::now());
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *cam = mParent->mPrimaryCamera;
        if (!cam) return;
        Location loc = cam->getProxy().extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        loc.setAxisOfRotation(about);
        loc.setAngularSpeed(amount);
        loc.setVelocity(Vector3f(0,0,0));

        cam->getProxy().setLocation(now, loc);
    }

    void stableRotateAction(float dir, float amount) {
        Task::AbsTime now(Task::AbsTime::now());
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();

        CameraEntity *cam = mParent->mPrimaryCamera;
        if (!cam) return;
        Location loc = cam->getProxy().extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        double p, r, y;
        quat2Euler(orient, p, r, y);
        Vector3f raxis;
        raxis.x = 0;
        raxis.y = std::cos(p*DEG2RAD);
        raxis.z = -std::sin(p*DEG2RAD);

        loc.setAxisOfRotation(raxis);
        loc.setAngularSpeed(dir*amount);
        loc.setVelocity(Vector3f(0,0,0));

        cam->getProxy().setLocation(now, loc);
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
        fflush(stdin);
        while (!feof(stdin)) {
            int c = fgetc(stdin);
            if (c == '\r') {
                c = fgetc(stdin);
            }
            if (c=='\n') {
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
        std::vector<std::string> files;
        files.push_back(filename);
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
        fprintf(output, "objtype,subtype,name,parent,");
        fprintf(output, "pos_x,pos_y,pos_z,orient_x,orient_y,orient_z,orient_w,scale_x,scale_y,scale_z,hull_x,hull_y,hull_z,");
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
        Task::AbsTime now = Task::AbsTime::now();
        ProxyObject *pp = e->getProxyPtr().get();
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
            ambientPower = LightEntity::computeClosestPower(linfo.mDiffuseColor, linfo.mAmbientColor, linfo.mPower);
            shadowPower = LightEntity::computeClosestPower(linfo.mSpecularColor, linfo.mShadowColor,  linfo.mPower);
            fprintf(fp, "light,%s,,%s,%f,%f,%f,%f,%f,%f,%s,,,,,,,,,,,,,",typestr,parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,
                    x,y,z,w.c_str());

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
            default:
                std::cout << "unknown physical mode! " << (int)phys.mode << std::endl;
            }
            std::string name = physicalName(mesh, saveSceneNames);
            fprintf(fp, "mesh,%s,%s,%s,%f,%f,%f,%f,%f,%f,%s,",subtype.c_str(),name.c_str(),parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,
                    x,y,z,w.c_str());

            fprintf(fp, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%s\n",
                    mesh->getScale().x,mesh->getScale().y,mesh->getScale().z,
                    phys.hull.x, phys.hull.y, phys.hull.z,
                    phys.density, phys.friction, phys.bounce, phys.colMask, phys.colMsg, uristr.c_str());
        }
        else if (camera) {
            fprintf(fp, "camera,,,%s,%f,%f,%f,%f,%f,%f,%s\n",parent.c_str(),
                    loc.getPosition().x,loc.getPosition().y,loc.getPosition().z,
                                    x,y,z,w.c_str());
        }
        else {
            fprintf(fp, "#unknown object type in dumpObject\n");
        }
    }

    void zoomAction(float value, Vector2f axes) {
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

        return EventResponse::nop();
    }

    EventResponse mouseClickHandler(EventPtr ev) {
        std::tr1::shared_ptr<MouseClickEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
        if (!mouseev)
            return EventResponse::nop();

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseClick(mouseev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

        InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
        mInputBinding.handle(inputev);

        return EventResponse::nop();
    }

    EventResponse mouseDragHandler(EventPtr evbase) {
        MouseDragEventPtr ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }

        // Give the browsers a chance to use this input first
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseDrag(ev);
        if (browser_resp == EventResponse::cancel())
            return EventResponse::cancel();

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
    MouseHandler(OgreSystem *parent) : mParent(parent), mCurrentGroup(SpaceObjectReference::null()), mWhichRayObject(0) {
        mLastHitCount=0;
        mLastHitX=0;
        mLastHitY=0;

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
                MouseDragEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseDragHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                MouseClickEvent::getEventId(),
                std::tr1::bind(&MouseHandler::mouseClickHandler, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                TextInputEvent::getEventId(),
                std::tr1::bind(&MouseHandler::textInputHandler, this, _1)));

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

        // Movement
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W, Input::MOD_SHIFT), mInputResponses["moveForward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S, Input::MOD_SHIFT), mInputResponses["moveBackward"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_D, Input::MOD_SHIFT), mInputResponses["moveLeft"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_A, Input::MOD_SHIFT), mInputResponses["moveRight"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_UP), mInputResponses["rotateXPos"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DOWN), mInputResponses["rotateXNeg"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_LEFT), mInputResponses["stableRotatePos"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_RIGHT), mInputResponses["stableRotateNeg"]);
        // Various other actions
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_B), mInputResponses["createLight"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_ENTER), mInputResponses["enterObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_RETURN), mInputResponses["enterObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_0), mInputResponses["leaveObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_ESCAPE), mInputResponses["leaveObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_G), mInputResponses["groupObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_G, Input::MOD_ALT), mInputResponses["ungroupObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_DELETE), mInputResponses["deleteObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_KP_PERIOD), mInputResponses["deleteObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_V, Input::MOD_CTRL), mInputResponses["cloneObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_D), mInputResponses["cloneObjects"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_O, Input::MOD_CTRL), mInputResponses["import"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_S, Input::MOD_CTRL), mInputResponses["saveScene"]);
        // Drag modes
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Q), mInputResponses["setDragModeNone"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_W), mInputResponses["setDragModeMoveObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_E), mInputResponses["setDragModeRotateObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_R), mInputResponses["setDragModeScaleObject"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_T), mInputResponses["setDragModeRotateCamera"]);
        mInputBinding.add(InputBindingEvent::Key(SDL_SCANCODE_Y), mInputResponses["setDragModePanCamera"]);

        // Mouse Zooming
        mInputBinding.add(InputBindingEvent::Axis(SDLMouse::WHEELY), mInputResponses["zoom"]);

        // Selection
        mInputBinding.add(InputBindingEvent::MouseClick(1), mInputResponses["selectObject"]);
        mInputBinding.add(InputBindingEvent::MouseClick(3), mInputResponses["selectObjectReverse"]);
    }
    ~MouseHandler() {
        for (std::vector<SubscriptionId>::const_iterator iter = mEvents.begin();
             iter != mEvents.end();
             ++iter) {
            mParent->mInputManager->unsubscribe(*iter);
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
        mMouseHandler->setParentGroupAndClear(obj->getProxy().getParent());
    }
    if (mMouseHandler->getParentGroup() == obj->getProxy().getParent()) {
        mMouseHandler->addToSelection(obj->getProxyPtr());
        obj->setSelected(true);
    }
}

}
}
