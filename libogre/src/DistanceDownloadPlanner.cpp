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
#include "ResourceLoader.hpp"

#include <sirikata/ogre/WebViewManager.hpp>

#include <FreeImage.h>

#define DLPLANNER_LOG(lvl,msg) SILOG(dlplanner, lvl, msg);

using namespace std;
using namespace Sirikata;
using namespace Sirikata::Transfer;
using namespace Sirikata::Graphics;

namespace Sirikata {
namespace Graphics {

DistanceDownloadPlanner::Object::Object(Graphics::Entity *m, const Transfer::URI& mesh_uri, ProxyObjectPtr _proxy)
: file(mesh_uri),
  mesh(m),
  name(m->id()),
  loaded(false),
  priority(0),
  proxy(_proxy)
{}

DistanceDownloadPlanner::Asset::Asset(const Transfer::URI& name)
 : uri(name),
   loadingResources(0)
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

void DistanceDownloadPlanner::addObject(Object* r) {
    mObjects[r->name] = r;
    mWaitingObjects[r->name] = r;
    checkShouldLoadNewObject(r);
}

DistanceDownloadPlanner::Object* DistanceDownloadPlanner::findObject(const String& name) {
    ObjectMap::iterator it = mObjects.find(name);
    return (it != mObjects.end() ? it->second : NULL);
}

void DistanceDownloadPlanner::removeObject(const String& name) {
    ObjectMap::iterator it = mObjects.find(name);
    if (it != mObjects.end()) {
        ObjectMap::iterator loaded_it = mLoadedObjects.find(name);
        if (loaded_it != mLoadedObjects.end()) mLoadedObjects.erase(loaded_it);

        ObjectMap::iterator waiting_it = mWaitingObjects.find(name);
        if (waiting_it != mWaitingObjects.end()) mWaitingObjects.erase(waiting_it);

        delete it->second;
        mObjects.erase(it);
    }
}

void DistanceDownloadPlanner::addNewObject(Entity *ent, const Transfer::URI& mesh) {
    addObject(new Object(ent, mesh));
}

void DistanceDownloadPlanner::addNewObject(ProxyObjectPtr p, Entity *mesh) {
    addObject(new Object(mesh, p->getMesh(), p));
}

void DistanceDownloadPlanner::updateObject(ProxyObjectPtr p) {
    Object* r = findObject(p->getObjectReference().toString());
    URI last_file = r->file;
    URI new_file = p->getMesh();
    if (new_file != last_file && r->loaded) {
        unrequestAssetForObject(r);
    }
    r->file = new_file;
    r->priority = calculatePriority(p);
    if (new_file != last_file && r->loaded) {
        requestAssetForObject(r);
    }
}

void DistanceDownloadPlanner::removeObject(ProxyObjectPtr p) {
    removeObject(p->getObjectReference().toString());
}

void DistanceDownloadPlanner::removeObject(Graphics::Entity* mesh) {
    removeObject(mesh->id());
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

void DistanceDownloadPlanner::checkShouldLoadNewObject(Object* r) {
    if ((int32)mLoadedObjects.size() < mMaxLoaded)
        loadObject(r);
}

bool DistanceDownloadPlanner::budgetRequiresChange() const {
    return
        ((int32)mLoadedObjects.size() < mMaxLoaded && !mWaitingObjects.empty()) ||
        ((int32)mLoadedObjects.size() > mMaxLoaded && !mLoadedObjects.empty());

}

void DistanceDownloadPlanner::loadObject(Object* r) {
    mWaitingObjects.erase(r->name);
    mLoadedObjects[r->name] = r;

    // After operation to get updated stats
    DLPLANNER_LOG(detailed, "Loading object " << r->name << " (" << r->file << "), " << mLoadedObjects.size() << " loaded, " << mWaitingObjects.size() << " waiting");

    r->loaded = true;
    requestAssetForObject(r);
}

void DistanceDownloadPlanner::unloadObject(Object* r) {
    mLoadedObjects.erase(r->name);
    mWaitingObjects[r->name] = r;

    // After operation to get updated stats
    DLPLANNER_LOG(detailed, "Unloading object " << r->name << " (" << r->file << "), " << mLoadedObjects.size() << " loaded, " << mWaitingObjects.size() << " waiting");

    r->loaded = false;
    unrequestAssetForObject(r);
}

void DistanceDownloadPlanner::poll()
{
    if (camera == NULL) return;

    // Update priorities, tracking the largest undisplayed priority and the
    // smallest displayed priority to decide if we're going to have to swap.
    float32 mMinLoadedPriority = 1000000, mMaxWaitingPriority = 0;
    for (ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
        Object* r = it->second;
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
        std::vector<Object*> loaded_resource_heap;
        std::vector<Object*> waiting_resource_heap;

        for (ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++) {
            Object* r = it->second;
            if (r->loaded)
                loaded_resource_heap.push_back(r);
            else
                waiting_resource_heap.push_back(r);
        }
        std::make_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Object::MinHeapComparator());
        std::make_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Object::MaxHeapComparator());

        while(true) {
            if ((int32)mLoadedObjects.size() < mMaxLoaded && !waiting_resource_heap.empty()) {
                // If we're under budget, just add to top waiting items
                Object* max_waiting = waiting_resource_heap.front();
                std::pop_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Object::MaxHeapComparator());
                waiting_resource_heap.pop_back();

                loadObject(max_waiting);
            }
            else if ((int32)mLoadedObjects.size() > mMaxLoaded && !loaded_resource_heap.empty()) {
                // Otherwise, extract the min and check if we can continue
                Object* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Object::MinHeapComparator());
                loaded_resource_heap.pop_back();

                unloadObject(min_loaded);
            }
            else if (!waiting_resource_heap.empty() && !loaded_resource_heap.empty()) {
                // They're equal, we're (potentially) exchanging
                Object* max_waiting = waiting_resource_heap.front();
                std::pop_heap(waiting_resource_heap.begin(), waiting_resource_heap.end(), Object::MaxHeapComparator());
                waiting_resource_heap.pop_back();

                Object* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Object::MinHeapComparator());
                loaded_resource_heap.pop_back();

                if (min_loaded->priority < max_waiting->priority) {
                    unloadObject(min_loaded);
                    loadObject(max_waiting);
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

void DistanceDownloadPlanner::stop() {
    for(AssetMap::iterator it = mAssets.begin(); it != mAssets.end(); it++) {
        Asset* asset = it->second;
        delete asset;
    }
}

void DistanceDownloadPlanner::requestAssetForObject(Object* forObject) {
    DLPLANNER_LOG(detailed, "Requesting " << forObject->file << " for " << forObject->name);

    if (forObject->file.empty()) {
        forObject->mesh->loadEmpty();
        return;
    }

    Asset* asset = NULL;
    // First make sure we have an Asset for this
    AssetMap::iterator asset_it = mAssets.find(forObject->file);
    if (asset_it == mAssets.end()) {
        asset = new Asset(forObject->file);
        mAssets.insert( AssetMap::value_type(forObject->file, asset) );
    }
    else {
        asset = asset_it->second;
    }

    // If we're already setup for this asset, just leave things alone
    // FIXME priorities...
    if (asset->waitingObjects.find(forObject->id()) != asset->waitingObjects.end()) return;

    asset->waitingObjects.insert(forObject->id());

    // Another Object might have already requested downloading
    // FIXME update priority
    if (!asset->downloadTask)
        downloadAsset(asset, forObject);
}

void DistanceDownloadPlanner::downloadAsset(Asset* asset, Object* forObject) {
    DLPLANNER_LOG(detailed, "Starting download of " << asset->uri);
    asset->downloadTask =
        AssetDownloadTask::construct(
            asset->uri, getScene(), forObject->priority,
            mContext->mainStrand->wrap(
                std::tr1::bind(&DistanceDownloadPlanner::loadAsset, this, asset->uri)
            ));
}

void DistanceDownloadPlanner::loadAsset(Transfer::URI asset_uri) {
    DLPLANNER_LOG(detailed, "Finished downloading " << asset_uri);

    if (mAssets.find(asset_uri) == mAssets.end()) return;
    Asset* asset = mAssets[asset_uri];

    DLPLANNER_LOG(detailed, "Loading asset " << asset->uri);

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
        return;
    }

    Mesh::BillboardPtr bbptr( std::tr1::dynamic_pointer_cast<Mesh::Billboard>(visptr) );
    if (bbptr) {
        loadBillboard(asset, bbptr, usingDefault);
        return;
    }

    // If we got here, we don't know how to load it
    SILOG(ogre, error, "Failed to load asset failed because it doesn't know how to load " << visptr->type());
    finishLoadAsset(asset, false);
    return;
}

void DistanceDownloadPlanner::finishLoadAsset(Asset* asset, bool success) {
    // We need to notify all Objects (objects) waiting for this to load that
    // it finished (or failed)
    for(ObjectSet::iterator it = asset->waitingObjects.begin(); it != asset->waitingObjects.end(); it++) {
        const String& resource_id = *it;

        DLPLANNER_LOG(detailed, "Using asset " << asset->uri << " for " << resource_id);

        // It may not even need it anymore if its not in the set of objects we
        // currently wnat loaded anymore (or not even exist anymore)
        ObjectMap::iterator rit = mObjects.find(resource_id);
        if (rit == mObjects.end()) continue;
        Object* resource = rit->second;
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
            }

            Mesh::BillboardPtr bbptr( std::tr1::dynamic_pointer_cast<Mesh::Billboard>(asset->downloadTask->asset()) );
            if (bbptr) {
                resource->mesh->loadBillboard(bbptr, asset->ogreAssetName);
            }
        }

        asset->usingObjects.insert(resource_id);
    }
    asset->downloadTask.reset();
    asset->waitingObjects.clear();

    checkRemoveAsset(asset);
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

    // Note that we don't check if the mesh exists here. Even if it does, we
    // still submit everything for loading so it can be ref counted by
    // ResourceLoader.

    loadDependentTextures(asset, usingDefault);

    if (!mdptr->instances.empty()) {
        int index=0;
        for (Mesh::MaterialEffectInfoList::const_iterator mat=mdptr->materials.begin(),mate=mdptr->materials.end();mat!=mate;++mat,++index) {
            std::string matname = ogreMaterialName(meshname, index);
            getScene()->getResourceLoader()->loadMaterial(
                matname, mdptr, *mat, asset->uri, asset->textureFingerprints,
                std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset)
            );
            asset->loadingResources++;
        }

        // Skeleton. Make sure this is submitted first so that the mesh loading
        // can find it by name.
        String skelname = "";
        if (!mdptr->joints.empty()) {
            skelname = ogreSkeletonName(meshname);
            getScene()->getResourceLoader()->loadSkeleton(
                skelname, mdptr, asset->animations,
                std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset)
            );
            asset->loadingResources++;
        }

        // Mesh
        getScene()->getResourceLoader()->loadMesh(
            meshname, mdptr, skelname,
            std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset)
        );
        asset->loadingResources++;
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
    std::string matname = ogreMaterialName(bbname, 0);
    asset->ogreAssetName = matname;
    getScene()->getResourceLoader()->loadBillboardMaterial(
        matname, bbptr->image, asset->uri, asset->textureFingerprints,
        std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset)
    );
    asset->loadingResources++;

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
                // Submit for loading. This would happen automatically, but
                // submitting it here blocks loading of the mesh before the
                // textures are all loaded and allows the textures to be loaded
                // across many frames
                getScene()->getResourceLoader()->loadTexture(
                    id,
                    std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset)
                );
                asset->loadingResources++;
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


void DistanceDownloadPlanner::handleLoadedResource(Asset* asset) {
    asset->loadingResources--;
    if (asset->loadingResources == 0) {
        finishLoadAsset(asset, true);
    }
}


void DistanceDownloadPlanner::unrequestAssetForObject(Object* forObject) {
    if (forObject->file.empty()) return;

    DLPLANNER_LOG(detailed, "Unrequesting " << forObject->file << " for " << forObject->name);

    assert(mAssets.find(forObject->file) != mAssets.end());
    Asset* asset = mAssets[forObject->file];

    // Make sure we're not displaying it anymore
    forObject->mesh->unload();

    // Clear the need for it
    ObjectSet::iterator rit;
    rit = asset->waitingObjects.find(forObject->name);
    if (rit != asset->waitingObjects.end())
        asset->waitingObjects.erase(rit);
    rit = asset->usingObjects.find(forObject->name);
    if (rit != asset->usingObjects.end())
        asset->usingObjects.erase(rit);

    // If nobody needs it anymore, clear it out.
    checkRemoveAsset(asset);
}

void DistanceDownloadPlanner::checkRemoveAsset(Asset* asset) {
    if (asset->waitingObjects.empty() && asset->usingObjects.empty()) {
        // We need to be careful if a download is in progress.
        if (asset->downloadTask) return;

        DLPLANNER_LOG(detailed, "Destroying unused asset " << asset->uri);

        mAssets.erase(asset->uri);
        delete asset;
    }
}

} // namespace Graphics
} // namespace Sirikata
