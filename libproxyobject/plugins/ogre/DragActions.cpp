/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  DragActions.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/proxyobject/Platform.hpp>
#include "OgreSystem.hpp"
#include "Camera.hpp"
#include "Entity.hpp"
#include "input/SDLInputManager.hpp"
#include "DragActions.hpp"
#include <sirikata/core/task/Time.hpp>

namespace Sirikata {
namespace Graphics {

using namespace Input;
using namespace Task;

DragActionRegistry &DragActionRegistry::getSingleton() {
    static DragActionRegistry *sSingleton = NULL;
    if (!sSingleton) {
        sSingleton = new DragActionRegistry;
    }
    return *sSingleton;
}

void DragActionRegistry::set(const std::string &name, const DragAction &obj) {
    getSingleton().mRegistry[name] = obj;
}

void DragActionRegistry::unset(const std::string &name) {
    std::tr1::unordered_map<std::string, DragAction>::iterator iter =
        getSingleton().mRegistry.find(name);
    if (iter != getSingleton().mRegistry.end()) {
        getSingleton().mRegistry.erase(iter);
    }
}

class RelativeDrag : public ActiveDrag {
    PointerDevicePtr mDevice;
public:
    RelativeDrag(const PointerDevicePtr &dev) : mDevice(dev) {
        if (dev) {
            mDevice->pushRelativeMode();
        }
    }
    ~RelativeDrag() {
        if (mDevice) {
            mDevice->popRelativeMode();
        }
    }
};

class NullDrag : public ActiveDrag {
public:
    NullDrag(const DragStartInfo &info) {}
    void mouseMoved(MouseDragEventPtr ev) {}
};

DragAction nullDragAction (ActiveDrag::factory<NullDrag>);

const DragAction &DragActionRegistry::get(const std::string &name) {
    std::tr1::unordered_map<std::string, DragAction>::iterator iter =
        getSingleton().mRegistry.find(name);
    if (iter == getSingleton().mRegistry.end()) {
        return nullDragAction;
    }
    return iter->second;
}

void pixelToRadians(Camera *cam, float deltaXPct, float deltaYPct, float &xRadians, float &yRadians) {
    // This function is useless and hopelessly broken, since radians have no meaning in perspective. Use pixelToDirection instead!!!
    SILOG(input,info,"FOV Y Radians: "<<cam->getOgreCamera()->getFOVy().valueRadians()<<"; aspect = "<<cam->getOgreCamera()->getAspectRatio());
    xRadians = cam->getOgreCamera()->getFOVy().valueRadians() * cam->getOgreCamera()->getAspectRatio() * deltaXPct;
    yRadians = cam->getOgreCamera()->getFOVy().valueRadians() * deltaYPct;
    SILOG(input,info,"X = "<<deltaXPct<<"; Y = "<<deltaYPct);
    SILOG(input,info,"Xradian = "<<xRadians<<"; Yradian = "<<yRadians);
}
// Uses perspective
Vector3f pixelToDirection(Camera *cam, Quaternion orient, float xPixel, float yPixel) {
    float xRadian, yRadian;
    //pixelToRadians(cam, xPixel/2, yPixel/2, xRadian, yRadian);
    xRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * cam->getOgreCamera()->getAspectRatio() * xPixel;
    yRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * yPixel;

    return Vector3f(-orient.zAxis()*cos(cam->getOgreCamera()->getFOVy().valueRadians()*.5) +
                    orient.xAxis() * xRadian +
                    orient.yAxis() * yRadian);
}


void rotateCamera(Camera *camera, float radianX, float radianY) {
    Time now = camera->following()->getScene()->simTime();

    Quaternion orient(camera->following()->getProxy().globalLocation(now).getOrientation());
    Quaternion dragStart (camera->following()->getProxy().extrapolateLocation(now).getOrientation());
    Quaternion dhorient = Quaternion(Vector3f(0,1,0),radianX);
    Quaternion dvorient = Quaternion(dhorient * orient * Vector3f(1,0,0),-radianY);

    Location location = camera->following()->getProxy().extrapolateLocation(now);
    location.setOrientation((dvorient * dhorient * dragStart).normal());
    //camera->following()->getProxy().resetLocation(now, location);
}

    void panCamera(Camera *camera, const Vector3d &oldLocalPosition, const Vector3d &toPan) {
        Time now = camera->following()->getScene()->simTime();

        Quaternion orient(camera->following()->getProxy().globalLocation(now).getOrientation());

        Location location (camera->following()->getProxy().extrapolateLocation(now));
        location.setPosition(orient * toPan + oldLocalPosition);
        //camera->following()->getProxy().resetLocation(now, location);
    }





class MoveObjectDrag : public ActiveDrag {
    std::vector<ProxyObjectWPtr> mSelectedObjects;
    std::vector<Vector3d> mPositions;
    Camera *camera;
    OgreSystem *mParent;
    Vector3d mMoveVector;
public:
    MoveObjectDrag(const DragStartInfo &info)
        : mSelectedObjects(info.objects.begin(), info.objects.end()) {
        camera = info.camera;
        mParent = info.sys;
        Time now = mParent->simTime();
        float moveDistance = 0.f; // Will be reset on first foundObject
        bool foundObject = false;
        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();
        mMoveVector = Vector3d(0,0,0);
        for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
            ProxyObjectPtr obj (mSelectedObjects[i].lock());
            if (!obj) {
                mPositions.push_back(Vector3d(0,0,0));
                continue;
            }
            mPositions.push_back(obj->extrapolateLocation(now).getPosition());
            Vector3d deltaPosition (obj->globalLocation(now).getPosition() - cameraLoc.getPosition());
            double dist = deltaPosition.dot(Vector3d(cameraAxis));
            if (!foundObject || dist < moveDistance) {
                foundObject = true;
                moveDistance = dist;
                mMoveVector = deltaPosition;
            }
        }
        SILOG(input,insane,"moveSelection: Moving selected objects at distance " << mMoveVector);
    }
    void mouseMoved(MouseDragEventPtr ev) {
        std::cout << "MOVE: mX = "<<ev->mX<<"; mY = "<<ev->mY<<". mXStart = "<< ev->mXStart<<"; mYStart = "<<ev->mYStart<<std::endl;
        if (mSelectedObjects.empty()) {
            SILOG(input,insane,"moveSelection: Found no selected objects");
            return;
        }

        Time now = mParent->simTime();


        /// dbm new way: ignore camera, just move along global axes
        Vector3d toMove(0,0,0);
        double sensitivity = 20.0;
        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();
        if (mParent->getInputManager()->isModifierDown(Input::MOD_ALT)) sensitivity = 5.0;
        if (mParent->getInputManager()->isModifierDown(Input::MOD_SHIFT &&
                mParent->getInputManager()->isModifierDown(Input::MOD_CTRL))) {
            toMove.y = ev->deltaY()*sensitivity;
        }
        else if (mParent->getInputManager()->isModifierDown(Input::MOD_SHIFT)) {
            if (cameraAxis.z > 0) sensitivity *=-1;
            toMove.x = ev->deltaX()*sensitivity;
        }
        else if (mParent->getInputManager()->isModifierDown(Input::MOD_CTRL)) {
            if (cameraAxis.x < 0) sensitivity *=-1;
            toMove.z = ev->deltaX()*sensitivity;
        }
        else {
            Vector3d startAxis (pixelToDirection(camera, cameraLoc.getOrientation(), ev->mXStart, ev->mYStart));
            Vector3d endAxis (pixelToDirection(camera, cameraLoc.getOrientation(), ev->mX, ev->mY));
            Vector3d start, end;
            float moveDistance = mMoveVector.dot(Vector3d(cameraAxis));
            start = startAxis * moveDistance; // / cameraAxis.dot(startAxis);
            end = endAxis * moveDistance; // / cameraAxis.dot(endAxis);
            toMove = (end - start);
            // Prevent moving outside of a small radius so you don't shoot an object into the horizon.
            if (toMove.length() > 10*mParent->getInputManager()->mWorldScale->as<float>()) {
                // moving too much.
                toMove *= (10*mParent->getInputManager()->mWorldScale->as<float>()/toMove.length());
            }
        }
        for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
            ProxyObjectPtr obj (mSelectedObjects[i].lock());
            if (obj) {
                Location toSet (obj->extrapolateLocation(now));
                SILOG(input,debug,"moveSelection: OLD " << toSet.getPosition());
                toSet.setPosition(mPositions[i] + toMove);
                SILOG(input,debug,"moveSelection: NEW " << toSet.getPosition());
                //obj->setLocation(now, toSet);
            }
        }
    }
};
DragActionRegistry::RegisterClass<MoveObjectDrag> moveobj("moveObject");

class RotateObjectDrag : public ActiveDrag {
    OgreSystem *mParent;
    std::vector<ProxyObjectWPtr> mSelectedObjects;
    std::vector<Quaternion > mOriginalRotation;
    std::vector<Vector3d> mOriginalPosition;
    Camera *camera;
public:
    RotateObjectDrag(const DragStartInfo &info)
            : mParent (info.sys),
            mSelectedObjects (info.objects.begin(), info.objects.end()) {
        camera = info.camera;
        mOriginalRotation.reserve(mSelectedObjects.size());
        mOriginalPosition.reserve(mSelectedObjects.size());
        Time now = mParent->simTime();
        for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
            ProxyObjectPtr obj (mSelectedObjects[i].lock());
            if (obj) {
                Location currentLoc = obj->extrapolateLocation(now);
                mOriginalRotation.push_back(currentLoc.getOrientation());
                mOriginalPosition.push_back(currentLoc.getPosition());
            } else {
                mOriginalRotation.push_back(Quaternion::identity());
                mOriginalPosition.push_back(Vector3d::nil());
            }
        }
    }
    void mouseMoved(MouseDragEventPtr ev) {
        Time now = mParent->simTime();

        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();
        float radianX = 0;
        float radianY = 0;
        float radianZ = 0;
        float sensitivity = 0.25;
        Vector3d avgPos(0,0,0);
        for (size_t i = 0; i< mSelectedObjects.size(); ++i) {
            ProxyObjectPtr ent (mSelectedObjects[i].lock());
            Location loc (ent->extrapolateLocation(now));
            avgPos += loc.getPosition();
        }
        avgPos /= mSelectedObjects.size();

        int ctlX, ctlZ;
        if ((cameraAxis.x > 0 && cameraAxis.z > 0)
                || (cameraAxis.x <= 0 && cameraAxis.z <= 0) ) {
            ctlX = Input::MOD_SHIFT;
            ctlZ = Input::MOD_CTRL;
        }
        else {
            ctlX = Input::MOD_CTRL;
            ctlZ = Input::MOD_SHIFT;
        }
        if (mParent->getInputManager()->isModifierDown(Input::MOD_ALT)) {
            sensitivity = 0.1;
        }
        if (mParent->getInputManager()->isModifierDown(ctlX)) {
            if (mParent->getInputManager()->isModifierDown(ctlZ)) {
                radianZ = 3.14159 * 2 * -ev->deltaX() * sensitivity;
            }
            else {
                if (cameraAxis.z > 0) sensitivity *=-1;
            }
            radianX = 3.14159 * 2 * -ev->deltaY() * sensitivity;
        }
        else if (mParent->getInputManager()->isModifierDown(ctlZ)) {
            if (cameraAxis.x <= 0) sensitivity *=-1;
            radianZ = 3.14159 * 2 * -ev->deltaY() * sensitivity;
        }
        else {
            radianY = 3.14159 * 2 * ev->deltaX() * sensitivity;
        }
        Quaternion dragRotation (   Quaternion(Vector3f(1,0,0),radianX)*
                                    Quaternion(Vector3f(0,1,0),radianY)*
                                    Quaternion(Vector3f(0,0,1),radianZ));

        for (size_t i = 0; i< mSelectedObjects.size(); ++i) {
            ProxyObjectPtr ent (mSelectedObjects[i].lock());
            if (!ent) continue;
            Location loc (ent->extrapolateLocation(now));
            Vector3d localTrans = mOriginalPosition[i] - avgPos;
            loc.setPosition(avgPos + dragRotation*localTrans);
            loc.setOrientation(dragRotation*mOriginalRotation[i]);
            //ent->resetLocation(now, loc);
        }
    }
};
DragActionRegistry::RegisterClass<RotateObjectDrag> rotateobj("rotateObject");

class ScaleObjectDrag : public RelativeDrag {
    OgreSystem *mParent;
    std::vector<ProxyObjectWPtr> mSelectedObjects;
    float dragMultiplier;
    std::vector<Vector3d> mOriginalPosition;
    float mTotalScale;
    Camera *camera;
public:
    ScaleObjectDrag(const DragStartInfo &info)
            : RelativeDrag(info.ev->getDevice()),
            mParent (info.sys),
            mSelectedObjects (info.objects.begin(), info.objects.end()),
            mTotalScale(1.0) {
        camera = info.camera;
        mOriginalPosition.reserve(mSelectedObjects.size());
        Time now = mParent->simTime();

        for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
            ProxyObjectPtr obj(mSelectedObjects[i].lock());
            Location currentLoc = obj->extrapolateLocation(now);
            mOriginalPosition.push_back(currentLoc.getPosition());
        }
        dragMultiplier = mParent->getInputManager()->mDragMultiplier->as<float>();
    }
    void mouseMoved(MouseDragEventPtr ev) {

        Time now = mParent->simTime();

        Vector3d avgPos(0,0,0);
        if (ev->deltaLastY() != 0) {
            float scaleamt = exp(dragMultiplier*ev->deltaLastY());
            mTotalScale *= scaleamt;
            int count = 0;
            for (size_t i = 0; i< mSelectedObjects.size(); ++i) {
                ProxyObjectPtr ent (mSelectedObjects[i].lock());
                if (!ent) {
                    continue;
                }
                Location loc (ent->extrapolateLocation(now));
                avgPos += loc.getPosition();
                ++count;
            }
            if (!count) return;
            avgPos /= count;
            for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
                ProxyObjectPtr ent (mSelectedObjects[i].lock());
                if (!ent) {
                    continue;
                }
                Location loc (ent->extrapolateLocation(now));
                Vector3d localTrans = mOriginalPosition[i] - avgPos;
                loc.setPosition(avgPos + localTrans*mTotalScale);
                std::cout << "debug avgPos: " << avgPos << " localTrans" << localTrans << " scale: " << mTotalScale << std::endl;
                //ent->resetLocation(now, loc);

                ent->setScale(ent->getScale()*scaleamt);
            }
        }
        if (ev->deltaLastX() != 0) {
            //rotateXZ(AxisValue::fromCentered(ev->deltaLastX()));
            //rotateCamera(mParent->mPrimaryCamera, ev->deltaLastX() * AXIS_TO_RADIANS, 0);
        }
    }
};
DragActionRegistry::RegisterClass<ScaleObjectDrag> scaleobj("scaleObject");

class RotateCameraDrag : public RelativeDrag {
    Camera *camera;
public:
    RotateCameraDrag(const DragStartInfo &info): RelativeDrag(info.ev->getDevice()) {
        camera = info.camera;
    }
    void mouseMoved(MouseDragEventPtr mouseev) {
        float radianX, radianY;
        pixelToRadians(camera, 2*mouseev->deltaLastX(), 2*mouseev->deltaLastY(), radianX, radianY);
        rotateCamera(camera, radianX, radianY);
    }
};
DragActionRegistry::RegisterClass<RotateCameraDrag> rotatecam("rotateCamera");

class PanCameraDrag : public ActiveDrag {
    Vector3d mStartPan;
    Camera *camera;
    bool mRelativePan;
    double mPanDistance;
    OgreSystem *mParent;
    Vector3f toMove;
public:
    PanCameraDrag(const DragStartInfo &info) {
        int hitCount=0;
        camera = info.camera;
        mParent = info.sys;
        double distance;
        int subent;
        Vector3f normal;
        Time now = mParent->simTime();
        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        toMove = Vector3f(
            pixelToDirection(camera, cameraLoc.getOrientation(), info.ev->mXStart, info.ev->mYStart));
        mRelativePan = false;
        mStartPan = camera->following()->getProxy().extrapolateLocation(now).getPosition();
		if (mParent->getInputManager()->isModifierDown(Input::MOD_CTRL)) {
			float WORLD_SCALE = mParent->getInputManager()->mWorldScale->as<float>();
            mPanDistance = WORLD_SCALE;
		} else if (!mParent->getInputManager()->isModifierDown(Input::MOD_SHIFT) &&
				   info.sys->rayTrace(cameraLoc.getPosition(), toMove, hitCount, distance, normal, subent)) {
            mPanDistance = distance;
        } else if (!info.objects.empty()) {
            Vector3d totalPosition(averageSelectedPosition(now, info.objects.begin(), info.objects.end()));

            mPanDistance = (totalPosition - cameraLoc.getPosition()).length();
        } else {
            float WORLD_SCALE = mParent->getInputManager()->mWorldScale->as<float>();
            mPanDistance = WORLD_SCALE;
        }
    }
    void mouseMoved(MouseDragEventPtr ev) {
        Time now = mParent->simTime();

        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        if (mPanDistance) {
            float radianX, radianY;
            pixelToRadians(camera, ev->deltaX(), ev->deltaY(), radianX, radianY);
            panCamera(camera, mStartPan, Vector3d(-radianX*mPanDistance, -radianY*mPanDistance, 0));
        }
    }
};
DragActionRegistry::RegisterClass<PanCameraDrag> pancamera("panCamera");

void zoomInOut(Input::AxisValue value, const Input::InputDevicePtr &dev, Camera *camera, const std::set<ProxyObjectWPtr>& objects, OgreSystem *parent) {
    if (!dev) return;
    float floatval = value.getCentered();
    Vector2f axes(
        dev->getAxis(Input::AXIS_CURSORX).getCentered(),
        dev->getAxis(Input::AXIS_CURSORY).getCentered()
    );
    zoomInOut(floatval, axes, camera, objects, parent);
}

void zoomInOut(float value, const Vector2f& axes, Camera *camera, const std::set<ProxyObjectWPtr>& objects, OgreSystem *parent) {
    SILOG(input,debug,"zoom "<<value);

    Time now = parent->simTime();

    Location cameraLoc = camera->following()->getProxy().extrapolateLocation(now);
    Location cameraGlobalLoc = camera->following()->getProxy().globalLocation(now);
    Vector3d toMove;
    int subent;

    toMove = Vector3d(pixelToDirection(camera, cameraLoc.getOrientation(), axes.x, axes.y));

    double distance;
    Vector3f normal;
    float WORLD_SCALE = parent->getInputManager()->mWorldScale->as<float>();
    int hitCount=0;
    if (!parent->getInputManager()->isModifierDown(Input::MOD_CTRL) &&
        !parent->getInputManager()->isModifierDown(Input::MOD_SHIFT)) {
        toMove *= WORLD_SCALE;
    } else if (parent->rayTrace(cameraGlobalLoc.getPosition(), direction(cameraGlobalLoc.getOrientation()), hitCount, distance, normal, subent) &&
               (distance*.75 < WORLD_SCALE || parent->getInputManager()->isModifierDown(Input::MOD_SHIFT))) {
        toMove *= distance*.75;
    } else if (!objects.empty()) {
        Vector3d totalPosition (averageSelectedPosition(now, objects.begin(), objects.end()));
        toMove *= (totalPosition - cameraGlobalLoc.getPosition()).length() * .75;
    } else {
        toMove *= WORLD_SCALE;
    }
    toMove *= value; // up == zoom in
    cameraLoc.setPosition(cameraLoc.getPosition() + toMove);
    //camera->following()->getProxy().resetLocation(now, cameraLoc);
}

class ZoomCameraDrag : public RelativeDrag {
    OgreSystem *mParent;
    Camera *mCamera;
    std::set<ProxyObjectWPtr> mSelection;
public:
    ZoomCameraDrag(const DragStartInfo &info)
        : RelativeDrag(info.ev->getDevice()) {
        mParent = info.sys;
        mCamera = info.camera;
        mSelection = info.objects;
    }
    void mouseMoved(MouseDragEventPtr ev) {
        if (ev->deltaLastY() != 0) {
            float dragMultiplier = mParent->getInputManager()->mDragMultiplier->as<float>();
            zoomInOut(AxisValue::fromCentered(dragMultiplier*ev->deltaLastY()), ev->getDevice(), mCamera, mSelection, mParent);
        }
    }
};
DragActionRegistry::RegisterClass<ZoomCameraDrag> zoomCamera("zoomCamera");


/*
    void orbitObject_BROKEN(AxisValue value) {
        SILOG(input,debug,"rotate "<<value);

        if (mSelectedObjects.empty()) {
            SILOG(input,debug,"rotateXZ: Found no selected objects");
            return;
        }
        Camera *camera = mParent->mPrimaryCamera;
        Time now(mParent->getLocalTimeOffset()->now(camera->following()->getProxy()));

        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        Vector3d totalPosition (averageSelectedPosition(now));
        Vector3d distance (cameraLoc.getPosition() - totalPosition);

        float radianX = value.getCentered() * 1.0;
        Quaternion dhorient = Quaternion(Vector3f(0,1,0), radianX);
        distance = dhorient * distance;
        Quaternion dhorient2 = Quaternion(Vector3f(0,1,0), -radianX);
        cameraLoc.setPosition(totalPosition + dhorient * distance);
        cameraLoc.setOrientation(dhorient2 * cameraLoc.getOrientation());
        camera->following()->getProxy().resetLocation(now, cameraLoc);
    }

 */

class OrbitObjectDrag : public RelativeDrag {
    OgreSystem *mParent;
    Camera *camera;
    std::vector<ProxyObjectWPtr> mSelectedObjects;
//    Vector3d mOrbitCenter;
//    Vector3d mOriginalPosition;
public:
    OrbitObjectDrag(const DragStartInfo &info)
        : RelativeDrag(info.ev->getDevice()),
         mSelectedObjects(info.objects.begin(), info.objects.end()) {
        mParent = info.sys;
        camera = info.camera;
    }
    void mouseMoved(MouseDragEventPtr ev) {

        Time now = mParent->simTime();

        Location cameraLoc = camera->following()->getProxy().globalLocation(now);
        Vector3d amount (ev->deltaX(), ev->deltaY(), 0);
/*
        Vector3f toMove (
            pixelToDirection(camera, cameraLoc.getOrientation(),
                             dev->getAxis(PointerDevice::CURSORX).getCentered(),
                             dev->getAxis(PointerDevice::CURSORY).getCentered()));
        if (mParent->rayTrace(cameraLoc.getPosition(), toMove, normal distance)) {
            rotateCamera(camera, amount.x, amount.y);
            panCamera(camera, amount * distance);
        } else */
        if (!mSelectedObjects.empty()) {
            Vector3d totalPosition (averageSelectedPosition(now, mSelectedObjects.begin(), mSelectedObjects.end()));
            double multiplier = (totalPosition - cameraLoc.getPosition()).length();
            rotateCamera(camera, amount.x, amount.y);
            panCamera(camera, camera->following()->getProxy().extrapolateLocation(now).getPosition(), amount * multiplier);
        }
    }
};

DragActionRegistry::RegisterClass<OrbitObjectDrag> orbit("orbitObject");


}
}
