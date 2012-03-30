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

#include <sirikata/core/transfer/MaxPriorityAggregation.hpp>

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
    static uint64 assetId = 0;
    textureFingerprints = TextureBindingsMapPtr(new TextureBindingsMap());
    internalId = assetId++;
}

DistanceDownloadPlanner::Asset::~Asset() {
    for(WebMaterialList::iterator it = webMaterials.begin(); it != webMaterials.end(); it++)
        WebViewManager::getSingleton().destroyWebView(*it);
    webMaterials.clear();
    Liveness::letDie();
}

DistanceDownloadPlanner::DistanceDownloadPlanner(Context* c, OgreRenderer* renderer)
 : ResourceDownloadPlanner(c, renderer),
   mStopped(false)
{

    //listen for commands
    if (c->commander())
    {
        mContext->commander()->registerCommand(
            "oh.ogre.ddplanner",
            std::tr1::bind(&DistanceDownloadPlanner::commandGetData, this, _1, _2, _3)
        );
        mContext->commander()->registerCommand(
            "oh.ogre.ddplanner.stats",
            std::tr1::bind(&DistanceDownloadPlanner::commandGetStats, this, _1, _2, _3)
        );
    }


    mAggregationAlgorithm = new Transfer::MaxPriorityAggregation();

    mCDNArchive = CDNArchiveFactory::getSingleton().addArchive();
    mActiveCDNArchive = true;
}

DistanceDownloadPlanner::~DistanceDownloadPlanner()
{
    delete mAggregationAlgorithm;
    if (mActiveCDNArchive) {
        CDNArchiveFactory::getSingleton().removeArchive(mCDNArchive);
        mActiveCDNArchive=false;
    }
}

void DistanceDownloadPlanner::addObject(Object* r)
{
  RMutex::scoped_lock lock(mDlPlannerMutex);
  DLPLANNER_LOG(detailed,"Request to add object with name "<<r->name);

  iAddObject(r, livenessToken());
}

void DistanceDownloadPlanner::iAddObject(Object* r, Liveness::Token alive)
{
    if (!alive)
    {
        delete r;
        return;
    }

    RMutex::scoped_lock lock(mDlPlannerMutex);
    r->priority = calculatePriority(r->proxy);
    mObjects[r->name] = r;
    mWaitingObjects[r->name] = r;
    DLPLANNER_LOG(detailed, "Adding object " << r->name << " (" << r->file << "), " << mLoadedObjects.size() << " loaded, " << mWaitingObjects.size() << " waiting");
    checkShouldLoadNewObject(r);
}

DistanceDownloadPlanner::Object* DistanceDownloadPlanner::findObject(const String& name)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);
    ObjectMap::iterator it = mObjects.find(name);
    return (it != mObjects.end() ? it->second : NULL);
}


void DistanceDownloadPlanner::commandGetData(
    const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);
    Command::Result result = Command::EmptyResult();


    //run through all objects that are loaded
    Command::Object loadedObjects = Command::Object();
    for (ObjectMap::iterator omIter = mLoadedObjects.begin();
         omIter != mObjects.end(); ++omIter)
    {
        Command::Object individualObject = Command::Object();
        individualObject["priority"] = omIter->second->priority;
        individualObject["name"] = omIter->second->name;
        individualObject["loaded"] = omIter->second->loaded;
        //append known object to map of results
        String index (omIter->first);
        loadedObjects[index.c_str()] = individualObject;
    }
    result.put("ddplanner.loaded_objects",loadedObjects);


    //run through all waiting objects
    Command::Object waitingObjects = Command::Object();
    for (ObjectMap::iterator omIter = mWaitingObjects.begin();
         omIter != mObjects.end(); ++omIter)
    {
        Command::Object individualObject = Command::Object();
        individualObject["priority"] = omIter->second->priority;
        individualObject["name"] = omIter->second->name;
        individualObject["loaded"] = omIter->second->loaded;
        //append known object to map of results
        String index (omIter->first);
        waitingObjects[index.c_str()] = individualObject;
    }
    result.put("ddplanner.waiting_objects",waitingObjects);


    //all loaded assets
    Command::Object assets = Command::Object();
    for (AssetMap::iterator assetMIter = mAssets.begin();
         assetMIter != mAssets.end(); ++assetMIter)
    {
        Command::Object individualAsset = Command::Object();
        individualAsset["name"]= assetMIter->second->uri.toString();

        //add waitingObjects
        Command::Object waitingObjects = Command::Object();
        for(ObjectSet::iterator osIter = assetMIter->second->waitingObjects.begin();
            osIter != assetMIter->second->waitingObjects.end(); ++osIter)
        {
            waitingObjects[osIter->c_str()] = *osIter;
        }
        individualAsset["waitingObjects"] = waitingObjects;

        //add usingObjects
        Command::Object usingObjects = Command::Object();
        for(ObjectSet::iterator osIter = assetMIter->second->usingObjects.begin();
            osIter != assetMIter->second->usingObjects.end(); ++osIter)
        {
            usingObjects[osIter->c_str()] = *osIter;
        }
        individualAsset["usingObjects"] = usingObjects;


        //list of loaded and loading resources, by uri
        std::vector<String> finishedDownloads;
        std::vector<String> activeDownloads;

        Command::Array loadingResources = Command::Array();
        Command::Array loadedResources = Command::Array();
        //priority
        double priority = -1;
        AssetDownloadTask::ActiveDownloadMap::size_type stillToDownload = 0;
        //perform different operations depending on whether still downloading or not.
        if (assetMIter->second->downloadTask)
        {
            priority = assetMIter->second->downloadTask->priority();
            stillToDownload = assetMIter->second->downloadTask->getOutstandingDependentDownloads();

            assetMIter->second->downloadTask->getDownloadTasks(finishedDownloads,activeDownloads);
        }
        else
        {
            //use the ogre-ified version of loaded resources
            finishedDownloads = assetMIter->second->loadedResources;
        }
        individualAsset["priority"] = priority;

        //finish downloaded resources
        for (std::vector<String>::iterator strIt = finishedDownloads.begin();
             strIt != finishedDownloads.end(); ++strIt)
        {
            loadedResources.push_back(*strIt);
        }
        individualAsset["loadedResources"] = loadedResources;


        //finish still-fetching resources
        for (std::vector<String>::iterator strIt = activeDownloads.begin();
             strIt != activeDownloads.end(); ++strIt)
        {
            loadingResources.push_back(*strIt);
        }
        individualAsset["loadingResources"] = loadingResources;


        //how many dependent resources does this asset still have to download
        individualAsset["stillToDownload"] = stillToDownload;


        //append known object to map of results
        String index (assetMIter->first.toString());
        assets[index] = individualAsset;
    }
    result.put("ddplanner.assets",assets);

    //actually
    cmdr->result(cmdid, result);
}


void DistanceDownloadPlanner::commandGetStats(
    const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);
    Command::Result result = Command::EmptyResult();

    // We don't keep fine-grained stats about the state of
    // objects/assets at the top-level and some of the names can be a
    // bit misleading. For example, mLoadedObjects refers to objects
    // which we've decided to load, and we don't even keep a flag for
    // whether the object was finally loaded. We need to scan through
    // the objects and assets to actually figure these properties out
    // and get accurate stats.
    result.put("ddplanner.objects.total", mObjects.size());
    result.put("ddplanner.objects.unloaded", mWaitingObjects.size());
    uint32 objects_loading = 0, objects_loaded = 0;
    for(ObjectMap::const_iterator objit = mLoadedObjects.begin(); objit != mLoadedObjects.end(); objit++) {
        if (objit->second->file.empty()) {
            objects_loaded++;
            continue;
        }
        assert(mAssets.find(objit->second->file) != mAssets.end());
        Asset* obj_asset = mAssets[objit->second->file];
        if (obj_asset->usingObjects.find(objit->second->id()) != obj_asset->usingObjects.end())
            objects_loaded++;
        else
            objects_loading++;
    }
    result.put("ddplanner.objects.loading", objects_loading);
    result.put("ddplanner.objects.loaded", objects_loaded);


    result.put("ddplanner.assets.total", mAssets.size());
    uint32 assets_loading = 0, assets_loaded = 0;
    for(AssetMap::const_iterator assit = mAssets.begin(); assit != mAssets.end(); assit++) {
        if (!assit->second->usingObjects.empty())
            assets_loaded++;
        else
            assets_loading++;
    }
    result.put("ddplanner.assets.loading", assets_loading);
    result.put("ddplanner.assets.loaded", assets_loaded);


    cmdr->result(cmdid, result);
}





void DistanceDownloadPlanner::removeObject(const String& name)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);
    DLPLANNER_LOG(detailed,"Request to remove object with name "<<name);

    iRemoveObject(name,livenessToken());
}

void DistanceDownloadPlanner::iRemoveObject(
    const String& name, Liveness::Token alive)
{
    if (!alive || mStopped)
        return;
    RMutex::scoped_lock lock(mDlPlannerMutex);
    ObjectMap::iterator it = mObjects.find(name);
    if (it != mObjects.end()) {
        Object* r = it->second;

        // Make sure we've unloaded it
        if (r->loaded)
            unloadObject(r);

        // It should definitely be in waiting objects now
        ObjectMap::iterator waiting_it = mWaitingObjects.find(name);
        if (waiting_it != mWaitingObjects.end()) mWaitingObjects.erase(waiting_it);

        // Log and cleanup
        DLPLANNER_LOG(detailed, "Removing object " << r->name << " (" << r->file << "), " << mLoadedObjects.size() << " loaded, " << mWaitingObjects.size() << " waiting");
        mObjects.erase(it);
        delete r;
    }
}

void DistanceDownloadPlanner::addNewObject(Entity *ent, const Transfer::URI& mesh) {
    addObject(new Object(ent, mesh));
}

void DistanceDownloadPlanner::addNewObject(ProxyObjectPtr p, Entity *mesh) {
    addObject(new Object(mesh, p->mesh(), p));
}


void DistanceDownloadPlanner::updateObject(ProxyObjectPtr p)
{
  RMutex::scoped_lock lock(mDlPlannerMutex);

  DLPLANNER_LOG(detailed,"Request to update object with name "<<        \
        p->getObjectReference().toString());

  iUpdateObject(p,livenessToken());
}


void DistanceDownloadPlanner::iUpdateObject(
    ProxyObjectPtr p, Liveness::Token alive)
{
    if (!alive)
        return;

    Object* r = findObject(p->getObjectReference().toString());
    if (!r)
    {
        DLPLANNER_LOG(warn,"Could not find object associated with "   <<\
            "reference "<<p->getObjectReference().toString()<< " in " <<\
            "distance download planner.  Not updating.");
        return;
    }
    URI last_file = r->file;
    URI new_file = p->mesh();
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
    Vector3d objLoc(proxy->location().position());
    Vector3d diff = cameraLoc - objLoc;

    /*double diff2d = sqrt(pow(diff.x, 2) + pow(diff.y, 2));
      double diff3d = sqrt(pow(diff2d, 2) + pow(diff.x, 2));*/

    if (diff.length() <= 0) return 1.0;

    double priority = 1/(diff.length());

    return priority;
}

void DistanceDownloadPlanner::checkShouldLoadNewObject(Object* r) {
    if ((int32)mLoadedObjects.size() < mMaxLoaded) {
        DLPLANNER_LOG(detailed, "Loading " << r->name << " immediately because we are under budget");
        loadObject(r);
    }
}

bool DistanceDownloadPlanner::budgetRequiresChange() const {
    return
        ((int32)mLoadedObjects.size() < mMaxLoaded && !mWaitingObjects.empty()) ||
        ((int32)mLoadedObjects.size() > mMaxLoaded && !mLoadedObjects.empty());

}

void DistanceDownloadPlanner::loadObject(Object* r) {
    RMutex::scoped_lock lock(mDlPlannerMutex);
    mWaitingObjects.erase(r->name);
    mLoadedObjects[r->name] = r;

    // After operation to get updated stats
    DLPLANNER_LOG(detailed, "Loading object " << r->name << " (" << r->file << "), " << mLoadedObjects.size() << " loaded, " << mWaitingObjects.size() << " waiting");

    r->loaded = true;
    requestAssetForObject(r);
}

void DistanceDownloadPlanner::unloadObject(Object* r) {
    RMutex::scoped_lock lock(mDlPlannerMutex);
    mLoadedObjects.erase(r->name);
    mWaitingObjects[r->name] = r;

    // After operation to get updated stats
    DLPLANNER_LOG(detailed, "Unloading object " << r->name << " (" << r->file << "), " << mLoadedObjects.size() << " loaded, " << mWaitingObjects.size() << " waiting");

    r->loaded = false;
    unrequestAssetForObject(r);
}

void DistanceDownloadPlanner::poll()
{
  RMutex::scoped_lock lock(mDlPlannerMutex);

  iPoll(livenessToken());
}

void DistanceDownloadPlanner::iPoll(Liveness::Token dpAlive)
{
    if (!dpAlive)
        return;

    if (camera == NULL) return;

    if (mContext->stopped()) return;

    RMutex::scoped_lock lock(mDlPlannerMutex);
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

                DLPLANNER_LOG(detailed, "Adding object " << max_waiting->name);
                loadObject(max_waiting);
            }
            else if ((int32)mLoadedObjects.size() > mMaxLoaded && !loaded_resource_heap.empty()) {
                // Otherwise, extract the min and check if we can continue
                Object* min_loaded = loaded_resource_heap.front();
                std::pop_heap(loaded_resource_heap.begin(), loaded_resource_heap.end(), Object::MinHeapComparator());
                loaded_resource_heap.pop_back();

                DLPLANNER_LOG(detailed, "Removing object " << min_loaded->name);
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
                    DLPLANNER_LOG(detailed, "Swapping object " << max_waiting->name << " for " << min_loaded->name);
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



    // Finally, now that we've settled on the set of Objects that are being
    // loaded, update the per-Asset priorities for currently loading assets
    for(AssetMap::iterator it = mAssets.begin(); it != mAssets.end(); it++) {
        Asset* asset = it->second;
        updateAssetPriority(asset);
    }
}

void DistanceDownloadPlanner::stop()
{
  RMutex::scoped_lock lock(mDlPlannerMutex);

  iStop(livenessToken());
}

void DistanceDownloadPlanner::iStop(Liveness::Token dpAlive)
{
    if (!dpAlive)
    {
        DLPLANNER_LOG(error,"Posted to internal stop after was already deleted");
        return;
    }

    mStopped = true;

    RMutex::scoped_lock lock(mDlPlannerMutex);
    for(AssetMap::iterator it = mAssets.begin(); it != mAssets.end(); it++)
    {
        Asset* asset = it->second;
        delete asset;
    }
    mAssets.clear();

    for(ObjectMap::iterator it = mObjects.begin(); it != mObjects.end(); it++)
    {
        Object* obj = it->second;
        delete obj;
    }
    mObjects.clear();
    mLoadedObjects.clear();
    mWaitingObjects.clear();
}

void DistanceDownloadPlanner::requestAssetForObject(Object* forObject) {
    RMutex::scoped_lock lock(mDlPlannerMutex);
    DLPLANNER_LOG(detailed, "Requesting " << forObject->file << " for " << forObject->name);
    if (forObject->file.empty()) {
        DLPLANNER_LOG(detailed,"Empty file for requested asset object");
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


    assert(asset->waitingObjects.find(forObject->id()) == asset->waitingObjects.end());
    asset->waitingObjects.insert(forObject->id());


    // The asset could already be in a variety of states:
    if (!asset->usingObjects.empty()) {
        // Objects are already using it, meaning it's already
        // completely done loading. We just need to trigger another
        // round of moving waiting objects -> using objects by using
        // the mesh
        finishLoadAsset(asset, true);
    }
    else if (asset->loadingResources > 0) {
        // We have some resources still loading, meaning we've already
        // downloaded the data but are still working on loading
        // it. We're in the waiting queue, so we'll get loaded with
        // any other objects. Nothing to do.
    }
    else if (asset->downloadTask) {
        // The download is already running. We're already tacked onto
        // the waiting list, but we can update the priority to account
        // for us.
        updateAssetPriority(asset);
    }
    else {
        // No download at all yet. Star the whole process.
        downloadAsset(asset, forObject);
    }
}

void DistanceDownloadPlanner::downloadAsset(Asset* asset, Object* forObject) {
    RMutex::scoped_lock lock(mDlPlannerMutex);
    DLPLANNER_LOG(detailed, "Starting download of " << asset->uri);

    bool is_aggregate = (forObject->proxy ? forObject->proxy->isAggregate() : false);
    asset->downloadTask =
        AssetDownloadTask::construct(
            asset->uri, getScene(), forObject->priority,
            is_aggregate,
            // We need some indirection because this callback is invoked
            // with a lock on in AssetDownload task and triggers
            // calls which need that lock. A post to a service would
            // be better, posting to a strand works ok
            mScene->renderStrand()->wrap(
                std::tr1::bind(&DistanceDownloadPlanner::loadAsset, this, asset->uri,asset->internalId)
            )
        );
}


void DistanceDownloadPlanner::loadAsset(Transfer::URI asset_uri,uint64 assetId)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);
    DLPLANNER_LOG(detailed, "Finished downloading " << asset_uri);

    Asset* asset = NULL;

    if (mAssets.find(asset_uri) == mAssets.end()) return;

    asset = mAssets[asset_uri];


    //in certain cases, may have an asset download a mesh, have that asset issue
    //a callback to DDPlanner::loadAsset, which gets stuck trying to acquire
    //lock.  In the meantime, the asset gets deleted, and then a new asset is
    //inserted to replace it.  This new asset does not have valid internal data
    //(asset->downloadTask->asset() may point to garbage), so that if the
    //callback issued then acquires the lock and proceeds, we'll run into problems.
    //As a result, each asset also maintains a 64-bit internal id, which (minus
    //wraparound) is unique to each asset.  Using the internal id, we can check
    //whether the callback is for the asset that we currently have stored in our
    //asset map.
    if (asset->internalId != assetId)
        return;


    DLPLANNER_LOG(detailed, "Loading asset " << asset->uri);
    assert(asset->usingObjects.empty());
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
    RMutex::scoped_lock lock(mDlPlannerMutex);
    if (asset->downloadTask) {
        DLPLANNER_LOG(detailed, "Finishing load of asset " << asset->uri << " (priority " << asset->downloadTask->priority() << ")");
        asset->visual = asset->downloadTask->asset();
    }
    else {
        DLPLANNER_LOG(detailed, "Finishing load of asset " << asset->uri << " (repeat)");
        assert(asset->visual);
    }
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
            Mesh::MeshdataPtr mdptr( std::tr1::dynamic_pointer_cast<Mesh::Meshdata>(asset->visual) );
            if (mdptr) {
                resource->mesh->loadMesh(mdptr, asset->ogreAssetName, asset->animations);
            }

            Mesh::BillboardPtr bbptr( std::tr1::dynamic_pointer_cast<Mesh::Billboard>(asset->visual) );
            if (bbptr) {
                resource->mesh->loadBillboard(bbptr, asset->ogreAssetName);
            }
        }

        asset->usingObjects.insert(resource_id);
    }
    asset->downloadTask.reset();
    asset->waitingObjects.clear();
    checkRemoveAsset(asset,asset->livenessToken());
}

void DistanceDownloadPlanner::loadMeshdata(Asset* asset, const Mesh::MeshdataPtr& mdptr, bool usingDefault) {
    RMutex::scoped_lock lock(mDlPlannerMutex);

    //Extract any animations from the new mesh.
    for (uint32 i=0;  i < mdptr->nodes.size(); i++) {
      Sirikata::Mesh::Node& node = mdptr->nodes[i];

      for (std::map<String, Sirikata::Mesh::TransformationKeyFrames>::iterator it = node.animations.begin();
           it != node.animations.end(); it++)
        {
          asset->animations.insert(it->first);
        }
    }

    // Note that we don't check if the mesh exists here. Even if it does, we
    // still submit everything for loading so it can be ref counted by
    // ResourceLoader.

    // We load textures first to setup the texture bindings map, used when
    // computing the mesh name.
    loadDependentTextures(asset, usingDefault);

    SHA256 sha = mdptr->hash;
    String hash = sha.convertToHexString();
    String meshname = ogreVisualName(mdptr, asset->textureFingerprints);
    asset->ogreAssetName = meshname;

    if (!mdptr->instances.empty()) {
        int index=0;
        for (Mesh::MaterialEffectInfoList::const_iterator mat=mdptr->materials.begin(),mate=mdptr->materials.end();mat!=mate;++mat,++index) {

            std::string matname = ogreMaterialName(*mat, asset->uri, asset->textureFingerprints);
            getScene()->getResourceLoader()->loadMaterial(
                matname, mdptr, *mat, asset->uri, asset->textureFingerprints,
                std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset,asset->livenessToken())
            );
            asset->loadedResources.push_back(matname);
            asset->loadingResources++;
        }



        // Skeleton. Make sure this is submitted first so that the mesh loading
        // can find it by name.
        String skelname = "";
        if (!mdptr->joints.empty()) {
            skelname = ogreSkeletonName(mdptr, asset->textureFingerprints);
            getScene()->getResourceLoader()->loadSkeleton(
                skelname, mdptr, asset->animations,
                std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset,asset->livenessToken())
            );
            asset->loadedResources.push_back(skelname);
            asset->loadingResources++;
        }

        // Mesh
        getScene()->getResourceLoader()->loadMesh(
            meshname, mdptr, skelname, asset->textureFingerprints,
            std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset,asset->livenessToken())
        );
        asset->loadedResources.push_back(meshname);
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
    RMutex::scoped_lock lock(mDlPlannerMutex);

    // We load textures first to setup the texture bindings map, used when
    // computing the mesh name.
    loadDependentTextures(asset, usingDefault);

    // Then we can compute names
    SHA256 sha = bbptr->hash;
    String hash = sha.convertToHexString();
    String bbname = ogreVisualName(bbptr, asset->textureFingerprints);

    // Load the single material
    std::string matname = ogreBillboardMaterialName(asset->textureFingerprints);
    asset->ogreAssetName = matname;
    getScene()->getResourceLoader()->loadBillboardMaterial(
        matname, bbptr->image, asset->uri, asset->textureFingerprints,
        std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset,asset->livenessToken())
    );
    asset->loadedResources.push_back(matname);
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
    RMutex::scoped_lock lock(mDlPlannerMutex);
    // we currently assume no dependencies for default
    if (usingDefault) return;

    for(AssetDownloadTask::Dependencies::const_iterator tex_it = asset->downloadTask->dependencies().begin();
        tex_it != asset->downloadTask->dependencies().end();
        tex_it++)
    {
        const Transfer::URI& uri = tex_it->first;
        const String& uri_str = uri.toString();
        const AssetDownloadTask::ResourceData& tex_data = tex_it->second;

        if (mActiveCDNArchive && asset->textureFingerprints->find(uri_str) == asset->textureFingerprints->end() ) {
            String id = tex_data.request->getIdentifier();

            //OGRE_LOG(warn, "Got asset ID of " << id);

            fixOgreURI(id);

            (*asset->textureFingerprints)[uri_str] = id;

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
                    std::tr1::bind(&DistanceDownloadPlanner::handleLoadedResource, this, asset,
                        asset->livenessToken())
                );
                asset->loadedResources.push_back(id);
                asset->loadingResources++;
            }
            else if (uri.scheme() == "http") {
                // Or, if its an http URL, we can try displaying it in a webview
                WebView* web_mat = WebViewManager::getSingleton().createWebViewMaterial(
                    mContext,
                    id,
                    512, 512, // Completely arbitrary...
                    mScene->renderStrand()
                );
                web_mat->loadURL(uri_str);
                asset->webMaterials.push_back(web_mat);
            }
        }
    }
}


void DistanceDownloadPlanner::handleLoadedResource(Asset* asset,Liveness::Token assetAlive)
{
    if (!assetAlive)
        return;

    RMutex::scoped_lock lock(mDlPlannerMutex);

    asset->loadingResources--;
    if (asset->loadingResources == 0) {
        finishLoadAsset(asset, true);
    }
}


void DistanceDownloadPlanner::updateAssetPriority(Asset* asset) {
    RMutex::scoped_lock lock(mDlPlannerMutex);
    // We only care about updating priorities when we're still downloading
    // the asset.
    if (!asset->downloadTask) return;

    std::vector<Priority> priorities;
    for(ObjectSet::iterator obj_it = asset->waitingObjects.begin(); obj_it != asset->waitingObjects.end(); obj_it++) {
        String objid = *obj_it;
        assert(mObjects.find(objid) != mObjects.end());
        Object* obj = mObjects[objid];

        priorities.push_back(obj->priority);
    }
    if (!priorities.empty())
        asset->downloadTask->updatePriority( mAggregationAlgorithm->aggregate(priorities) );
}

void DistanceDownloadPlanner::unrequestAssetForObject(Object* forObject) {
    //use the scoped mutex up here so that don't invalidate waiting objects
    //while running.
    RMutex::scoped_lock lock(mDlPlannerMutex);
    if (forObject->file.empty()) return;

    DLPLANNER_LOG(detailed, "Unrequesting " << forObject->file << " for " << forObject->name);

    Asset* asset = NULL;
    assert(mAssets.find(forObject->file) != mAssets.end());
    asset = mAssets[forObject->file];



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

    // If somebody is still using it, update priority
    updateAssetPriority(asset);
    // If nobody needs it anymore, clear it out.

    checkRemoveAsset(asset,asset->livenessToken());
}

void DistanceDownloadPlanner::checkRemoveAsset(Asset* asset,Liveness::Token assetAlive)
{
    RMutex::scoped_lock lock(mDlPlannerMutex);

    //means that the asset was likely already deleted
    if (mStopped)
        return;

    if (!assetAlive)
        return;

    if (asset->waitingObjects.empty() && asset->usingObjects.empty()) {

        // We need to be careful if a download is in progress.
        if (asset->downloadTask)
            asset->downloadTask->cancel();


        DLPLANNER_LOG(detailed, "Destroying unused asset " << asset->uri);

        // Tell ResourceLoader we no longer need the data. Go
        // backwards so that dependencies get resolved properly.
        for(Asset::ResourceNameList::reverse_iterator rit = asset->loadedResources.rbegin();
            rit != asset->loadedResources.rend(); rit++) {
            getScene()->getResourceLoader()->unloadResource(*rit);
        }

        // And really erase it
        if (mAssets.find(asset->uri) == mAssets.end())
            assert(false);

        mAssets.erase(asset->uri);
        delete asset;
    }
}

} // namespace Graphics
} // namespace Sirikata
