// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/ogre/Skybox.hpp>
#include "ResourceLoader.hpp"
#include <sirikata/ogre/resourceManager/CDNArchive.hpp>

namespace Sirikata {
namespace Graphics {

Skybox::Skybox()
 : shape((SkyboxShape)-1),
   image(),
   distance(4000),
   tiling(1),
   curvature(10),
   orientation(Quaternion::identity()),
   mCDNArchive(0),
   mActiveCDNArchive(false),
   mSceneManager(NULL),
   mResourceLoader(NULL),
   mTextureID(),
   mLoaded(false)
{
}

Skybox::Skybox(SkyboxShape shap, const String& img)
 : shape(shap),
   image(img),
   distance(4000),
   tiling(1),
   curvature(10),
   orientation(Quaternion::identity()),
   mCDNArchive(0),
   mActiveCDNArchive(false),
   mSceneManager(NULL),
   mResourceLoader(NULL),
   mTextureID(),
   mLoaded(false)
{
}

Skybox::~Skybox() {
    Liveness::letDie();

    unload();

    if (mActiveCDNArchive)
        CDNArchiveFactory::getSingleton().removeArchive(mCDNArchive);
}

String Skybox::materialID() const {
    return "mat_skybox_" + image;
}

void Skybox::load(Ogre::SceneManager* scene_mgr, ResourceLoader* loader, Transfer::TransferPoolPtr tpool) {
    mSceneManager = scene_mgr;
    mResourceLoader = loader;

    mImageDownload = Transfer::ResourceDownloadTask::construct(
        Transfer::URI(image), tpool, 100000.f, // Very high priority
        std::tr1::bind(&Skybox::imageDownloadFinished, this, livenessToken(), _1, _2, _3)
    );
    mImageDownload->start();
}

void Skybox::imageDownloadFinished(Liveness::Token alive, Transfer::ResourceDownloadTaskPtr taskptr,
    Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response)
{
    if (!alive) return;

    String tex_id = request->getIdentifier();
    if (!response) {
        SILOG(ogre, error, "Failed to load skybox because " << image << " (" << tex_id << ") couldn't be downloaded.");
        mImageDownload.reset();
        return;
    }

    // Mark as loaded now. Technically we're just putting it in the queue to
    // load, but after we do that, we'll need to send corresponding unloads if
    // something changes.
    mLoaded = true;
    mTextureID = tex_id;

    if (!mActiveCDNArchive) {
        mCDNArchive = CDNArchiveFactory::getSingleton().addArchive();
        mActiveCDNArchive = true;
    }

    // Load texture data. The callback is ignored because we only need to know
    // about the material finishing.
    CDNArchiveFactory::getSingleton().addArchiveData(mCDNArchive, mTextureID, Transfer::SparseData(response));
    mResourceLoader->loadTexture(mTextureID, ResourceLoader::LoadedCallback() );

    TextureBindingsMapPtr texfingerprints(new TextureBindingsMap);
    (*texfingerprints)[image] = mTextureID;
    mResourceLoader->loadBillboardMaterial(
        materialID(), image, Transfer::URI(), texfingerprints,
        std::tr1::bind(&Skybox::materialLoadFinished, this, livenessToken(), materialID())
    );
}

void Skybox::materialLoadFinished(Liveness::Token alive, String matid) {
    if (!alive) return;

    // We may have gotten an unload request in the meantime. Check whether we're
    // still marked for loading *and* that the loaded material id is the one we
    // still want.
    if (!mLoaded || matid != materialID())
        return;

    switch(shape) {
      case SKYBOX_CUBE:
          {
              mSceneManager->setSkyBox(true, materialID());
          }
        break;
      case SKYBOX_DOME:
          {
              Quaternion o = orientation.normal();
              mSceneManager->setSkyDome(true, materialID(),
                  curvature, tiling, distance, true,
                  Ogre::Quaternion(o.w, o.x, o.y, o.z)
              );
          }
        break;
      case SKYBOX_PLANE:
          {
              // FIXME
              //mSceneManager->setSkyPlane(true, materialID());
          }
        break;
    }
}


void Skybox::unload() {
    // Try to proactively cancel image downloads
    if (mImageDownload)
        mImageDownload.reset();

    // If we've started/finished the loading process, unload.
    if (mLoaded) {
        mResourceLoader->unloadResource(mTextureID);
        mResourceLoader->unloadResource(materialID());
        mLoaded = false;
    }
}





} // namespace Graphics
} // namespace Sirikata
