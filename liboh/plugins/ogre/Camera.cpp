#include <oh/Platform.hpp>
#include "Camera.hpp"
#include <options/Options.hpp>
namespace Sirikata {
namespace Graphics {
Camera::Camera(OgreSystem *scene,
           const UUID &id,
           String cameraName)
    : Entity(scene,
             id,
             scene->getSceneManager()->hasCamera(cameraName=id.readableHexData())?mCamera=scene->getSceneManager()->getCamera(cameraName):mCamera=scene->getSceneManager()->createCamera(cameraName)),mRenderTarget(NULL),mViewport(NULL) {
    mCamera->setNearClipDistance(scene->getOptions()->referenceOption("nearplane")->as<float32>());
    mCamera->setFarClipDistance(scene->getOptions()->referenceOption("farplane")->as<float32>());
}
void Camera::created() {

}
void Camera::destroyed() {

}

void Camera::attach (const String&renderTargetName,
                     uint32 width,
                     uint32 height){
    detatch();
    mRenderTarget = mScene->createRenderTarget(renderTargetName,
                                               width,
                                               height);
    mViewport= mRenderTarget->addViewport(mCamera);
    mViewport->setBackgroundColour(Ogre::ColourValue(0,.125,.25,1));
    mCamera->setAspectRatio((float32)mViewport->getActualWidth()/(float32)mViewport->getActualHeight());
}
void Camera::detatch() {
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
}

Camera::~Camera() {
    mSceneNode->detachAllObjects();
    mScene->getSceneManager()->destroyCamera(mId.readableHexData());
}


}
}
