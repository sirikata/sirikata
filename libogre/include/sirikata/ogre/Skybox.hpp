// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_SKYBOX_HPP_
#define _SIRIKATA_OGRE_SKYBOX_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>
#include <sirikata/core/util/Liveness.hpp>

namespace Sirikata {
namespace Graphics {

class ResourceLoader;

/** A skybox (or skydome, skyplane, etc). This isn't Visual because it doesn't
 *  really fit the idea of a single object to display (lacks bounds, no parser
 *  currently, etc.) and it is currently internal to Ogre.
 */
struct SIRIKATA_OGRE_EXPORT Skybox : public Liveness {
  public:
    enum SkyboxShape {
        SKYBOX_CUBE,
        SKYBOX_DOME,
        SKYBOX_PLANE
    };

    Skybox();
    Skybox(SkyboxShape shap, const String& img);
    virtual ~Skybox();

    operator bool() const { return shape != (SkyboxShape)-1 && !image.empty(); }

    void load(Ogre::SceneManager* scene_mgr, ResourceLoader* loader, Transfer::TransferPoolPtr tpool);
    void unload();

    // Properties
    SkyboxShape shape;
    String image;

    float32 distance;
    // Properties - skydome
    float32 tiling;
    float32 curvature;
    Quaternion orientation;

  private:
    void imageDownloadFinished(Liveness::Token alive, Transfer::ResourceDownloadTaskPtr taskptr,
        Transfer::TransferRequestPtr request, Transfer::DenseDataPtr response);
    void materialLoadFinished(Liveness::Token alive, String matid);


    // Data used to track
    unsigned int mCDNArchive;
    bool mActiveCDNArchive;

    // These get set when we load the skybox, are needed to perform unloading
    Ogre::SceneManager* mSceneManager;
    ResourceLoader* mResourceLoader;

    // Intermediate data
    Transfer::ResourceDownloadTaskPtr mImageDownload;

    // Data stored to track loaded assets and unload them.
    String mTextureID;
    String materialID() const;
    bool mLoaded;
};

typedef std::tr1::shared_ptr<Skybox> SkyboxPtr;
typedef std::tr1::weak_ptr<Skybox> SkyboxWPtr;


} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_SKYBOX_HPP_
