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
#include "DragActions.hpp"
#include <task/Time.hpp>

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

void pixelToRadians(CameraEntity *cam, float deltaXPct, float deltaYPct, float &xRadians, float &yRadians) {
    // This function is useless and hopelessly broken, since radians have no meaning in perspective. Use pixelToDirection instead!!!
    SILOG(input,info,"FOV Y Radians: "<<cam->getOgreCamera()->getFOVy().valueRadians()<<"; aspect = "<<cam->getOgreCamera()->getAspectRatio());
    xRadians = cam->getOgreCamera()->getFOVy().valueRadians() * cam->getOgreCamera()->getAspectRatio() * deltaXPct;
    yRadians = cam->getOgreCamera()->getFOVy().valueRadians() * deltaYPct;
    SILOG(input,info,"X = "<<deltaXPct<<"; Y = "<<deltaYPct);
    SILOG(input,info,"Xradian = "<<xRadians<<"; Yradian = "<<yRadians);
}
// Uses perspective
Vector3f pixelToDirection(CameraEntity *cam, Quaternion orient, float xPixel, float yPixel) {
    float xRadian, yRadian;
    //pixelToRadians(cam, xPixel/2, yPixel/2, xRadian, yRadian);
    xRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * cam->getOgreCamera()->getAspectRatio() * xPixel;
    yRadian = sin(cam->getOgreCamera()->getFOVy().valueRadians()*.5) * yPixel;

    return Vector3f(-orient.zAxis()*cos(cam->getOgreCamera()->getFOVy().valueRadians()*.5) +
                    orient.xAxis() * xRadian +
                    orient.yAxis() * yRadian);
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





class MoveObjectDrag : public ActiveDrag {
    std::vector<ProxyPositionObjectPtr> mSelectedObjects;
    std::vector<Vector3d> mPositions;
    CameraEntity *camera;
    OgreSystem *mParent;
    Vector3d mMoveVector;
public:
    MoveObjectDrag(const DragStartInfo &info)
        : mSelectedObjects(info.objects.begin(), info.objects.end()) {
        camera = info.camera;
        mParent = info.sys;
        Task::AbsTime now = Task::AbsTime::now();
        float moveDistance = 0.f; // Will be reset on first foundObject
        bool foundObject = false;
        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();
        mMoveVector = Vector3d(0,0,0);
        for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
            mPositions.push_back(mSelectedObjects[i]->extrapolateLocation(now).getPosition());
            Vector3d deltaPosition (mSelectedObjects[i]->globalLocation(now).getPosition() - cameraLoc.getPosition());
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
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3f cameraAxis = -cameraLoc.getOrientation().zAxis();

        Vector3d startAxis (pixelToDirection(camera, cameraLoc.getOrientation(), ev->mXStart, ev->mYStart));
        Vector3d endAxis (pixelToDirection(camera, cameraLoc.getOrientation(), ev->mX, ev->mY));
        Vector3d start, end;
        if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_SHIFT)) {
            start = startAxis.normal() * (mMoveVector.y/startAxis.y);
            end = endAxis.normal() * (mMoveVector.y/endAxis.y);
        } else if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_CTRL)) {
            start = startAxis.normal() * (mMoveVector.z/startAxis.z);
            end = endAxis.normal() * (mMoveVector.z/endAxis.z);
        } else {
            float moveDistance = mMoveVector.dot(Vector3d(cameraAxis));
            start = startAxis * moveDistance; // / cameraAxis.dot(startAxis);
            end = endAxis * moveDistance; // / cameraAxis.dot(endAxis);
        }
        Vector3d toMove (end - start);
		// Prevent moving outside of a small radius so you don't shoot an object into the horizon.
		if (toMove.length() > 10*mParent->getInputManager()->mWorldScale->as<float>()) {
			// moving too much.
			toMove *= (10*mParent->getInputManager()->mWorldScale->as<float>()/toMove.length());
		}
        SILOG(input,debug,"Start "<<start<<"; End "<<end<<"; toMove "<<toMove);
        for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
            Location toSet (mSelectedObjects[i]->extrapolateLocation(now));
            SILOG(input,debug,"moveSelection: OLD " << toSet.getPosition());
            toSet.setPosition(mPositions[i] + toMove);
            SILOG(input,debug,"moveSelection: NEW " << toSet.getPosition());
            mSelectedObjects[i]->setPositionVelocity(now, toSet);
        }
    }
};
DragActionRegistry::RegisterClass<MoveObjectDrag> moveobj("moveObject");

class RotateObjectDrag : public ActiveDrag {
    OgreSystem *mParent;
    std::vector<ProxyPositionObjectPtr> mSelectedObjects;
    std::vector<Quaternion > mOriginalRotation;
public:
    RotateObjectDrag(const DragStartInfo &info)
        : mParent (info.sys),
          mSelectedObjects (info.objects.begin(), info.objects.end()) {
		mOriginalRotation.reserve(mSelectedObjects.size());
		Task::AbsTime now = Task::AbsTime::now();
		for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
			Location currentLoc = mSelectedObjects[i]->extrapolateLocation(now);
			mOriginalRotation.push_back(currentLoc.getOrientation());
		}
    }
    void mouseMoved(MouseDragEventPtr ev) {
            Task::AbsTime now(Task::AbsTime::now());
            // one screen width = one full rotation
			float SNAP_RADIANS = mParent->getInputManager()->mRotateSnap->as<float>();
            float radianX = 0;
            float radianY = 0;
            float radianZ = 0;
            float sensitivity = 0.25;
            if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_ALT)) {
                sensitivity = 0.1;
            }
            if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_SHIFT)) {
                radianX = 3.14159 * 2 * -ev->deltaY() * sensitivity;
                if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_CTRL)) {
                    radianZ = 3.14159 * 2 * -ev->deltaX() * sensitivity;
                }
            }
            else if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_CTRL)) {
                radianZ = 3.14159 * 2 * -ev->deltaY() * sensitivity;
            }
            else {
                radianY = 3.14159 * 2 * ev->deltaX() * sensitivity;
            }
            for (size_t i = 0; i< mSelectedObjects.size(); ++i) {
                const ProxyPositionObjectPtr &ent = mSelectedObjects[i];
                Location loc (ent->extrapolateLocation(now));
                loc.setOrientation(
                        Quaternion(Vector3f(1,0,0),radianX)*
                        Quaternion(Vector3f(0,1,0),radianY)*
                        Quaternion(Vector3f(0,0,1),radianZ)*
                        mOriginalRotation[i]);
                ent->resetPositionVelocity(now, loc);
            }
    }
};
DragActionRegistry::RegisterClass<RotateObjectDrag> rotateobj("rotateObject");

class ScaleObjectDrag : public RelativeDrag {
    OgreSystem *mParent;
    std::vector<ProxyPositionObjectPtr> mSelectedObjects;
    float dragMultiplier;
public:
    ScaleObjectDrag(const DragStartInfo &info)
        : RelativeDrag(info.ev->getDevice()),
          mParent (info.sys),
          mSelectedObjects (info.objects.begin(), info.objects.end()) {
        dragMultiplier = mParent->getInputManager()->mDragMultiplier->as<float>();
    }
    void mouseMoved(MouseDragEventPtr ev) {
        if (ev->deltaLastY() != 0) {
            float scaleamt = exp(dragMultiplier*ev->deltaLastY());
            for (size_t i = 0; i < mSelectedObjects.size(); ++i) {
                const ProxyPositionObjectPtr &ent = mSelectedObjects[i];
                if (!ent) {
                    continue;
                }
                std::tr1::shared_ptr<ProxyMeshObject> meshptr (
                    std::tr1::dynamic_pointer_cast<ProxyMeshObject>(ent));
                if (meshptr) {
                    meshptr->setScale(meshptr->getScale() * scaleamt);
                }
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
    CameraEntity *camera;
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
    CameraEntity *camera;
    bool mRelativePan;
    double mPanDistance;
    OgreSystem *mParent;
    Vector3f toMove;
public:
    PanCameraDrag(const DragStartInfo &info) {
        camera = info.camera;
        mParent = info.sys;
        double distance;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        toMove = Vector3f(
            pixelToDirection(camera, cameraLoc.getOrientation(), info.ev->mXStart, info.ev->mYStart));
        mRelativePan = false;
        mStartPan = camera->getProxy().extrapolateLocation(now).getPosition();
		if (mParent->getInputManager()->isModifierDown(InputDevice::MOD_CTRL)) {
			float WORLD_SCALE = mParent->getInputManager()->mWorldScale->as<float>();
            mPanDistance = WORLD_SCALE;
		} else if (!mParent->getInputManager()->isModifierDown(InputDevice::MOD_SHIFT) &&
				   info.sys->rayTrace(cameraLoc.getPosition(), toMove, distance)) {
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
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        if (mPanDistance) {
            float radianX, radianY;
            pixelToRadians(camera, ev->deltaX(), ev->deltaY(), radianX, radianY);
            panCamera(camera, mStartPan, Vector3d(-radianX*mPanDistance, -radianY*mPanDistance, 0));
        }
    }
};
DragActionRegistry::RegisterClass<PanCameraDrag> pancamera("panCamera");

void zoomInOut(AxisValue value, const InputDevicePtr &dev, CameraEntity *camera, std::set<ProxyPositionObjectPtr> objects, OgreSystem *parent) {
    if (!dev) {
        return;
    }
    SILOG(input,debug,"zoom "<<value);

    Task::AbsTime now = Task::AbsTime::now();
    Location cameraLoc = camera->getProxy().globalLocation(now);
    Vector3d toMove;

    toMove = Vector3d(
        pixelToDirection(camera, cameraLoc.getOrientation(),
                         dev->getAxis(PointerDevice::CURSORX).getCentered(),
                         dev->getAxis(PointerDevice::CURSORY).getCentered()));
    double distance;
    float WORLD_SCALE = parent->getInputManager()->mWorldScale->as<float>();
    if (!parent->getInputManager()->isModifierDown(InputDevice::MOD_CTRL) &&
        !parent->getInputManager()->isModifierDown(InputDevice::MOD_SHIFT)) {
        toMove *= WORLD_SCALE;
    } else if (parent->rayTrace(cameraLoc.getPosition(), direction(cameraLoc.getOrientation()), distance) &&
               (distance*.75 < WORLD_SCALE || parent->getInputManager()->isModifierDown(InputDevice::MOD_SHIFT))) {
        toMove *= distance*.75;
    } else if (!objects.empty()) {
        Vector3d totalPosition (averageSelectedPosition(now, objects.begin(), objects.end()));
        toMove *= (totalPosition - cameraLoc.getPosition()).length() * .75;
    } else {
        toMove *= WORLD_SCALE;
    }
    toMove *= value.getCentered(); // up == zoom in
    cameraLoc.setPosition(cameraLoc.getPosition() + toMove);
    camera->getProxy().resetPositionVelocity(now, cameraLoc);
}

class ZoomCameraDrag : public RelativeDrag {
    OgreSystem *mParent;
    CameraEntity *mCamera;
    std::set<ProxyPositionObjectPtr> mSelection;
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

 */

class OrbitObjectDrag : public RelativeDrag {
    OgreSystem *mParent;
    CameraEntity *camera;
    std::vector<ProxyPositionObjectPtr> mSelectedObjects;
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
        double distance;
        Task::AbsTime now = Task::AbsTime::now();
        Location cameraLoc = camera->getProxy().globalLocation(now);
        Vector3d amount (ev->deltaX(), ev->deltaY(), 0);
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
            Vector3d totalPosition (averageSelectedPosition(now, mSelectedObjects.begin(), mSelectedObjects.end()));
            double multiplier = (totalPosition - cameraLoc.getPosition()).length();
            rotateCamera(camera, amount.x, amount.y);
            panCamera(camera, camera->getProxy().extrapolatePosition(now), amount * multiplier);
        }
    }
};

DragActionRegistry::RegisterClass<OrbitObjectDrag> orbit("orbitObject");


}
}
