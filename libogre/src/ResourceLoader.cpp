// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include "ResourceLoader.hpp"
#include "ManualMeshLoader.hpp"
#include "ManualSkeletonLoader.hpp"
#include "ManualMaterialLoader.hpp"
#include "ManualSkeletonLoader.hpp"
#include "ManualMeshLoader.hpp"

#include <OgreMeshManager.h>
#include <OgreSkeletonManager.h>
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>

#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgrePass.h>

#include <sirikata/core/util/Time.hpp>

#define OGRERL_LOG(lvl, msg) SILOG(ogre-resource-loader, lvl, msg)

namespace Sirikata {
namespace Graphics {

ResourceLoader::ResourceLoader(Context* ctx, const Duration& per_frame_time)
 : mPerFrameTime(per_frame_time),
   mNeedRenderQueuesReset(false)
{
    mProfilerStage = ctx->profiler->addStage("Ogre Resource Loader");
}

ResourceLoader::~ResourceLoader() {
    delete mProfilerStage;
}

void ResourceLoader::loadMaterial(const String& name, Mesh::MeshdataPtr mesh, const Mesh::MaterialEffectInfo& mat, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb) {
    incRefCount(name, ResourceTypeMaterial);
    mTasks.push( std::tr1::bind(&ResourceLoader::loadMaterialWork, this, name, mesh, mat, uri, textureFingerprints, cb) );
}

void ResourceLoader::loadMaterialWork(const String& name, Mesh::MeshdataPtr mesh, const Mesh::MaterialEffectInfo& mat, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb) {
    Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr matPtr = matm.getByName(name);
    if (matPtr.isNull()) {
        Ogre::ManualResourceLoader * reload;
        matPtr = matm.create(
            name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
            (reload=new ManualMaterialLoader (mesh, name, mat, uri, textureFingerprints))
        );

        reload->prepareResource(&*matPtr);
        reload->loadResource(&*matPtr);
    }
    if (cb) cb();
}

void ResourceLoader::loadBillboardMaterial(const String& name, const String& texuri, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb) {
    incRefCount(name, ResourceTypeMaterial);
    mTasks.push( std::tr1::bind(&ResourceLoader::loadBillboardMaterialWork, this, name, texuri, uri, textureFingerprints, cb) );
}

void ResourceLoader::loadBillboardMaterialWork(const String& name, const String& texuri, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb) {
    Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr matPtr = matm.getByName(name);
    if (matPtr.isNull()) {
        Ogre::ManualResourceLoader* reload;

        // We need to fill in a MaterialEffectInfo because the loader we're
        // using only knows how to process them for Meshdatas right now
        Mesh::MaterialEffectInfo matinfo;
        matinfo.shininess = 0.0f;
        matinfo.reflectivity = 1.0f;
        matinfo.textures.push_back(Mesh::MaterialEffectInfo::Texture());
        Mesh::MaterialEffectInfo::Texture& tex = matinfo.textures.back();
        tex.uri = texuri;
        tex.color = Vector4f(1.f, 1.f, 1.f, 1.f);
        tex.texCoord = 0;
        tex.affecting = Mesh::MaterialEffectInfo::Texture::AMBIENT;
        tex.samplerType = Mesh::MaterialEffectInfo::Texture::SAMPLER_TYPE_2D;
        tex.minFilter = Mesh::MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR;
        tex.magFilter = Mesh::MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR;
        tex.wrapS = Mesh::MaterialEffectInfo::Texture::WRAP_MODE_NONE;
        tex.wrapT = Mesh::MaterialEffectInfo::Texture::WRAP_MODE_NONE;
        tex.wrapU = Mesh::MaterialEffectInfo::Texture::WRAP_MODE_NONE;
        tex.mipBias = 0.0f;
        tex.maxMipLevel = 20;

        matPtr = matm.create(
            name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
            (reload = new ManualMaterialLoader (Mesh::VisualPtr(), name, matinfo, uri, textureFingerprints))
        );

        reload->prepareResource(&*matPtr);
        reload->loadResource(&*matPtr);
    }

    if (cb) cb();
}



void ResourceLoader::loadSkeleton(const String& name, Mesh::MeshdataPtr mesh, const std::set<String>& animationList, LoadedCallback cb) {
    incRefCount(name, ResourceTypeSkeleton);
    mTasks.push( std::tr1::bind(&ResourceLoader::loadSkeletonWork, this, name, mesh, animationList, cb) );
}

void ResourceLoader::loadSkeletonWork(const String& name, Mesh::MeshdataPtr mesh, const std::set<String>& animationList, LoadedCallback cb) {
    Ogre::SkeletonManager& skel_mgr = Ogre::SkeletonManager::getSingleton();
    Ogre::SkeletonPtr skel = skel_mgr.getByName(name);
    if (skel.isNull()) {
        Ogre::ManualResourceLoader *reload;
        Ogre::SkeletonPtr skel = Ogre::SkeletonPtr(skel_mgr.create(name,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                (reload=new ManualSkeletonLoader(mesh, animationList))));
        reload->prepareResource(&*skel);
        reload->loadResource(&*skel);
    }
    if (cb) cb();
}


void ResourceLoader::loadMesh(const String& name, Mesh::MeshdataPtr mesh, const String& skeletonName, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb) {
    incRefCount(name, ResourceTypeMesh);
    mTasks.push( std::tr1::bind(&ResourceLoader::loadMeshWork, this, name, mesh, skeletonName, textureFingerprints, cb) );
}

void ResourceLoader::loadMeshWork(const String& name, Mesh::MeshdataPtr mesh, const String& skeletonName, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb) {
    Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
    Ogre::MeshPtr mo = mm.getByName(name);
    if (mo.isNull()) {
        /// FIXME: set bounds, bounding radius here
        Ogre::ManualResourceLoader *reload;
        mo = mm.createManual(name,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,(reload=
#ifdef _WIN32
#ifdef NDEBUG
                OGRE_NEW
#else
                new
#endif
#else
                OGRE_NEW
#endif
                ManualMeshLoader(mesh, textureFingerprints)));
        reload->prepareResource(&*mo);
        reload->loadResource(&*mo);

        if (!skeletonName.empty()) {
            Ogre::SkeletonManager& skel_mgr = Ogre::SkeletonManager::getSingleton();
            Ogre::SkeletonPtr skel = skel_mgr.getByName(skeletonName);
            if (!skel.isNull())
                mo->_notifySkeleton(skel);
        }
    }
    if (cb) cb();
}


void ResourceLoader::loadTexture(const String& name, LoadedCallback cb) {
    incRefCount(name, ResourceTypeTexture);
    mTasks.push( std::tr1::bind(&ResourceLoader::loadTextureWork, this, name, cb) );
}

void ResourceLoader::loadTextureWork(const String& name, LoadedCallback cb) {
    Ogre::TextureManager& tm = Ogre::TextureManager::getSingleton();
    tm.load(name,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    if (cb) cb();
}

void ResourceLoader::unloadResource(const String& name) {
    decRefCount(name);
}

void ResourceLoader::incRefCount(const String& name, ResourceType type) {
    RefCountMap::iterator it = mRefCounts.find(name);
    if (it == mRefCounts.end())
        mRefCounts.insert( RefCountMap::value_type(name, ResourceData(type)) );
    else
        it->second.refcount++;
}

void ResourceLoader::decRefCount(const String& name) {
    RefCountMap::iterator it = mRefCounts.find(name);
    assert(it != mRefCounts.end());
    it->second.refcount--;
    if (it->second.refcount == 0) {
        // Schedule unloading. This is performed as a task because
        // loading may not have actually occurred yet -- despite
        // refcount > 0 it may still be sitting in the task
        // queue. This way we guarantee ordering.
        OGRERL_LOG(detailed, "Unloading " << name);
        mTasks.push( std::tr1::bind(&ResourceLoader::unloadResourceWork, this, name, it->second.type) );
        mRefCounts.erase(it);
    }
}

void ResourceLoader::unloadResourceWork(const String& name, ResourceType type) {
    switch(type) {
      case ResourceTypeMaterial:
          {
              Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
              matm.remove(name);
              mNeedRenderQueuesReset = true;
          }
        break;
      case ResourceTypeTexture:
          {
              Ogre::TextureManager& texm = Ogre::TextureManager::getSingleton();
              texm.remove(name);
          }
        break;
      case ResourceTypeSkeleton:
          {
              Ogre::SkeletonManager& skelm = Ogre::SkeletonManager::getSingleton();
              skelm.remove(name);
          }
        break;
      case ResourceTypeMesh:
          {
              Ogre::MeshManager& meshm = Ogre::MeshManager::getSingleton();
              meshm.remove(name);
          }
        break;
    }
}


void ResourceLoader::tick() {
    if (mTasks.empty()) return;

    mProfilerStage->started();

    static Duration null_offset = Duration::zero();
    Time start = Time::now(null_offset);

    while(!mTasks.empty()) {
        mTasks.front()();
        mTasks.pop();

        Time end = Time::now(null_offset);
        if (end - start > mPerFrameTime) break;
    }

    if (mNeedRenderQueuesReset)
        resetRenderQueues();

    mProfilerStage->finished();
}

void ResourceLoader::resetRenderQueues() {
    Ogre::Root& root = Ogre::Root::getSingleton();
    Ogre::SceneManagerEnumerator::SceneManagerIterator scenesIter = root.getSceneManagerIterator();
    while (scenesIter.hasMoreElements()) {
        Ogre::SceneManager* pScene = scenesIter.getNext();
        if (pScene) {
            Ogre::RenderQueue* pQueue = pScene->getRenderQueue();
            if (pQueue) {
                pQueue->clear(true);
            }
        }
    }
    Ogre::Pass::processPendingPassUpdates();
    mNeedRenderQueuesReset = false;
}

} // namespace Graphics
} // namespace Sirikata
