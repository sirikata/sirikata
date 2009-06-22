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
#include <oh/ProxyPositionObject.hpp>
#include <oh/ProxyMeshObject.hpp>
#include <oh/ProxyLightObject.hpp>
#include "input/InputEvents.hpp"
#include "input/SDLInputDevice.hpp"
#include "DragActions.hpp"
#include <task/Event.hpp>
#include <task/Time.hpp>
#include <task/EventManager.hpp>
#include <SDL_keysym.h>


#include <set>


namespace Sirikata {
namespace Graphics {
using namespace Input;
using namespace Task;

// Defined in DragActions.cpp.

class OgreSystem::MouseHandler {
    OgreSystem *mParent;
    std::vector<SubscriptionId> mEvents;
    typedef std::multimap<InputDevice*, SubscriptionId> DeviceSubMap;
    DeviceSubMap mDeviceSubscriptions;
    
    SpaceObjectReference mCurrentGroup;
    typedef std::set<ProxyPositionObjectPtr> SelectedObjectSet;
    SelectedObjectSet mSelectedObjects;
    SpaceObjectReference mLastShiftSelected;

    // map from mouse button to drag for that mouse button.
    /* as far as multiple cursors are concerned,
       each cursor should have its own MouseHandler instance */
    std::map<int, DragAction> mDragAction;
    std::map<int, ActiveDrag*> mActiveDrag;
/*
    typedef EventResponse (MouseHandler::*ClickAction) (EventPtr evbase);
    std::map<int, ClickAction> mClickAction;
*/

    class SubObjectIterator {
        typedef Entity* value_type;
        //typedef ssize_t difference_type;
        typedef size_t size_type;
        OgreSystem::SceneEntitiesMap::const_iterator mIter;
        Entity *mParentEntity;
        OgreSystem *mOgreSys;
        void findNext() {
            while (!end() && !((*mIter).second->getProxy().getParent() == mParentEntity->id())){
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

    Entity *hoverEntity (CameraEntity *cam, Task::AbsTime time, float xPixel, float yPixel, int which=0) {
        Location location(cam->getProxy().globalLocation(time));
        Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
        SILOG(input,info,"X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<location.getPosition()<<"; dir = "<<dir);

        double dist;
        Entity *mouseOverEntity = mParent->rayTrace(location.getPosition(), dir, dist, which);
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
            Entity *ent = mParent->getEntity((*selectIter)->getObjectReference());
            if (ent) {
                ent->setSelected(false);
            }
            // Fire deselected event.
        }
        mSelectedObjects.clear();
    }
private:

    int mWhichRayObject;
    EventResponse selectObject(EventPtr ev, int direction) {
        std::tr1::shared_ptr<MouseClickEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
        if (!mouseev) {
            return EventResponse::nop();
        }
        CameraEntity *camera = mParent->mPrimaryCamera;
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
            // add object.
            Entity *mouseOver = hoverEntity(camera, Task::AbsTime::now(), mouseev->mX, mouseev->mY, mWhichRayObject);
            if (!mouseOver) {
                return EventResponse::nop();
            }
            if (mouseOver->id() == mLastShiftSelected) {
                SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
                if (selectIter != mSelectedObjects.end()) {
                    Entity *ent = mParent->getEntity((*selectIter)->getObjectReference());
                    if (ent) {
                        ent->setSelected(false);
                    }
                    mSelectedObjects.erase(selectIter);
                }
                mWhichRayObject+=direction;
                mLastShiftSelected = SpaceObjectReference::null();
            }
            mouseOver = hoverEntity(camera, Task::AbsTime::now(), mouseev->mX, mouseev->mY, mWhichRayObject);
            if (!mouseOver) {
                return EventResponse::nop();
            }

            SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
            if (selectIter == mSelectedObjects.end()) {
                SILOG(input,info,"Added selection " << mouseOver->id());
                mSelectedObjects.insert(mouseOver->getProxyPtr());
                mouseOver->setSelected(true);
                mLastShiftSelected = mouseOver->id();
                // Fire selected event.
            } else {
                SILOG(input,info,"Deselected " << (*selectIter)->getObjectReference());
                Entity *ent = mParent->getEntity((*selectIter)->getObjectReference());
                if (ent) {
                    ent->setSelected(false);
                }
                mSelectedObjects.erase(selectIter);
                // Fire deselected event.
            }
        } else if (mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            SILOG(input,info,"Cleared selection");
            clearSelection();
            mLastShiftSelected = SpaceObjectReference::null();
        } else {
            // reset selection.
            clearSelection();
            mWhichRayObject+=direction;
            Entity *mouseOver = hoverEntity(camera, Task::AbsTime::now(), mouseev->mX, mouseev->mY, mWhichRayObject);
            if (mouseOver) {
                mSelectedObjects.insert(mouseOver->getProxyPtr());
                mouseOver->setSelected(true);
                SILOG(input,info,"Replaced selection with " << mouseOver->id());
                // Fire selected event.
            }
            mLastShiftSelected = SpaceObjectReference::null();
        }
        return EventResponse::cancel();
    }

///////////////////// DRAG HANDLERS //////////////////////

    EventResponse wheelListener(EventPtr evbase) {
        std::tr1::shared_ptr<AxisEvent> ev(std::tr1::dynamic_pointer_cast<AxisEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (ev->mAxis == SDLMouse::WHEELY || ev->mAxis == SDLMouse::RELY) {
            zoomInOut(ev->mValue, ev->getDevice(), mParent->mPrimaryCamera, mSelectedObjects, mParent);
        } else if (ev->mAxis == SDLMouse::WHEELX || ev->mAxis == PointerDevice::RELX) {
            //orbitObject(Vector3d(ev->mValue.getCentered() * AXIS_TO_RADIANS, 0, 0), ev->getDevice());
        }
        return EventResponse::cancel();
    }

    ///////////////// KEYBOARD HANDLERS /////////////////

    EventResponse deleteObjects(EventPtr ev) {
        Task::AbsTime now(Task::AbsTime::now());
        while (doUngroupObjects(now)) {
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter)->getObjectReference());
            if (ent) {
                ent->getProxy().getProxyManager()->destroyObject(ent->getProxyPtr());
            }
        }
        mSelectedObjects.clear();
        return EventResponse::nop();
    }

    Entity *doCloneObject(Entity *ent, const ProxyPositionObjectPtr &parentPtr, Task::AbsTime now) {
        SpaceObjectReference newId = SpaceObjectReference(ent->id().space(), ObjectReference(UUID::random()));
        Location loc = ent->getProxy().globalLocation(now);
        Location localLoc = ent->getProxy().extrapolateLocation(now);
        ProxyManager *proxyMgr = ent->getProxy().getProxyManager();

        std::tr1::shared_ptr<ProxyMeshObject> meshObj(
            std::tr1::dynamic_pointer_cast<ProxyMeshObject>(ent->getProxyPtr()));
        std::tr1::shared_ptr<ProxyLightObject> lightObj(
            std::tr1::dynamic_pointer_cast<ProxyLightObject>(ent->getProxyPtr()));
        ProxyPositionObjectPtr newObj;
        if (meshObj) {
            std::tr1::shared_ptr<ProxyMeshObject> newMeshObject (new ProxyMeshObject(proxyMgr, newId));
            newObj = newMeshObject;
            proxyMgr->createObject(newMeshObject);
            newMeshObject->setMesh(meshObj->getMesh());
            newMeshObject->setScale(meshObj->getScale());
        } else if (lightObj) {
            std::tr1::shared_ptr<ProxyLightObject> newLightObject (new ProxyLightObject(proxyMgr, newId));
            newObj = newLightObject;
            proxyMgr->createObject(newLightObject);
            newLightObject->update(lightObj->getLastLightInfo());
        } else {
            newObj = ProxyPositionObjectPtr(new ProxyPositionObject(proxyMgr, newId));
            proxyMgr->createObject(newObj);
        }
        if (newObj) {
            if (parentPtr) {
                newObj->setParent(parentPtr, now, loc, localLoc);
                newObj->resetPositionVelocity(now, localLoc);
            } else {
                newObj->resetPositionVelocity(now, loc);
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
    EventResponse cloneObjects(EventPtr ev) {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::AbsTime now(Task::AbsTime::now());
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter)->getObjectReference());
            if (!ent) {
                continue;
            }
            Entity *newEnt = doCloneObject(ent, ent->getProxy().getParentProxy(), now);
            Location loc (ent->getProxy().extrapolateLocation(now));
            loc.setPosition(loc.getPosition() + Vector3d(WORLD_SCALE/2.,0,0));
            newEnt->getProxy().resetPositionVelocity(now, loc);
            newSelectedObjects.insert(newEnt->getProxyPtr());
            newEnt->setSelected(true);
            ent->setSelected(false);
        }
        mSelectedObjects.swap(newSelectedObjects);
        return EventResponse::nop();
    }
    EventResponse groupObjects(EventPtr ev) {
        if (mSelectedObjects.size()<2) {
            return EventResponse::nop();
        }
        SpaceObjectReference parentId = mCurrentGroup;
        Task::AbsTime now(Task::AbsTime::now());
        ProxyManager *proxyMgr = mParent->mPrimaryCamera->getProxy().getProxyManager();
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter)->getObjectReference());
            if (!ent) continue;
            if (ent->getProxy().getProxyManager() != proxyMgr) {
                SILOG(input,error,"Attempting to group objects owned by different proxy manager!");
                return EventResponse::nop();
            }
            if (!(ent->getProxy().getParent() == parentId)) {
                SILOG(input,error,"Multiple select "<< (*iter)->getObjectReference() << 
                      " has parent  "<<ent->getProxy().getParent() << " instead of " << mCurrentGroup);
                return EventResponse::nop();
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
        proxyMgr->createObject(ProxyObjectPtr(new ProxyPositionObject(proxyMgr, newParentId)));
        Entity *newParentEntity = mParent->getEntity(newParentId);
        newParentEntity->getProxy().resetPositionVelocity(now, totalLocation);

        if (parentEntity) {
            newParentEntity->getProxy().setParent(parentEntity->getProxyPtr(), now);
        }
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter)->getObjectReference());
            if (!ent) continue;
            ent->getProxy().setParent(newParentEntity->getProxyPtr(), now);
            ent->setSelected(false);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(newParentEntity->getProxyPtr());
        newParentEntity->setSelected(true);
        return EventResponse::nop();
    }

    bool doUngroupObjects(Task::AbsTime now) {
        int numUngrouped = 0;
        SelectedObjectSet newSelectedObjects;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *parentEnt = mParent->getEntity((*iter)->getObjectReference());
            if (!parentEnt) {
                continue;
            }
            ProxyManager *proxyMgr = parentEnt->getProxy().getProxyManager();
            ProxyPositionObjectPtr parentParent (parentEnt->getProxy().getParentProxy());
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
            } else {
                newSelectedObjects.insert(parentEnt->getProxyPtr());
            }
        }
        mSelectedObjects.swap(newSelectedObjects);
        return (numUngrouped>0);
    }

    EventResponse ungroupObjects(EventPtr ev) {
        Task::AbsTime now(Task::AbsTime::now());
        doUngroupObjects(now);
        return EventResponse::nop();
    }

    EventResponse enterObject(EventPtr ev) {
        Task::AbsTime now(Task::AbsTime::now());
        if (mSelectedObjects.size() != 1) {
            return EventResponse::nop();
        }
        Entity *parentEnt = NULL;
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            parentEnt = mParent->getEntity((*iter)->getObjectReference());
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
        return EventResponse::nop();
    }

    EventResponse leaveObject(EventPtr ev) {
        Task::AbsTime now(Task::AbsTime::now());
        for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *selent = mParent->getEntity((*iter)->getObjectReference());
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
        } else {
            mCurrentGroup = SpaceObjectReference::null();
        }
        return EventResponse::nop();
    }

    EventResponse createLight(EventPtr ev) {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::AbsTime now(Task::AbsTime::now());
        CameraEntity *camera = mParent->mPrimaryCamera;
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
            newLightObject->resetPositionVelocity(now, localLoc);
        } else {
            newLightObject->resetPositionVelocity(now, loc);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(newLightObject);
        Entity *ent = mParent->getEntity(newId);
        if (ent) {
            ent->setSelected(true);
        }
        return EventResponse::nop();
    }

    EventResponse moveHandler(EventPtr ev) {
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        Task::AbsTime now(Task::AbsTime::now());
        std::tr1::shared_ptr<ButtonEvent> buttonev (
            std::tr1::dynamic_pointer_cast<ButtonEvent>(ev));
        if (!buttonev) {
            return EventResponse::nop();
        }
        float amount = buttonev->mPressed?1:0;

        CameraEntity *cam = mParent->mPrimaryCamera;
        Location loc = cam->getProxy().extrapolateLocation(now);
        const Quaternion &orient = loc.getOrientation();

        switch (buttonev->mButton) {
          case 's':
            amount*=-1;
          case 'w':
            amount *= WORLD_SCALE;
            loc.setVelocity(direction(orient)*amount);
            loc.setAngularSpeed(0);
            break;
          case 'a':
            amount*=-1;
          case 'd':
            amount *= WORLD_SCALE;
            loc.setVelocity(orient.xAxis()*amount);
            loc.setAngularSpeed(0);
            break;
          case SDLK_DOWN:
            amount*=-1;
          case SDLK_UP:
            loc.setAxisOfRotation(Vector3f(1,0,0));
            loc.setAngularSpeed(buttonev->mPressed?amount:0);
            loc.setVelocity(Vector3f(0,0,0));
            break;
          case SDLK_RIGHT:
            amount*=-1;
          case SDLK_LEFT:
            amount*=2;
            loc.setAxisOfRotation(Vector3f(0,1,0));
            loc.setAngularSpeed(buttonev->mPressed?amount:0);
            loc.setVelocity(Vector3f(0,0,0));
            break;
          default:
            break;
        }
        cam->getProxy().setPositionVelocity(now, loc);
        return EventResponse::nop();
    }

    EventResponse import(EventPtr ev) {
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
				return EventResponse::nop();
			}
			std::cout << (unsigned char)c;
			filename += (unsigned char)c;
		}
		std::cout << '\n';
		std::vector<std::string> files;
		files.push_back(filename);
		mParent->mInputManager->filesDropped(files);
		return EventResponse::cancel();
	}


    ///////////////// DEVICE FUNCTIONS ////////////////

    SubscriptionId registerAxisListener(const InputDevicePtr &dev,
                              EventResponse(MouseHandler::*func)(EventPtr),
                              int axis) {
        Task::IdPair eventId (AxisEvent::getEventId(),
                              AxisEvent::getSecondaryId(dev, axis));
        SubscriptionId subId = mParent->mInputManager->subscribeId(eventId,
            std::tr1::bind(func, this, _1));
        mEvents.push_back(subId);
        mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*dev, subId));
        return subId;
    }

    SubscriptionId registerButtonListener(const InputDevicePtr &dev,
                              EventResponse(MouseHandler::*func)(EventPtr),
                              int button, bool released=false, InputDevice::Modifier mod=0) {
        Task::IdPair eventId (released?ButtonReleased::getEventId():ButtonPressed::getEventId(),
                              ButtonEvent::getSecondaryId(button, mod, dev));
        SubscriptionId subId = mParent->mInputManager->subscribeId(eventId,
            std::tr1::bind(func, this, _1));
        mEvents.push_back(subId);
        mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*dev, subId));
        return subId;
    }

    EventResponse setDragMode(EventPtr evbase) {
        std::tr1::shared_ptr<ButtonEvent> ev (
            std::tr1::dynamic_pointer_cast<ButtonEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        switch (ev->mButton) {
          case 'q':
            mDragAction[1] = 0;
            break;
          case 'w':
            mDragAction[1] = DragActionRegistry::get("moveObject");
            break;
          case 'e':
            mDragAction[1] = DragActionRegistry::get("rotateObject");
            break;
          case 'r':
            mDragAction[1] = DragActionRegistry::get("scaleObject");
            break;
          case 't':
            mDragAction[1] = DragActionRegistry::get("rotateCamera");
            break;
        }
        return EventResponse::nop();
    }

    EventResponse deviceListener(EventPtr evbase) {
        std::tr1::shared_ptr<InputDeviceEvent> ev (std::tr1::dynamic_pointer_cast<InputDeviceEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        switch (ev->mType) {
          case InputDeviceEvent::ADDED:
            if (!!(std::tr1::dynamic_pointer_cast<SDLMouse>(ev->mDevice))) {
                registerAxisListener(ev->mDevice, &MouseHandler::wheelListener, SDLMouse::WHEELX);
                registerAxisListener(ev->mDevice, &MouseHandler::wheelListener, SDLMouse::WHEELY);
            }
            if (!!(std::tr1::dynamic_pointer_cast<SDLKeyboard>(ev->mDevice))) {
                registerButtonListener(ev->mDevice, &MouseHandler::groupObjects, 'g');
                registerButtonListener(ev->mDevice, &MouseHandler::ungroupObjects, 'g',false,InputDevice::MOD_ALT);
                registerButtonListener(ev->mDevice, &MouseHandler::deleteObjects, SDLK_DELETE);
                registerButtonListener(ev->mDevice, &MouseHandler::deleteObjects, SDLK_KP_PERIOD); // Del
                registerButtonListener(ev->mDevice, &MouseHandler::cloneObjects, 'v',false,InputDevice::MOD_CTRL);
                registerButtonListener(ev->mDevice, &MouseHandler::cloneObjects, 'd');
                registerButtonListener(ev->mDevice, &MouseHandler::enterObject, SDLK_KP_ENTER);
                registerButtonListener(ev->mDevice, &MouseHandler::enterObject, SDLK_RETURN);
                registerButtonListener(ev->mDevice, &MouseHandler::leaveObject, SDLK_KP_0);
                registerButtonListener(ev->mDevice, &MouseHandler::leaveObject, SDLK_ESCAPE);
                registerButtonListener(ev->mDevice, &MouseHandler::createLight, 'b');

                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'w',false, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'a',false, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 's',false, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'd',false, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_UP);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_DOWN);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_LEFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_RIGHT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'w',true, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'a',true, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 's',true, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'd',true, InputDevice::MOD_SHIFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_UP,true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_DOWN,true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_LEFT,true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_RIGHT,true);
                registerButtonListener(ev->mDevice, &MouseHandler::import, 'o', false, InputDevice::MOD_CTRL);

                registerButtonListener(ev->mDevice, &MouseHandler::setDragMode, 'q');
                registerButtonListener(ev->mDevice, &MouseHandler::setDragMode, 'w');
                registerButtonListener(ev->mDevice, &MouseHandler::setDragMode, 'e');
                registerButtonListener(ev->mDevice, &MouseHandler::setDragMode, 'r');
                registerButtonListener(ev->mDevice, &MouseHandler::setDragMode, 't');

            }
            break;
          case InputDeviceEvent::REMOVED:
            {
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

    EventResponse doDrag(EventPtr evbase) {
        MouseDragEventPtr ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        ActiveDrag * &drag = mActiveDrag[ev->mButton];
        if (ev->mType == MouseDragEvent::START) {
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

            if (ev->mType == MouseDragEvent::END) {
                delete drag;
                drag = 0;
            }
        }
        return EventResponse::nop();
    }

// .....................

// SCROLL WHEEL: up/down = move object closer/farther. left/right = rotate object about Y axis.
// holding down middle button up/down = move object closer/farther, left/right = rotate object about Y axis.

// Accuracy: relative versus absolute mode; exponential decay versus pixels.

public:
    MouseHandler(OgreSystem *parent) : mParent(parent), mCurrentGroup(SpaceObjectReference::null()), mWhichRayObject(0) {
        mEvents.push_back(mParent->mInputManager->registerDeviceListener(
            std::tr1::bind(&MouseHandler::deviceListener, this, _1)));

        mEvents.push_back(mParent->mInputManager->subscribeId(
                              MouseDragEvent::getEventId(),
                              std::tr1::bind(&MouseHandler::doDrag, this, _1)));
        mDragAction[1] = 0;
        mDragAction[2] = DragActionRegistry::get("zoomCamera");
        mDragAction[3] = DragActionRegistry::get("panCamera");
        mDragAction[4] = DragActionRegistry::get("rotateCamera");

        mEvents.push_back(mParent->mInputManager->subscribeId(
                              IdPair(MouseClickEvent::getEventId(), 
                                     MouseDownEvent::getSecondaryId(1)),
                              std::tr1::bind(&MouseHandler::selectObject, this, _1, 1)));
        mEvents.push_back(mParent->mInputManager->subscribeId(
                              IdPair(MouseClickEvent::getEventId(), 
                                     MouseDownEvent::getSecondaryId(3)),
                              std::tr1::bind(&MouseHandler::selectObject, this, _1, -1)));
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
    void addToSelection(const ProxyPositionObjectPtr &obj) {
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
