/*  Sirikata Graphical Object Host
 *  Camera.cpp
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

#include "OgreSystem.hpp"
#include <sirikata/proxyobject/Platform.hpp>
#include "Camera.hpp"
#include <sirikata/core/options/Options.hpp>
#include "Entity.hpp"

namespace Sirikata {
namespace Graphics {

Camera::Camera(OgreSystem *scene, Entity* follow)
 : mScene(scene),
   mOgreCamera(NULL),
   mSceneNode(NULL),
   mRenderTarget(NULL),
   mViewport(NULL),
   mFollowing(follow),
   mMode(FirstPerson),
   mOffset(Vector3d(0, 0, 0))
{
    String cameraName = ogreCameraName(following()->id());

    mSceneNode = scene->getSceneManager()->createSceneNode(cameraName);
    mSceneNode->setInheritScale(false);
    mScene->getSceneManager()->getRootSceneNode()->addChild(mSceneNode);


    if (scene->getSceneManager()->hasCamera(cameraName))
        mOgreCamera = scene->getSceneManager()->getCamera(cameraName);
    else
        mOgreCamera = scene->getSceneManager()->createCamera(cameraName);

    mSceneNode->attachObject(mOgreCamera);

    mOgreCamera->setNearClipDistance(scene->getOptions()->referenceOption("nearplane")->as<float32>());
    mOgreCamera->setFarClipDistance(scene->getOptions()->referenceOption("farplane")->as<float32>());

    Vector3d goalPos = mFollowing->getProxyPtr()->getPosition();
    Quaternion goalOrient = mFollowing->getProxyPtr()->getOrientation();
    mSceneNode->setPosition(toOgre(goalPos, mFollowing->getScene()->getOffset()));
    mSceneNode->setOrientation(toOgre(goalOrient));

    setMode(FirstPerson);
}

Camera::~Camera() {
    if ((!mViewport) || (mViewport && mRenderTarget)) {
        detach();
    }

    mSceneNode->detachObject(mOgreCamera);
    mScene->getSceneManager()->destroyCamera(mOgreCamera);

    mSceneNode->removeAllChildren();
    mScene->getSceneManager()->destroySceneNode(mSceneNode);
}

Entity* Camera::following() const {
    return mFollowing;
}

void Camera::setMode(Mode m) {
    mMode = m;
    mFollowing->setVisible( mMode == FirstPerson ? false : true );
}

Vector3d Camera::getPosition() const {
    return fromOgre(mSceneNode->getPosition(), mFollowing->getScene()->getOffset());
}
Quaternion Camera::getOrientation() const {
    return fromOgre(mSceneNode->getOrientation());
}

void Camera::attach (const String&renderTargetName,uint32 width,uint32 height,Vector4f back_color)
{
    this->detach();
    mRenderTarget = mScene->createRenderTarget(renderTargetName,
                                               width,
                                               height);
    mViewport= mRenderTarget->addViewport(mOgreCamera);
    mViewport->setBackgroundColour(Ogre::ColourValue(back_color.x, back_color.y, back_color.z, back_color.w));
    mOgreCamera->setAspectRatio((float32)mViewport->getActualWidth()/(float32)mViewport->getActualHeight());
    mScene->attachCamera(renderTargetName,this);
}


void Camera::detach() {
    if (mViewport && mRenderTarget) {
        mRenderTarget->removeViewport(mViewport->getZOrder());
    }else {
        assert(!mViewport);
    }
    if (mRenderTarget) {
        mScene->destroyRenderTarget(mRenderTarget->getName());
        mRenderTarget=NULL;
    }
    mScene->detachCamera(this);
}

void Camera::windowResized() {
    // This just forces a resize so that the "actual dimensions" get updated to
    // reflect the new shape. Ogre just has an odd API where you set the size
    // based on float [0,1] coordinates but then it immediately computes and
    // continues to use the size based on the total size, e.g. [0,width] and
    // [0,height].
    mViewport->setDimensions(0.f, 0.f, 1.f, 1.f);
    // In any case, our camera's aspect ratio would be screwed up, so now fix
    // that up.
    mOgreCamera->setAspectRatio((float32)mViewport->getActualWidth()/(float32)mViewport->getActualHeight());
}

void Camera::tick(const Time& t, const Duration& dt) {
    Vector3d goalPos = mFollowing->getOgrePosition();
    Quaternion goalOrient = mFollowing->getOgreOrientation();

    Vector3d pos;
    Quaternion orient;

    if (mMode == FirstPerson) {
        // In first person mode we are tied tightly to the position of
        // the object.
        pos = goalPos;
        orient = goalOrient;
    }
    else {
        // In third person mode, the target is offset so we'll be behind and
        // above ourselves and we need to interpolate to the target.
        // Offset the goal.
        BoundingSphere3f following_bounds = mFollowing->getProxyPtr()->getBounds();
        goalPos += Vector3d(following_bounds.center());
        // > 1 factor gets us beyond the top of the object
        goalPos += mOffset * (following_bounds.radius());
        // Restore the current values from the scene node.
        pos = fromOgre(mSceneNode->getPosition(), mFollowing->getScene()->getOffset());
        orient = fromOgre(mSceneNode->getOrientation());
        // And interpolate.
        Vector3d toGoal = goalPos-pos;
        double toGoalLen = toGoal.length();
        if (toGoalLen < 1e-06) {
            pos = goalPos;
            orient = goalOrient;
        } else {
            double step = exp(-dt.seconds()*2.f);
            pos = goalPos - (toGoal/toGoalLen)*(toGoalLen*step);
            orient = (goalOrient*(1.f-step) + orient*step).normal();
        }
    }

    mSceneNode->setPosition(toOgre(pos, mFollowing->getScene()->getOffset()));
    mSceneNode->setOrientation(toOgre(orient));
}


std::string Camera::ogreCameraName(const SpaceObjectReference&ref) {
    return "Camera:"+ref.toString();
}

}
}
