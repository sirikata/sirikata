/*  Meru
 *  ResourceDownloadTask.cpp
 *
 *  Copyright (c) 2009, Stanford University
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


#include "DistanceDownloadPlanner.hpp"
#include <sirikata/ogre/Entity.hpp>
#include <stdlib.h>
#include <algorithm>
#include <math.h>

#include <sirikata/core/network/IOStrandImpl.hpp>

#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include <sirikata/ogre/resourceManager/CDNArchive.hpp>

#include <OgreMeshManager.h>
#include <OgreMaterialManager.h>
#include <OgreSkeletonManager.h>

#include "ManualMaterialLoader.hpp"
#include "ManualSkeletonLoader.hpp"
#include "ManualMeshLoader.hpp"

#include <sirikata/ogre/WebViewManager.hpp>

#include <FreeImage.h>

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;
using namespace Sirikata::Graphics;

namespace Sirikata {
namespace Graphics {

DistanceDownloadPlanner::Resource::Resource(Graphics::Entity *m, const Transfer::URI& mesh_uri, ProxyObjectPtr _proxy)
: file(mesh_uri),
  mesh(m),
  name(m->id()),
  loaded(false),
  priority(0),
  proxy(_proxy)
{}

DistanceDownloadPlanner::Asset::Asset(const Transfer::URI& name)
 : uri(name)
{
    textureFingerprints = TextureBindingsMapPtr(new TextureBindingsMap());
}

DistanceDownloadPlanner::Asset::~Asset() {
    for(WebMaterialList::iterator it = webMaterials.begin(); it != webMaterials.end(); it++)
        WebViewManager::getSingleton().destroyWebView(*it);
    webMaterials.clear();
}

DistanceDownloadPlanner::DistanceDownloadPlanner(Context* c, OgreRenderer* renderer)
 : ResourceDownloadPlanner(c, renderer)
{
    mCDNArchive = CDNArchiveFactory::getSingleton().addArchive();
    mActiveCDNArchive = true;
}

DistanceDownloadPlanner::~DistanceDownloadPlanner()
{
    if (mActiveCDNArchive) {
        CDNArchiveFactory::getSingleton().removeArchive(mCDNArchive);
        mActiveCDNArchive=false;
    }
}

void DistanceDownloadPlanner::addResource(Resource* r) {
    mResources[r->name] = r;
    mWaitingResources[r->name] = r;
    checkShouldLoadNewResource(r);
}

DistanceDownloadPlanner::Resource* DistanceDownloadPlanner::findResource(const String& name) {
    ResourceMap::iterator it = mResources.find(name);
    return (it != mResources.end() ? it->second : NULL);
}

void DistanceDownloadPlanner::removeResource(const String& name) {
    ResourceMap::iterator it = mResources.find(name);
    if (it != mResources.end()) {
        ResourceMap::iterator loaded_it = mLoadedResources.find(name);
        if (loaded_it != mLoadedResources.end()) mLoadedResources.erase(loaded_it);

        ResourceMap::iterator waiting_it = mWaitingResources.find(name);
        if (waiting_it != mWaitingResources.end()) mWaitingResources.erase(waiting_it);

        delete it->second;
        mResources.erase(it);
    }
}

void DistanceDownloadPlanner::addNewObject(Entity *ent, const Transfer::URI& mesh) {
    addResource(new Resource(ent, mesh));
}

void DistanceDownloadPlanner::addNewObject(ProxyObjectPtr p, Entity *mesh) {
    addResource(new Resource(mesh, p->getMesh(), p));
}

void DistanceDownloadPlanner::updateObject(ProxyObjectPtr p) {
    Resource* r = findResource(p->getObjectReference().toString());
    URI last_file = r->file;
    r->file = p->getMesh();
    r->priority = calculatePriority(p);
    if (r->file != last_file && r->loaded) {
        requestAssetForResource(r);
    }
}

void DistanceDownloadPlanner::removeObject(ProxyObjectPtr p) {
    removeResource(p->getObjectReference().toString());
}

void DistanceDownloadPlanner::removeObject(Graphics::Entity* mesh) {
    removeResource(mesh->id());
}


double DistanceDownloadPlanner::calculatePriority(ProxyObjectPtr proxy)
{
    if (camera == NULL || !proxy) return 0;

    Vector3d cameraLoc = camera->getPosition();
    Vector3d objLoc = proxy->getPosition();
    Vector3d diff = cameraLoc - objLoc;

    /*double diff2d = sqrt(pow(diff.x, 2) + pow(diff.y, 2));
      double diff3d = sqrt(pow(diff2d, 2) + pow(diff.x, 2));*/

    if (diff.length() <= 0) return 1.0;

    double priority = 1/(diff.length());

    return priority;
}

void DistanceDownloadPlanner::checkShouldLoadNewResource(Resource* r) {
    if ((int32)mLoadedResources.size() < mMaxLoaded)
        loadResource(r);
}

bool DistanceDownloadPlanner::budgetRequiresChange() const {
    return
        ((int32)mLoadedResources.size() < mMaxLoaded && !mWaitingResources.empty()) ||
        ((int32)mLoadedResources.size() > mMaxLoaded && !mLoadedResources.empty());

}

void DistanceDownloadPlanner::loadResource(Resource* r) {
    mWaitingResources.erase(r->name);
    mLoadedResources[r->name] = r;

    r->loaded = true;
    requestAssetForResource(r);
}

void DistanceDownloadPlanner::unloadResource(Resource* r) {
    mLoadedResources.erase(r->name);
    mWaitingResources[r->name] = r;

    r->loaded = false;
    r->mesh->unload();
}

void DistanceDownloadPlanner::poll()
{
    if (camera == NULL) return;

    // Update priorities, tracking the largest undisplayed priority and the
    // smallest displayed priority to decide if we're going to have to swap.
    float32 mMinLoadedPriority = 1000000, mMaxWaitingPriority = 0;
    for (ResourceMap::iterator it = mResources.begin(); it != mResources.end(); it++) {
        Resource* r = it->second;
        r->priority = calculatePriority(r->proxy);

        if (r->loaded)
            mMinLoadedPriority = std::min(mMinLoadedPriority, (float32)r->priority);
        else
            mMaxWaitingPriority = std::max(mMaxWaitingPriority, (float32)r->priority);
    }

    // If the min and max computed above are on the wrong sides, then, we need
    // to do more work to figure out which things to swap between loaded &
    // waiting.
    //
    // Or, if we've just gone under budget (e.g. the max number
    // allowed increased, objects left the scene, etc) then we also
    // run this, which will safely add if we're under budget.
    if (mMinLoadedPriority < mMaxWaitingPriority || budgetRequiresChange()) {
        std::vector<Resource*> loaded_resource_heap;
        std::vector<Resource*> waiting_resource_heap;

        for (ResourceMap::iterator it = mResources.begin(); it != mResources.end(); it++) {
            Resource* r = it->second;
            if (r->loaded)
                loaded_resource_heap.push_back(r);
            else
                waiting_resource_heap.push_back(r);
        }
        std::make_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Resource::MinHeapComparator());
        std::make_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Resource::MaxHeapComparator());

        while(true) {
            if ((int32)mLoadedResources.size() < mMaxLoaded && !waiting_resource_heap.empty()) {
                // If we're under budget, just add to top waiting items
                Resource* max_waiting = waiting_resource_heap.front();
                std::pop_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Resource::MaxHeapComparator());
                waiting_resource_heap.pop_back();

                loadResource(max_waiting);
            }
            else if ((int32)mLoadedResources.size() > mMaxLoaded && !loaded_resource_heap.empty()) {
                // Otherwise, extract the min and check if we can continue
                Resource* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Resource::MinHeapComparator());
                loaded_resource_heap.pop_back();

                unloadResource(min_loaded);
            }
            else if (!waiting_resource_heap.empty() && !loaded_resource_heap.empty()) {
                // They're equal, we're (potentially) exchanging
                Resource* max_waiting = waiting_resource_heap.front();
                std::pop_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Resource::MaxHeapComparator());
                waiting_resource_heap.pop_back();

                Resource* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Resource::MinHeapComparator());
                loaded_resource_heap.pop_back();

                if (min_loaded->priority < max_waiting->priority) {
                    unloadResource(min_loaded);
                    loadResource(max_waiting);
                }
                else {
                    break;
                }
            }
            else {
                break;
            }
        }
    }
}

void DistanceDownloadPlanner::stop()
{
}

void DistanceDownloadPlanner::requestAssetForResource(Resource* forResource) {
    if (forResource->file.empty()) {
        forResource->mesh->loadEmpty();
        return;
    }

    Asset* asset = NULL;
    // First make sure we have an Asset for this
    AssetMap::iterator asset_it = mAssets.find(forResource->file);
    if (asset_it == mAssets.end()) {
        asset = new Asset(forResource->file);
        mAssets.insert( AssetMap::value_type(forResource->file, asset) );
    }
    else {
        asset = asset_it->second;
    }

    // If we're already setup for this asset, just leave things alone
    // FIXME priorities...
    if (asset->waitingResources.find(forResource->id()) != asset->waitingResources.end()) return;

    asset->waitingResources.insert(forResource->id());

    // Another Resource might have already requested downloading
    // FIXME update priority
    if (!asset->downloadTask)
        downloadAsset(asset, forResource);
}

void DistanceDownloadPlanner::downloadAsset(Asset* asset, Resource* forResource) {
    asset->downloadTask =
        AssetDownloadTask::construct(
            asset->uri, getScene(), forResource->priority,
            mContext->mainStrand->wrap(
                std::tr1::bind(&DistanceDownloadPlanner::loadAsset, this, asset->uri)
            ));
}

void DistanceDownloadPlanner::loadAsset(Transfer::URI asset_uri) {
    if (mAssets.find(asset_uri) == mAssets.end()) return;
    Asset* asset = mAssets[asset_uri];

    //get the mesh data and check that it is valid.
    bool usingDefault = false;
    Mesh::VisualPtr visptr = asset->downloadTask->asset();

    if (!visptr) {
        usingDefault = true;
        visptr = getScene()->defaultMesh();
        if (!visptr) {
            finishLoadAsset(asset, false);
            return;
        }
    }

    Mesh::MeshdataPtr mdptr( std::tr1::dynamic_pointer_cast<Mesh::Meshdata>(visptr) );
    if (mdptr) {
        loadMeshdata(asset, mdptr, usingDefault);
        finishLoadAsset(asset, true);
        return;
    }

    Mesh::BillboardPtr bbptr( std::tr1::dynamic_pointer_cast<Mesh::Billboard>(visptr) );
    if (bbptr) {
        loadBillboard(asset, bbptr, usingDefault);
        finishLoadAsset(asset, true);
        return;
    }

    // If we got here, we don't know how to load it
    SILOG(ogre, error, "Failed to load asset failed because it doesn't know how to load " << visptr->type());
    finishLoadAsset(asset, false);
    return;
}

void DistanceDownloadPlanner::finishLoadAsset(Asset* asset, bool success) {
    // We need to notify all Resources (objects) waiting for this to load that
    // it finished (or failed)
    for(ResourceSet::iterator it = asset->waitingResources.begin(); it != asset->waitingResources.end(); it++) {
        const String& resource_id = *it;
        // It may not even need it anymore if its not in the set of objects we
        // currently wnat loaded anymore (or not even exist anymore)
        ResourceMap::iterator rit = mResources.find(resource_id);
        if (rit == mResources.end()) continue;
        Resource* resource = rit->second;
        if (resource->loaded == false) continue;
        // It may even have switched meshes by now.
        if (resource->file != asset->uri) continue;

        // Otherwise, we just inform it of whether it can load or not
        if (!success) {
            resource->mesh->loadFailed();
        }
        else {
            Mesh::MeshdataPtr mdptr( std::tr1::dynamic_pointer_cast<Mesh::Meshdata>(asset->downloadTask->asset()) );
            if (mdptr) {
                resource->mesh->loadMesh(mdptr, asset->ogreAssetName, asset->animations);
                continue;
            }

            Mesh::BillboardPtr bbptr( std::tr1::dynamic_pointer_cast<Mesh::Billboard>(asset->downloadTask->asset()) );
            if (bbptr) {
                resource->mesh->loadBillboard(bbptr, asset->ogreAssetName);
                continue;
            }
        }
    }
    asset->downloadTask.reset();
    asset->waitingResources.clear();
}

namespace {

// Utility to compute full hash of the content of a visual. This is needed
// because the hash of some date (e.g. a collada file) may be the same, but
// some textures may differ (the relative names are the same, but the data
// the absolute names point to differs).
SHA256 computeVisualHash(const Mesh::VisualPtr& visptr, AssetDownloadTaskPtr assetDownload) {
    // To compute a hash of the entire visual, we just combine the hashes of the
    // original and all the dependent resources, then hash the result. The
    // approach right now uses strings and could do somethiing like xor'ing
    // everything instead, but this is easy and shouldn't happen all that often.
    String data = visptr->hash.toString();

    for(AssetDownloadTask::Dependencies::const_iterator tex_it = assetDownload->dependencies().begin(); tex_it != assetDownload->dependencies().end(); tex_it++) {
        const AssetDownloadTask::ResourceData& tex_data = tex_it->second;
        data += tex_data.request->getMetadata().getFingerprint().toString();
    }

    return SHA256::computeDigest(data);
}

bool ogreHasMesh(const String& meshname) {
    Ogre::MeshPtr mp = Ogre::MeshManager::getSingleton().getByName(meshname);
    return !mp.isNull();
}

}


void DistanceDownloadPlanner::loadMeshdata(Asset* asset, const Mesh::MeshdataPtr& mdptr, bool usingDefault) {
    AssetDownloadTaskPtr assetDownload = asset->downloadTask;

    //Extract any animations from the new mesh.
    for (uint32 i=0;  i < mdptr->nodes.size(); i++) {
      Sirikata::Mesh::Node& node = mdptr->nodes[i];

      for (std::map<String, Sirikata::Mesh::TransformationKeyFrames>::iterator it = node.animations.begin();
           it != node.animations.end(); it++)
        {
          asset->animations.insert(it->first);
        }
    }


    SHA256 sha = mdptr->hash;
    String hash = sha.convertToHexString();
    String meshname = computeVisualHash(mdptr, assetDownload).toString();
    asset->ogreAssetName = meshname;

    // If we already have it, just load the existing one
    if (ogreHasMesh(meshname)) return;

    loadDependentTextures(asset, usingDefault);

    if (!mdptr->instances.empty()) {
        Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();
        int index=0;
        for (Mesh::MaterialEffectInfoList::const_iterator mat=mdptr->materials.begin(),mate=mdptr->materials.end();mat!=mate;++mat,++index) {
            std::string matname = ogreMaterialName(meshname, index);
            Ogre::MaterialPtr matPtr=matm.getByName(matname);
            if (matPtr.isNull()) {
                Ogre::ManualResourceLoader * reload;
                matPtr = matm.create(
                    matname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                    (reload=new ManualMaterialLoader (mdptr, matname, *mat, asset->uri, asset->textureFingerprints))
                );

                reload->prepareResource(&*matPtr);
                reload->loadResource(&*matPtr);
            }
        }

        // Skeleton
        Ogre::SkeletonPtr skel(NULL);
        if (!mdptr->joints.empty()) {

            Ogre::SkeletonManager& skel_mgr = Ogre::SkeletonManager::getSingleton();
            Ogre::ManualResourceLoader *reload;
            skel = Ogre::SkeletonPtr(skel_mgr.create(ogreSkeletonName(meshname),Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
                                                     (reload=new ManualSkeletonLoader(mdptr, asset->animations))));
            reload->prepareResource(&*skel);
            reload->loadResource(&*skel);

            if (! ((ManualSkeletonLoader*)reload)->wasSkeletonLoaded()) {
              skel.setNull();
            }
        }


        // Mesh
        {
            Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
            /// FIXME: set bounds, bounding radius here
            Ogre::ManualResourceLoader *reload;
            Ogre::MeshPtr mo (mm.createManual(meshname,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,(reload=
#ifdef _WIN32
#ifdef NDEBUG
			OGRE_NEW
#else
			new
#endif
#else
			OGRE_NEW
#endif
                        ManualMeshLoader(mdptr, meshname))));
            reload->prepareResource(&*mo);
            reload->loadResource(&*mo);

            if (!skel.isNull())
                mo->_notifySkeleton(skel);


            bool check = mm.resourceExists(meshname);
        }
    }

    // Lights
    /* FIXME these need to be instanced per object they are loaded for...
    int light_idx = 0;
    Meshdata::LightInstanceIterator lightinst_it = mdptr->getLightInstanceIterator();
    uint32 lightinst_idx;
    Matrix4x4f pos_xform;
    while( lightinst_it.next(&lightinst_idx, &pos_xform) ) {
        const LightInstance& lightinst = mdptr->lightInstances[lightinst_idx];

        // Get the instanced submesh
        if(lightinst.lightIndex >= (int)mdptr->lights.size()){
            SILOG(ogre,error, "bad light index %d for lights only sized %d\n"<<lightinst.lightIndex<<"/"<<(int)mdptr->lights.size());
            continue;
        }
        const LightInfo& sublight = mdptr->lights[lightinst.lightIndex];

        String lightname = ogreLightName(mName, meshname, light_idx++);
        Ogre::Light* light = constructOgreLight(getScene()->getSceneManager(), lightname, sublight);
        if (!light->isAttached()) {
            mLights.push_back(light);

            // Lights just assume local position at the origin. We just need to
            // transform appropriately.
            Vector4f v_xform = pos_xform * Vector4f(0.f, 0.f, 0.f, 1.f);
            // The light has an extra scene node to handle the specific transformation
            Ogre::SceneNode* xformnode = mScene->getSceneManager()->createSceneNode();
            xformnode->translate(v_xform[0], v_xform[1], v_xform[2]);
            // Rotation needs to b handled by extracting rotation information.
            Quaternion qrot(pos_xform.extract3x3());
            xformnode->rotate(toOgre(qrot));
            xformnode->attachObject(light);

            mSceneNode->addChild(xformnode);
            //light->setDebugDisplayEnabled(true);
        }
    }
    */
}

void DistanceDownloadPlanner::loadBillboard(Asset* asset, const Mesh::BillboardPtr& bbptr, bool usingDefault) {
    AssetDownloadTaskPtr assetDownload = asset->downloadTask;

    SHA256 sha = bbptr->hash;
    String hash = sha.convertToHexString();
    String bbname = computeVisualHash(bbptr, assetDownload).toString();

    loadDependentTextures(asset, usingDefault);

    // Load the single material
    Ogre::MaterialManager& matm = Ogre::MaterialManager::getSingleton();

    std::string matname = ogreMaterialName(bbname, 0);
    asset->ogreAssetName = matname;
    Ogre::MaterialPtr matPtr = matm.getByName(matname);
    if (matPtr.isNull()) {
        Ogre::ManualResourceLoader* reload;

        // We need to fill in a MaterialEffectInfo because the loader we're
        // using only knows how to process them for Meshdatas right now
        Mesh::MaterialEffectInfo matinfo;
        matinfo.shininess = 0.0f;
        matinfo.reflectivity = 1.0f;
        matinfo.textures.push_back(Mesh::MaterialEffectInfo::Texture());
        Mesh::MaterialEffectInfo::Texture& tex = matinfo.textures.back();
        tex.uri = bbptr->image;
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
            matname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true,
            (reload = new ManualMaterialLoader (bbptr, matname, matinfo, asset->uri, asset->textureFingerprints))
        );

        reload->prepareResource(&*matPtr);
        reload->loadResource(&*matPtr);
    }

    // The BillboardSet that actually gets rendered is like an Ogre::Entity,
    // load it in a similar way. There is no equivalent of the Ogre::Mesh -- we
    // only need to load up the material
}

namespace {
static void fixOgreURI(String &uri) {
    for (String::iterator i=uri.begin();i!=uri.end();++i) {
        if(*i=='.') *i='{';
    }
}
}

void DistanceDownloadPlanner::loadDependentTextures(Asset* asset, bool usingDefault) {
    // we currently assume no dependencies for default
    if (usingDefault) return;

    for(AssetDownloadTask::Dependencies::const_iterator tex_it = asset->downloadTask->dependencies().begin();
        tex_it != asset->downloadTask->dependencies().end();
        tex_it++)
    {
        const AssetDownloadTask::ResourceData& tex_data = tex_it->second;
        if (mActiveCDNArchive && asset->textureFingerprints->find(tex_data.request->getURI().toString()) == asset->textureFingerprints->end() ) {
            String id = tex_data.request->getURI().toString() + tex_data.request->getMetadata().getFingerprint().toString();
            fixOgreURI(id);

            (*asset->textureFingerprints)[tex_data.request->getURI().toString()] = id;

            // This could be a regular texture or a . If its ever a static
            // image, we want to decode it directly...
            FIMEMORY* mem_img_data = FreeImage_OpenMemory((BYTE*)tex_data.response->data(), tex_data.response->size());
            FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem_img_data);
            FreeImage_CloseMemory(mem_img_data);
            if (fif != FIF_UNKNOWN) {
                CDNArchiveFactory::getSingleton().addArchiveData(mCDNArchive,id,SparseData(tex_data.response));
            }
            else if (tex_data.request->getURI().scheme() == "http") {
                // Or, if its an http URL, we can try displaying it in a webview
                OGRE_LOG(detailed,"Using webview for " << id << ": " << tex_data.request->getURI());
                WebView* web_mat = WebViewManager::getSingleton().createWebViewMaterial(
                    mContext,
                    id,
                    512, 512 // Completely arbitrary...
                );
                web_mat->loadURL(tex_data.request->getURI().toString());
                asset->webMaterials.push_back(web_mat);
            }
        }
    }
}

} // namespace Graphics
} // namespace Sirikata
