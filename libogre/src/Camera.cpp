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

#include <sirikata/ogre/Camera.hpp>
#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/OgreConversions.hpp>
#include <sirikata/core/options/Options.hpp>
#include "Ogre.h"
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {
namespace Graphics {

Camera::Camera(OgreRenderer *scene, Ogre::SceneManager* scenemgr, const String& cameraName)
 : mScene(scene),
   mOgreSceneManager(scenemgr),
   mOgreCamera(NULL),
   mSceneNode(NULL),
   mRenderTarget(NULL),
   mViewport(NULL)
{
    mSceneNode = mOgreSceneManager->createSceneNode(cameraName);
    mSceneNode->setInheritScale(false);
    mOgreSceneManager->getRootSceneNode()->addChild(mSceneNode);


    if (mOgreSceneManager->hasCamera(cameraName))
        mOgreCamera = mOgreSceneManager->getCamera(cameraName);
    else
        mOgreCamera = mOgreSceneManager->createCamera(cameraName);

    mSceneNode->attachObject(mOgreCamera);

    mOgreCamera->setNearClipDistance(scene->nearPlane());
    mOgreCamera->setFarClipDistance(scene->farPlane());
}

Camera::~Camera() {
    if ((!mViewport) || (mViewport && mRenderTarget)) {
        detach();
    }

    mSceneNode->detachObject(mOgreCamera);
    mOgreSceneManager->destroyCamera(mOgreCamera);

    mSceneNode->removeAllChildren();
    mOgreSceneManager->destroySceneNode(mSceneNode);
}

Vector3d Camera::getPosition() const {
    return fromOgre(mSceneNode->getPosition(), mScene->getOffset());
}

void Camera::setPosition(const Vector3d& pos) {
    mSceneNode->setPosition(toOgre(pos, mScene->getOffset()));
}

Quaternion Camera::getOrientation() const {
    return fromOgre(mSceneNode->getOrientation());
}

void Camera::setOrientation(const Quaternion& orient) {
    mSceneNode->setOrientation(toOgre(orient));
}

void Camera::attach (const String&renderTargetName,uint32 width,uint32 height,Vector4f back_color, int zorder)
{
    this->detach();
    mRenderTarget = mScene->createRenderTarget(renderTargetName, width, height);
    mViewport = mRenderTarget->addViewport(mOgreCamera, zorder);
    mViewport->setBackgroundColour(Ogre::ColourValue(back_color.x, back_color.y, back_color.z, back_color.w));
    if (back_color.w < 1.f)
        mViewport->setClearEveryFrame(true, Ogre::FBT_DEPTH);
    this->setViewportDimensions(0.0f, 0.0f, 1.0f, 1.0f); // initial setting
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

void Camera::setViewportDimensions(int32 left, int32 top, int32 right, int32 bottom) {
    float32 wid = mRenderTarget->getWidth();
    float32 height = mRenderTarget->getHeight();

    setViewportDimensions(left / wid, top / height, right / wid, bottom / height);
}

void Camera::setViewportDimensions(float32 left, float32 top, float32 right, float32 bottom) {
    // We need to save these for if the window resizes.
    mViewportLeft = left; mViewportRight = right;
    mViewportTop = top; mViewportBottom = bottom;

    mViewport->setDimensions(left, top, right - left, bottom - top);
    // And fix up the aspect ratio since it very likely changed.
    mOgreCamera->setAspectRatio((float32)mViewport->getActualWidth()/(float32)mViewport->getActualHeight());
}

void Camera::windowResized() {
    // This just forces a resize so that the "actual dimensions" get updated to
    // reflect the new shape. Ogre just has an odd API where you set the size
    // based on float [0,1] coordinates but then it immediately computes and
    // continues to use the size based on the total size, e.g. [0,width] and
    // [0,height].
    this->setViewportDimensions(mViewportLeft, mViewportTop, mViewportRight, mViewportBottom);
}

}
}
