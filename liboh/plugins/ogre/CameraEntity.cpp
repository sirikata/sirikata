/*  Sirikata Graphical Object Host
 *  CameraEntity.cpp
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

#include <proxyobject/Platform.hpp>
#include "CameraEntity.hpp"
#include <options/Options.hpp>
namespace Sirikata {
namespace Graphics {
CameraEntity::CameraEntity(OgreSystem *scene,
                           const std::tr1::shared_ptr<ProxyCameraObject> &pco,
                           std::string ogreName)
    : Entity(scene,
             pco,
             ogreName.length()?ogreName:ogreName=ogreCameraName(pco->getObjectReference()),
             NULL),
      mRenderTarget(NULL),
      mViewport(NULL) {
    mAttachedIter=scene->mAttachedCameras.end();
    getProxy().CameraProvider::addListener(this);
    String cameraName = ogreName;
    if (scene->getSceneManager()->hasCamera(cameraName)) {
        init(scene->getSceneManager()->getCamera(cameraName));
    } else {
        init(scene->getSceneManager()->createCamera(cameraName));
    }
    getOgreCamera()->setNearClipDistance(scene->getOptions()->referenceOption("nearplane")->as<float32>());
    getOgreCamera()->setFarClipDistance(scene->getOptions()->referenceOption("farplane")->as<float32>());
}

void CameraEntity::attach (const String&renderTargetName,
                     uint32 width,
                     uint32 height){
    this->detach();
    mRenderTarget = mScene->createRenderTarget(renderTargetName,
                                               width,
                                               height);
    mViewport= mRenderTarget->addViewport(getOgreCamera());
    mViewport->setBackgroundColour(Ogre::ColourValue(0,.125,.25,1));
    getOgreCamera()->setAspectRatio((float32)mViewport->getActualWidth()/(float32)mViewport->getActualHeight());
    mAttachedIter = mScene->attachCamera(renderTargetName,this);
}
void CameraEntity::detach() {
    if (mViewport&&mRenderTarget) {
        mRenderTarget->removeViewport(mViewport->getZOrder());
/*
  unsigned int numViewports=sm->getNumViewports();
  for (unsigned int i=0;i<numViewports;++i){
  if (sm->getViewport(i)==mViewport) {
  sm->removeViewport(i);
  break;
  }
  }
*/
    }else {
        assert(!mViewport);
    }
    if (mRenderTarget) {
        mScene->destroyRenderTarget(mRenderTarget->getName());
        mRenderTarget=NULL;
    }
    mAttachedIter=mScene->detachCamera(mAttachedIter);

}

CameraEntity::~CameraEntity() {
    if ((!mViewport) || (mViewport && mRenderTarget)) {
        detach();
    }
    if (mAttachedIter != mScene->mAttachedCameras.end()) {
        mScene->mAttachedCameras.erase(mAttachedIter);
    }
    Ogre::Camera*toDestroy=getOgreCamera();
    init(NULL);
    mScene->getSceneManager()->destroyCamera(toDestroy);
    getProxy().CameraProvider::removeListener(this);
}
std::string CameraEntity::ogreCameraName(const SpaceObjectReference&ref) {
    return "Camera:"+ref.toString();
}
std::string CameraEntity::ogreMovableName()const{
    return ogreCameraName(id());
}

}
}
