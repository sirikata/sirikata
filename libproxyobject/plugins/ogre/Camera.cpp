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

namespace Sirikata {
namespace Graphics {

Camera::Camera(OgreSystem *scene, Entity* follow)
 : mScene(scene),
   mOgreCamera(NULL),
   mSceneNode(NULL),
   mRenderTarget(NULL),
   mViewport(NULL),
   mFollowing(follow)
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

void Camera::attach (const String&renderTargetName,uint32 width,uint32 height)
{
    this->detach();
    mRenderTarget = mScene->createRenderTarget(renderTargetName,
                                               width,
                                               height);
    mViewport= mRenderTarget->addViewport(mOgreCamera);
    mViewport->setBackgroundColour(Ogre::ColourValue(0,.125,.25,1));
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

void Camera::tick() {
    mSceneNode->setPosition(toOgre( mFollowing->getOgrePosition(), mFollowing->getScene()->getOffset() ));
    mSceneNode->setOrientation(toOgre( mFollowing->getOgreOrientation() ));
}


std::string Camera::ogreCameraName(const SpaceObjectReference&ref) {
    return "Camera:"+ref.toString();
}

}
}
