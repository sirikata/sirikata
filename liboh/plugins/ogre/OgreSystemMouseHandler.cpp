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
#include <task/Event.hpp>
#include <task/Time.hpp>
#include <task/EventManager.hpp>
#include <SDL_keysym.h>
#include <boost/math/special_functions/fpclassify.hpp>


#include <map>


namespace Sirikata {
namespace Graphics {
using namespace Input;
using namespace Task;

class OgreSystem::MouseHandler {
    OgreSystem *mParent;
    std::vector<SubscriptionId> mEvents;
    typedef std::multimap<InputDevice*, SubscriptionId> DeviceSubMap;
    DeviceSubMap mDeviceSubscriptions;
    
    SpaceObjectReference mCurrentGroup;
    typedef std::map<SpaceObjectReference, Location> SelectedObjectMap;
    SelectedObjectMap mSelectedObjects;
    SpaceObjectReference mLastShiftSelected;
    Vector3d mMoveVector;

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

    // Assumes orthographic projection
    static void pixelToRadians(CameraEntity *cam, float deltaXPct, float deltaYPct, float &xRadians, float &yRadians) {
        // This function is useless and hopelessly broken, since radians have no meaning in perspective. Use pixelToDirection instead!!!
        SILOG(input,info,"FOV Y Radians: "<<cam->getOgreCamera()->getFOVy().valueRadians()<<"; aspect = "<<cam->getOgreCamera()->getAspectRatio());
        xRadians = cam->getOgreCamera()->getFOVy().valueRadians() * cam->getOgreCamera()->getAspectRatio() * deltaXPct;
        yRadians = cam->getOgreCamera()->getFOVy().valueRadians() * deltaYPct;
        SILOG(input,info,"X = "<<deltaXPct<<"; Y = "<<deltaYPct);
        SILOG(input,info,"Xradian = "<<xRadians<<"; Yradian = "<<yRadians);
    }
    // Uses perspective
    static Vector3f pixelToDirection(CameraEntity *cam, Quaternion orient, float xPixel, float yPixel) {
        float xRadian, yRadian;
        //pixelToRadians(cam, xPixel/2, yPixel/2, xRadian, yRadian);
        xRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * cam->getOgreCamera()->getAspectRatio() * xPixel;
        yRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * yPixel;

        return Vector3f(-orient.zAxis()*cos(cam->getOgreCamera()->getFOVy().valueRadians()*.5) + 
                        orient.xAxis() * xRadian +
                        orient.yAxis() * yRadian);
    }

    static inline Vector3f direction(Quaternion cameraAngle) {
        return -cameraAngle.zAxis();
    }

    Vector3d averageSelectedPosition(Task::AbsTime now) const {
        Vector3d totalPosition(0,0,0);
        if (mSelectedObjects.empty()) {
            return totalPosition;
        }
        for (SelectedObjectMap::const_iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter).first);
            if (!ent) continue;
            totalPosition += (ent->getProxy().globalLocation(now).getPosition());
        }
        return totalPosition / mSelectedObjects.size();
    }

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
        for (SelectedObjectMap::const_iterator selectIter = mSelectedObjects.begin();
             selectIter != mSelectedObjects.end(); ++selectIter) {
            mParent->getEntity((*selectIter).first)->setSelected(false);
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
                SelectedObjectMap::iterator selectIter = mSelectedObjects.find(mouseOver->id());
                if (selectIter != mSelectedObjects.end()) {
                    if (mParent->getEntity((*selectIter).first)) {
                        mParent->getEntity((*selectIter).first)->setSelected(false);
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

            SelectedObjectMap::iterator selectIter = mSelectedObjects.find(mouseOver->id());
            if (selectIter == mSelectedObjects.end()) {
                SILOG(input,info,"Added selection " << mouseOver->id());
                mSelectedObjects.insert(SelectedObjectMap::value_type(mouseOver->id(), mouseOver->getProxy().extrapolateLocation(Task::AbsTime::now())));
                mouseOver->setSelected(true);
                mLastShiftSelected = mouseOver->id();
                // Fire selected event.
            } else {
                SILOG(input,info,"Deselected " << (*selectIter).first);
                if (mParent->getEntity((*selectIter).first)) {
                    mParent->getEntity((*selectIter).first)->setSelected(false);
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
                mSelectedObjects.insert(SelectedObjectMap::value_type(mouseOver->id(), mouseOver->getProxy().extrapolateLocation(Task::AbsTime::now())));
                mouseOver->setSelected(true);
                SILOG(input,info,"Replaced selection with " << mouseOver->id());
                // Fire selected event.
            }
            mLastShiftSelected = SpaceObjectReference::null();
        }
        return EventResponse::cancel();
    }

///////////////////// DRAG HANDLERS //////////////////////

    void rotateCamera(CameraEntity *camera, float radianX, float radianY) {
        Task::AbsTime now = Task::AbsTime::now();
        
        Quaternion orient(camera->getProxy().globalLocation(now).getOrientation());
        Quaternion dragStart (camera->getProxy().extrapolateLocation(now).getOrientation());
        Quaternion dhorient = Quaternion(Vector3f(0,1,0),radianX);
        Quaternion dvorient = Quaternion(dhorient * orient * Vector3f(1,0,0),-radianY);

        Location location = camera->getProxy().extrapolateLocation(now);
        location.setOrientation((dvorient * dhorient * dragStart).normal());
        camera->getProxy().resetPositionVelocity(now, location);
    }

    void panCamera(CameraEntity *camera, const Vector3d &oldLocalPosition, const Vector3d &toPan) {
        Task::AbsTime now = Task::AbsTime::now();
        
        Quaternion orient(camera->getProxy().globalLocation(now).getOrientation());

        Location location (camera->getProxy().extrapolateLocation(now));
        location.setPosition(orient * toPan + oldLocalPosition);
        camera->getProxy().resetPositionVelocity(now, location);
    }

    bool mIsRotating;
    EventResponse rotateScene(EventPtr ev) {
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
            return EventResponse::nop();
        }
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            return EventResponse::nop();
        }
        std::tr1::shared_ptr<MouseDragEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(ev));
        if (!mouseev) {
            return EventResponse::nop();
        }
        CameraEntity *camera = mParent->mPrimaryCamera;

        if (mouseev->mType == MouseDragEvent::START) {
            mIsRotating = true;
        } else if (!mIsRotating) {
            return EventResponse::nop();
        }
        float radianX, radianY;
        pixelToRadians(camera, mouseev->deltaLastX(), mouseev->deltaLastY(), radianX, radianY);
        rotateCamera(camera, radianX, radianY);
        
        return EventResponse::cancel();
    }

    EventResponse moveSelection(EventPtr ev) {
        std::tr1::shared_ptr<MouseDragEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(ev));
        if (!mouseev) {
            return EventResponse::nop();
        }
        
        if (mSelectedObjects.empty()) {
            SILOG(input,insane,"moveSelection: Found no selected objects");
            return EventResponse::nop();
        }
        CameraEntity *camera = mParent->mPrimaryCamera;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();
        if (mouseev->mType == MouseDragEvent::START) {

            float moveDistance;
            bool foundObject = false;
            for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
                 iter != mSelectedObjects.end(); ++iter) {
                Entity *ent = mParent->getEntity((*iter).first);
                if (!ent) continue;
                (*iter).second = ent->getProxy().extrapolateLocation(now);
                Vector3d deltaPosition (ent->getProxy().globalLocation(now).getPosition() - cameraLoc.getPosition());
                double dist = deltaPosition.dot(Vector3d(cameraAxis));
                if (!foundObject || dist < moveDistance) {
                    foundObject = true;
                    moveDistance = dist;
                    mMoveVector = deltaPosition;
                }
            }
            SILOG(input,insane,"moveSelection: Moving selected objects at distance " << mMoveVector);
        }

        Vector3d startAxis (pixelToDirection(camera, cameraLoc.getOrientation(), mouseev->mXStart, mouseev->mYStart));
        Vector3d endAxis (pixelToDirection(camera, cameraLoc.getOrientation(), mouseev->mX, mouseev->mY));
        Vector3d start, end;
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
            start = startAxis.normal() * (mMoveVector.y/startAxis.y);
            end = endAxis.normal() * (mMoveVector.y/endAxis.y);
        } else if (mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            start = startAxis.normal() * (mMoveVector.z/startAxis.z);
            end = endAxis.normal() * (mMoveVector.z/endAxis.z);
        } else {
            float moveDistance = mMoveVector.dot(Vector3d(cameraAxis));
            start = startAxis * moveDistance; // / cameraAxis.dot(startAxis);
            end = endAxis * moveDistance; // / cameraAxis.dot(endAxis);
        }
        Vector3d toMove (end - start);
		if (!boost::math::isfinite(toMove.x) || !boost::math::isfinite(toMove.y) || !boost::math::isfinite(toMove.z)) {
			return EventResponse::cancel();
		}
		// Prevent moving outside of a small radius so you don't shoot an object into the horizon.
		if (toMove.length() > 10*mParent->mInputManager->mWorldScale->as<float>()) {
			// moving too much.
			toMove *= (10*mParent->mInputManager->mWorldScale->as<float>()/toMove.length());
		}
        SILOG(input,debug,"Start "<<start<<"; End "<<end<<"; toMove "<<toMove);
        for (SelectedObjectMap::const_iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter).first);
            if (!ent) continue;
            Location toSet (ent->getProxy().extrapolateLocation(now));
            SILOG(input,debug,"moveSelection: OLD " << toSet.getPosition());
            toSet.setPosition((*iter).second.getPosition() + toMove);
            SILOG(input,debug,"moveSelection: NEW " << toSet.getPosition());
            ent->getProxy().setPositionVelocity(now, toSet);
        }
        return EventResponse::cancel();
    }

    Vector3d mStartPan;
    double mPanDistance;
    bool mRelativePan;
    EventResponse panScene(EventPtr ev) {
        if (!mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            return EventResponse::nop();
        }
        std::tr1::shared_ptr<MouseDragEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(ev));
        if (!mouseev) {
            return EventResponse::nop();
        }
        double distance;
        CameraEntity *camera = mParent->mPrimaryCamera;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);

        if (mouseev->mType == MouseDragEvent::START) {
            mRelativePan = false;
            mStartPan = camera->getProxy().extrapolateLocation(now).getPosition();
            Vector3f toMove (
                pixelToDirection(camera, cameraLoc.getOrientation(), mouseev->mXStart, mouseev->mYStart));
            if (mParent->rayTrace(cameraLoc.getPosition(), toMove, distance)) {
                mPanDistance = distance;
            } else if (!mSelectedObjects.empty()) {
                Vector3d totalPosition(averageSelectedPosition(now));
                mPanDistance = (totalPosition - cameraLoc.getPosition()).length();
            } else {
                float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
                mPanDistance = WORLD_SCALE;
                mouseev->getDevice()->pushRelativeMode();
                mRelativePan = true;
            }
        } else if (mouseev->mType == MouseDragEvent::END && mRelativePan) {
            mouseev->getDevice()->popRelativeMode();
        }
        if (mPanDistance) {
            float radianX, radianY;
            pixelToRadians(camera, mouseev->deltaX(), mouseev->deltaY(), radianX, radianY);
            panCamera(camera, mStartPan, Vector3d(-radianX*mPanDistance, -radianY*mPanDistance, 0));
        }
        return EventResponse::nop();
    }

    void orbitObject(Vector3d amount, const InputDevicePtr &dev) {
        double distance;
        CameraEntity *camera = mParent->mPrimaryCamera;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
/*
        Vector3f toMove (
            pixelToDirection(camera, cameraLoc.getOrientation(),
                             dev->getAxis(PointerDevice::CURSORX).getCentered(),
                             dev->getAxis(PointerDevice::CURSORY).getCentered()));
        if (mParent->rayTrace(cameraLoc.getPosition(), toMove, distance)) {
            rotateCamera(camera, amount.x, amount.y);
            panCamera(camera, amount * distance);
        } else */
        if (!mSelectedObjects.empty()) {
            Vector3d totalPosition (averageSelectedPosition(now));
            double multiplier = (totalPosition - cameraLoc.getPosition()).length();
            rotateCamera(camera, amount.x, amount.y);
            panCamera(camera, camera->getProxy().extrapolatePosition(now), amount * multiplier);
        } else {
            //rotateCamera(camera, -amount.x, -amount.y);
        }
    }

    void orbitObject_BROKEN(AxisValue value) {
        SILOG(input,debug,"rotate "<<value);
        
        if (mSelectedObjects.empty()) {
            SILOG(input,debug,"rotateXZ: Found no selected objects");
            return;
        }
        CameraEntity *camera = mParent->mPrimaryCamera;
        Task::AbsTime now = Task::AbsTime::now();

        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3d totalPosition (averageSelectedPosition(now));
        Vector3d distance (cameraLoc.getPosition() - totalPosition);

        float radianX = value.getCentered() * 1.0;
        Quaternion dhorient = Quaternion(Vector3f(0,1,0), radianX);
        distance = dhorient * distance;
        Quaternion dhorient2 = Quaternion(Vector3f(0,1,0), -radianX);
        cameraLoc.setPosition(totalPosition + dhorient * distance);
        cameraLoc.setOrientation(dhorient2 * cameraLoc.getOrientation());
        camera->getProxy().resetPositionVelocity(now, cameraLoc);
    }
    void zoomInOut(AxisValue value, const InputDevicePtr &dev) {
        if (!dev) {
            return;
        }
        SILOG(input,debug,"zoom "<<value);

        CameraEntity *camera = mParent->mPrimaryCamera;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3d toMove;

        toMove = Vector3d(
            pixelToDirection(camera, cameraLoc.getOrientation(),
                             dev->getAxis(PointerDevice::CURSORX).getCentered(),
                             dev->getAxis(PointerDevice::CURSORY).getCentered()));
        double distance;
        float WORLD_SCALE = mParent->mInputManager->mWorldScale->as<float>();
        if (!mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL) &&
            !mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
            toMove *= WORLD_SCALE;
        } else if (mParent->rayTrace(cameraLoc.getPosition(), direction(cameraLoc.getOrientation()), distance) && 
                (distance*.75 < WORLD_SCALE || mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT))) {
            toMove *= distance*.75;
        } else if (!mSelectedObjects.empty()) {
            Vector3d totalPosition (averageSelectedPosition(now));
            toMove *= (totalPosition - cameraLoc.getPosition()).length() * .75;
        } else {
            toMove *= WORLD_SCALE;
        }
        toMove *= value.getCentered(); // up == zoom in
        cameraLoc.setPosition(cameraLoc.getPosition() + toMove);
        camera->getProxy().resetPositionVelocity(now, cameraLoc);
    }

    EventResponse wheelListener(EventPtr evbase) {
        std::tr1::shared_ptr<AxisEvent> ev(std::tr1::dynamic_pointer_cast<AxisEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (ev->mAxis == SDLMouse::WHEELY || ev->mAxis == SDLMouse::RELY) {
            zoomInOut(ev->mValue, ev->getDevice());
        } else if (ev->mAxis == SDLMouse::WHEELX || ev->mAxis == PointerDevice::RELX) {
            //orbitObject(Vector3d(ev->mValue.getCentered() * AXIS_TO_RADIANS, 0, 0), ev->getDevice());
        }
        return EventResponse::cancel();
    }

    EventResponse middleDragHandler(EventPtr evbase) {
        std::tr1::shared_ptr<MouseDragEvent> ev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (ev->mType == MouseDragEvent::START) {
            ev->getDevice()->pushRelativeMode();
        } else if (ev->mType == MouseDragEvent::END) {
            ev->getDevice()->popRelativeMode();
        }
        if (ev->deltaLastY() != 0) {
            float dragMultiplier = mParent->mInputManager->mDragMultiplier->as<float>();
            zoomInOut(AxisValue::fromCentered(dragMultiplier*ev->deltaLastY()), ev->getDevice());
        }
        if (ev->deltaLastX() != 0) {
            //rotateXZ(AxisValue::fromCentered(ev->deltaLastX()));
            //rotateCamera(mParent->mPrimaryCamera, ev->deltaLastX() * AXIS_TO_RADIANS, 0);
        }
        return EventResponse::cancel();
    }

    EventResponse scaleSelection(EventPtr evbase) {
        std::tr1::shared_ptr<MouseDragEvent> ev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)||
            mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            return EventResponse::nop();
        }
        if (ev->mType == MouseDragEvent::START) {
            ev->getDevice()->pushRelativeMode();
        } else if (ev->mType == MouseDragEvent::END) {
            ev->getDevice()->popRelativeMode();
        }
        if (ev->deltaLastY() != 0) {
            float dragMultiplier = mParent->mInputManager->mDragMultiplier->as<float>();
            float scaleamt = exp(dragMultiplier*ev->deltaLastY());
            for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
                 iter != mSelectedObjects.end(); ++iter) {
                Entity *ent = mParent->getEntity((*iter).first);
                if (!ent) {
                    continue;
                }
                std::tr1::shared_ptr<ProxyMeshObject> meshptr (
                    std::tr1::dynamic_pointer_cast<ProxyMeshObject>(ent->getProxyPtr()));
                if (meshptr) {
                    meshptr->setScale(meshptr->getScale() * scaleamt);
                }
            }
        }
        if (ev->deltaLastX() != 0) {
            //rotateXZ(AxisValue::fromCentered(ev->deltaLastX()));
            //rotateCamera(mParent->mPrimaryCamera, ev->deltaLastX() * AXIS_TO_RADIANS, 0);
        }
        return EventResponse::cancel();
    }

    EventResponse rotateSelection(EventPtr evbase) {
        std::tr1::shared_ptr<MouseDragEvent> ev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (!mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)&&
            !mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            return EventResponse::nop();
        }
        if (ev->mType == MouseDragEvent::START) {
            ev->getDevice()->pushRelativeMode();
        } else if (ev->mType == MouseDragEvent::END) {
            ev->getDevice()->popRelativeMode();
        }
        if (ev->deltaLastX() != 0 || ev->deltaLastY() != 0) {
            Task::AbsTime now(Task::AbsTime::now());
            float dragMultiplier = mParent->mInputManager->mDragMultiplier->as<float>();
            float radianX, radianY;
            pixelToRadians(mParent->mPrimaryCamera,ev->deltaLastX(),ev->deltaLastY(),radianX,radianY);
            if (!mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
                radianY = 0;
            }
            if (!mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
                radianX = 0;
            }
            for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
                 iter != mSelectedObjects.end(); ++iter) {
                Entity *ent = mParent->getEntity((*iter).first);
                if (!ent) {
                    continue;
                }
                Location loc (ent->getProxy().extrapolateLocation(now));
                loc.setOrientation(Quaternion(Vector3f(0,1,0),radianX)*
                                   Quaternion(Vector3f(1,0,0),radianY)*
                                   loc.getOrientation());
                ent->getProxy().resetPositionVelocity(now, loc);
            }
        }
        return EventResponse::cancel();
    }

    bool mIsOrbiting;
    EventResponse orbitDragHandler(EventPtr evbase) {
        std::tr1::shared_ptr<MouseDragEvent> ev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
        if (!ev) {
            return EventResponse::nop();
        }
        if (ev->mType == MouseDragEvent::END && mIsOrbiting) {
            ev->getDevice()->popRelativeMode();
        }
        if (!mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
            return EventResponse::nop();
        }
        if (ev->mType == MouseDragEvent::START) {
            ev->getDevice()->pushRelativeMode();
            mIsOrbiting=true;
        } else if (!mIsOrbiting) {
            return EventResponse::nop();
        }
        float dragMultiplier = mParent->mInputManager->mDragMultiplier->as<float>();
        float toRadians = mParent->mInputManager->mAxisToRadians->as<float>();
        orbitObject(Vector3d(ev->deltaLastX(), ev->deltaLastY(), 0) * -toRadians*dragMultiplier,
                    ev->getDevice());
        return EventResponse::cancel();
    }


    ///////////////// KEYBOARD HANDLERS /////////////////

    EventResponse deleteObjects(EventPtr ev) {
        Task::AbsTime now(Task::AbsTime::now());
        while (doUngroupObjects(now)) {
        }
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter).first);
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
        SelectedObjectMap newSelectedObjectMap;
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter).first);
            if (!ent) {
                continue;
            }
            Entity *newEnt = doCloneObject(ent, ent->getProxy().getParentProxy(), now);
            Location loc (ent->getProxy().extrapolateLocation(now));
            loc.setPosition(loc.getPosition() + Vector3d(WORLD_SCALE/2.,0,0));
            newEnt->getProxy().resetPositionVelocity(now, loc);
            newSelectedObjectMap.insert(SelectedObjectMap::value_type(newEnt->id(),newEnt->getProxy().extrapolateLocation(now)));
            newEnt->setSelected(true);
            ent->setSelected(false);
        }
        mSelectedObjects.swap(newSelectedObjectMap);
        return EventResponse::nop();
    }
    EventResponse groupObjects(EventPtr ev) {
        if (mSelectedObjects.size()<2) {
            return EventResponse::nop();
        }
        SpaceObjectReference parentId = mCurrentGroup;
        Task::AbsTime now(Task::AbsTime::now());
        ProxyManager *proxyMgr = NULL;
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter).first);
            if (!ent) continue;
            if (proxyMgr==NULL) {
                proxyMgr = ent->getProxy().getProxyManager();
            }
            if (ent->getProxy().getProxyManager() != proxyMgr) {
                SILOG(input,error,"Attempting to group objects owned by different proxy manager!");
                return EventResponse::nop();
            }
            if (!(ent->getProxy().getParent() == parentId)) {
                SILOG(input,error,"Multiple select "<< (*iter).first << 
                      " has parent  "<<ent->getProxy().getParent() << " instead of " << mCurrentGroup);
                return EventResponse::nop();
            }
        }
        Vector3d totalPosition (averageSelectedPosition(now));
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
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *ent = mParent->getEntity((*iter).first);
            if (!ent) continue;
            ent->getProxy().setParent(newParentEntity->getProxyPtr(), now);
            ent->setSelected(false);
        }
        mSelectedObjects.clear();
        mSelectedObjects.insert(SelectedObjectMap::value_type(newParentId, newParentEntity->getProxy().extrapolateLocation(now)));
        newParentEntity->setSelected(true);
        return EventResponse::nop();
    }

    bool doUngroupObjects(Task::AbsTime now) {
        int numUngrouped = 0;
        SelectedObjectMap newSelectedObjectMap;
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *parentEnt = mParent->getEntity((*iter).first);
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
                newSelectedObjectMap.insert(SelectedObjectMap::value_type(ent->id(), ent->getProxy().extrapolateLocation(now)));
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                parentEnt->setSelected(false);
                proxyMgr->destroyObject(parentEnt->getProxyPtr());
                parentEnt = NULL; // dies.
                numUngrouped++;
            } else {
                newSelectedObjectMap.insert(SelectedObjectMap::value_type(parentEnt->id(), parentEnt->getProxy().extrapolateLocation(now)));
            }
        }
        mSelectedObjects.swap(newSelectedObjectMap);
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
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            parentEnt = mParent->getEntity((*iter).first);
        }
        if (parentEnt) {
            SelectedObjectMap newSelectedObjectMap;
            bool hasSubObjects = false;
            for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
                hasSubObjects = true;
                Entity *ent = *subIter;
                newSelectedObjectMap.insert(SelectedObjectMap::value_type(ent->id(), ent->getProxy().extrapolateLocation(now)));
                ent->setSelected(true);
            }
            if (hasSubObjects) {
                mSelectedObjects.swap(newSelectedObjectMap);
                mCurrentGroup = parentEnt->id();
            }
        }
        return EventResponse::nop();
    }

    EventResponse leaveObject(EventPtr ev) {
        Task::AbsTime now(Task::AbsTime::now());
        for (SelectedObjectMap::iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Entity *selent = mParent->getEntity((*iter).first);
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
                mSelectedObjects.insert(SelectedObjectMap::value_type(mCurrentGroup,parentEnt->getProxy().extrapolateLocation(now)));
            }
        } else {
            mCurrentGroup = SpaceObjectReference::null();
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
                registerButtonListener(ev->mDevice, &MouseHandler::ungroupObjects, 'u');
                registerButtonListener(ev->mDevice, &MouseHandler::deleteObjects, SDLK_DELETE);
                registerButtonListener(ev->mDevice, &MouseHandler::deleteObjects, SDLK_KP_PERIOD); // Del
                registerButtonListener(ev->mDevice, &MouseHandler::cloneObjects, 'c');
                registerButtonListener(ev->mDevice, &MouseHandler::enterObject, SDLK_KP_ENTER);
                registerButtonListener(ev->mDevice, &MouseHandler::enterObject, SDLK_RETURN);
                registerButtonListener(ev->mDevice, &MouseHandler::leaveObject, SDLK_KP_0);
                registerButtonListener(ev->mDevice, &MouseHandler::leaveObject, SDLK_ESCAPE);

                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'w');
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'a');
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 's');
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'd');
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_UP);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_DOWN);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_LEFT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_RIGHT);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'w',true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'a',true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 's',true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, 'd',true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_UP,true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_DOWN,true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_LEFT,true);
                registerButtonListener(ev->mDevice, &MouseHandler::moveHandler, SDLK_RIGHT,true);
                registerButtonListener(ev->mDevice, &MouseHandler::import, 'o', false, InputDevice::MOD_CTRL);
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

// .....................

// SCROLL WHEEL: up/down = move object closer/farther. left/right = rotate object about Y axis.
// holding down middle button up/down = move object closer/farther, left/right = rotate object about Y axis.

// Accuracy: relative versus absolute mode; exponential decay versus pixels.

public:
    void registerDragHandler(EventResponse (MouseHandler::*func) (EventPtr),
                             int button) {
        IdPair::Secondary secId = MouseDownEvent::getSecondaryId(button);
        
        mEvents.push_back(mParent->mInputManager->subscribeId(
                              IdPair(MouseDragEvent::getEventId(), secId),
                              std::tr1::bind(func, this, _1)));
    }
    MouseHandler(OgreSystem *parent) : mParent(parent), mCurrentGroup(SpaceObjectReference::null()), mWhichRayObject(0), mIsRotating(false),mPanDistance(0),mIsOrbiting(false) {
        mEvents.push_back(mParent->mInputManager->registerDeviceListener(
            std::tr1::bind(&MouseHandler::deviceListener, this, _1)));

        registerDragHandler(&MouseHandler::orbitDragHandler, 1);//shift
        registerDragHandler(&MouseHandler::panScene, 1);//ctrl
        registerDragHandler(&MouseHandler::rotateScene, 1);//none

        registerDragHandler(&MouseHandler::rotateSelection, 4);
        registerDragHandler(&MouseHandler::scaleSelection, 4);
        registerDragHandler(&MouseHandler::moveSelection, 3);
        registerDragHandler(&MouseHandler::middleDragHandler, 2);
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
		mSelectedObjects.insert(SelectedObjectMap::value_type(obj->getObjectReference(), obj->extrapolateLocation(Task::AbsTime::now())));
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
