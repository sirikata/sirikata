#include <oh/Platform.hpp>
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

    mAttachedIter = mScene->mAttachedCameras.insert(mScene->mAttachedCameras.end(), this);
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
    if (mAttachedIter != mScene->mAttachedCameras.end()) {
        mScene->mAttachedCameras.erase(mAttachedIter);
        mAttachedIter = mScene->mAttachedCameras.end();
    }
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
