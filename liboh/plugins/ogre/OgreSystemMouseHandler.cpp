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
#include "input/SDLInputManager.hpp"
#include <oh/ProxyPositionObject.hpp>
#include "input/InputEvents.hpp"
#include "input/SDLInputDevice.hpp"
#include <task/Event.hpp>
#include <task/Time.hpp>
#include <task/EventManager.hpp>

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
    
    std::map<Entity*, Location> mSelectedObjects;
    Vector3d mMoveVector;

    static void pixelToRadians(CameraEntity *cam, float deltaXPct, float deltaYPct, float &xRadians, float &yRadians) {
        SILOG(input,info,"FOV Y Radians: "<<cam->getOgreCamera()->getFOVy().valueRadians()<<"; aspect = "<<cam->getOgreCamera()->getAspectRatio());
        xRadians = cam->getOgreCamera()->getFOVy().valueRadians() * cam->getOgreCamera()->getAspectRatio() * deltaXPct;
        yRadians = cam->getOgreCamera()->getFOVy().valueRadians() * deltaYPct;
        SILOG(input,info,"X = "<<deltaXPct<<"; Y = "<<deltaYPct);
        SILOG(input,info,"Xradian = "<<xRadians<<"; Yradian = "<<yRadians);
    }
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

    Entity *hoverEntity (CameraEntity *cam, Task::AbsTime time, float xPixel, float yPixel) {
        Location location(cam->getProxy().globalLocation(time));
        Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
        SILOG(input,info,"X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<location.getPosition()<<"; dir = "<<dir);

        double dist;
        Entity *mouseOverEntity = mParent->rayTrace(location.getPosition(), dir, dist);
        if (mouseOverEntity) {
            // return true
            return mouseOverEntity;
        }
        return NULL;
    }

    void clearSelection() {
        for (std::map<Entity*,Location>::const_iterator selectIter = mSelectedObjects.begin();
             selectIter != mSelectedObjects.end(); ++selectIter) {
            (*selectIter).first->setSelected(false);
            // Fire deselected event.
        }
        mSelectedObjects.clear();
    }

    EventResponse selectObject(EventPtr ev) {
        std::tr1::shared_ptr<MouseClickEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
        if (!mouseev) {
            return EventResponse::nop();
        }
        CameraEntity *camera = mParent->mPrimaryCamera;
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT)) {
            // add object.
            Entity *mouseOver = hoverEntity(camera, Task::AbsTime::now(), mouseev->mX, mouseev->mY);
            if (mouseOver) {
                std::map<Entity*,Location>::iterator selectIter = mSelectedObjects.find(mouseOver);
                if (selectIter == mSelectedObjects.end()) {
                    SILOG(input,info,"Added selection " << mouseOver->id());
                    mSelectedObjects.insert(std::map<Entity*,Location>::value_type(mouseOver, Location()));
                    mouseOver->setSelected(true);
                    // Fire selected event.
                } else {
                    SILOG(input,info,"Deselected " << (*selectIter).first->id());
                    (*selectIter).first->setSelected(false);
                    mSelectedObjects.erase(selectIter);
                    // Fire deselected event.
                }
            }
        } else if (mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            SILOG(input,info,"Cleared selection");
            clearSelection();
        } else {
            // reset selection.
            clearSelection();
            Entity *mouseOver = hoverEntity(camera, Task::AbsTime::now(), mouseev->mX, mouseev->mY);
            if (mouseOver) {
                mSelectedObjects.insert(std::map<Entity*,Location>::value_type(mouseOver, Location()));
                mouseOver->setSelected(true);
                SILOG(input,info,"Replaced selection with " << mouseOver->id());
                // Fire selected event.
            }
        }
        return EventResponse::cancel();
    }

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

    EventResponse groupObjects(EventPtr ev) {
        return EventResponse::nop();
    }

    EventResponse rotateSelection(EventPtr ev) {
        return EventResponse::nop();
    }
    EventResponse moveSelection(EventPtr ev) {
        std::tr1::shared_ptr<MouseDragEvent> mouseev (
            std::tr1::dynamic_pointer_cast<MouseDragEvent>(ev));
        if (!mouseev) {
            return EventResponse::nop();
        }
        
        if (mSelectedObjects.empty()) {
            SILOG(input,debug,"moveSelection: Found no selected objects");
            return EventResponse::nop();
        }
        CameraEntity *camera = mParent->mPrimaryCamera;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();
        if (mouseev->mType == MouseDragEvent::START) {

            float moveDistance;
            bool foundObject = false;
            for (std::map<Entity*,Location>::iterator iter = mSelectedObjects.begin();
                 iter != mSelectedObjects.end(); ++iter) {
                (*iter).second = (*iter).first->getProxy().extrapolateLocation(now);
                Vector3d deltaPosition ((*iter).first->getProxy().globalLocation(now).getPosition() - cameraLoc.getPosition());
                double dist = deltaPosition.dot(Vector3d(cameraAxis));
                if (!foundObject || dist < moveDistance) {
                    foundObject = true;
                    moveDistance = dist;
                    mMoveVector = deltaPosition;
                }
            }
            SILOG(input,debug,"moveSelection: Moving selected objects at distance " << mMoveVector);
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
        SILOG(input,debug,"Start "<<start<<"; End "<<end<<"; toMove "<<toMove);
        for (std::map<Entity*,Location>::const_iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            Location toSet ((*iter).first->getProxy().extrapolateLocation(now));
            SILOG(input,debug,"moveSelection: OLD " << toSet.getPosition());
            toSet.setPosition((*iter).second.getPosition() + toMove);
            SILOG(input,debug,"moveSelection: NEW " << toSet.getPosition());
            (*iter).first->getProxy().setPositionVelocity(now, toSet);
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
                Vector3d totalPosition(0,0,0);
                for (std::map<Entity*,Location>::const_iterator iter = mSelectedObjects.begin();
                     iter != mSelectedObjects.end(); ++iter) {
                    totalPosition += ((*iter).first->getProxy().globalLocation(now).getPosition());
                }
                totalPosition /= mSelectedObjects.size();
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
            Vector3d totalPosition(0,0,0);
            for (std::map<Entity*,Location>::const_iterator iter = mSelectedObjects.begin();
                 iter != mSelectedObjects.end(); ++iter) {
                totalPosition += ((*iter).first->getProxy().globalLocation(now).getPosition());
            }
            totalPosition /= mSelectedObjects.size();
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

        Vector3d totalPosition(0,0,0);
        for (std::map<Entity*,Location>::const_iterator iter = mSelectedObjects.begin();
             iter != mSelectedObjects.end(); ++iter) {
            totalPosition += ((*iter).first->getProxy().globalLocation(now).getPosition());
        }
        totalPosition /= mSelectedObjects.size();

        Location cameraLoc = camera->getProxy().globalLocation(now);
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
        if (mParent->mInputManager->isModifierDown(InputDevice::MOD_CTRL)) {
            toMove *= WORLD_SCALE;
        } else if (mParent->rayTrace(cameraLoc.getPosition(), direction(cameraLoc.getOrientation()), distance) && 
                (distance*.75 < WORLD_SCALE || mParent->mInputManager->isModifierDown(InputDevice::MOD_SHIFT))) {
            toMove *= distance*.75;
        } else if (!mSelectedObjects.empty()) {
            Vector3d totalPosition(0,0,0);
            for (std::map<Entity*,Location>::const_iterator iter = mSelectedObjects.begin();
                 iter != mSelectedObjects.end(); ++iter) {
                totalPosition += ((*iter).first->getProxy().globalLocation(now).getPosition());
            }
            totalPosition /= mSelectedObjects.size();
            toMove *= (totalPosition - cameraLoc.getPosition()).length() * .75;
            SILOG(input,debug,"Total Position: "<<totalPosition);
        } else {
            toMove *= WORLD_SCALE;
        }
        toMove *= value.getCentered(); // up == zoom in
        SILOG(input,debug,"ZOOMIN: "<<toMove);
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

    std::vector<SubscriptionId> mMiddleDragListeners;
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
    MouseHandler(OgreSystem *parent) : mParent(parent), mIsRotating(false),mPanDistance(0),mIsOrbiting(false) {
        mEvents.push_back(mParent->mInputManager->registerDeviceListener(
            std::tr1::bind(&MouseHandler::deviceListener, this, _1)));

        registerDragHandler(&MouseHandler::orbitDragHandler, 1);//shift
        registerDragHandler(&MouseHandler::panScene, 1);//ctrl
        registerDragHandler(&MouseHandler::rotateScene, 1);//none

        registerDragHandler(&MouseHandler::moveSelection, 3);
        registerDragHandler(&MouseHandler::middleDragHandler, 2);
        mEvents.push_back(mParent->mInputManager->subscribeId(
                              IdPair(MouseClickEvent::getEventId(), 
                                     MouseDownEvent::getSecondaryId(1)),
                              std::tr1::bind(&MouseHandler::selectObject, this, _1)));
    }
    ~MouseHandler() {
        for (std::vector<SubscriptionId>::const_iterator iter = mEvents.begin(); 
             iter != mEvents.end(); 
             ++iter) {
            mParent->mInputManager->unsubscribe(*iter);
        }
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

}
}
