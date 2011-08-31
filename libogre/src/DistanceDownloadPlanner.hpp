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

#ifndef _DISTANCE_DOWNLOAD_PLANNER_HPP
#define _DISTANCE_DOWNLOAD_PLANNER_HPP

#include <sirikata/ogre/ResourceDownloadPlanner.hpp>
#include <sirikata/ogre/resourceManager/AssetDownloadTask.hpp>
#include <sirikata/ogre/Util.hpp>
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/Billboard.hpp>

namespace Sirikata {
namespace Graphics {

class WebView;

class DistanceDownloadPlanner : public ResourceDownloadPlanner
{
public:
    DistanceDownloadPlanner(Context* c, OgreRenderer* renderer);
    ~DistanceDownloadPlanner();

    virtual void addNewObject(Graphics::Entity *ent, const Transfer::URI& mesh);
    virtual void addNewObject(ProxyObjectPtr p, Graphics::Entity *mesh);
    virtual void updateObject(ProxyObjectPtr p);
    virtual void removeObject(ProxyObjectPtr p);
    virtual void removeObject(Graphics::Entity* ent);

    //PollingService interface
    virtual void poll();
    virtual void stop();

protected:
    struct Resource;

    void addResource(Resource* r);
    Resource* findResource(const String& sporef);
    void removeResource(const String& sporef);

    virtual double calculatePriority(ProxyObjectPtr proxy);

    void checkShouldLoadNewResource(Resource* r);

    // Checks if changes just due to budgets are possible,
    // e.g. regardless of priorities, we have waiting objects and free
    // spots for them.
    bool budgetRequiresChange() const;

    void loadResource(Resource* r);
    void unloadResource(Resource* r);

    struct Resource {
        Resource(Graphics::Entity *m, const Transfer::URI& mesh_uri, ProxyObjectPtr _proxy = ProxyObjectPtr());
        virtual ~Resource(){}

        const String& id() const { return name; }

        Transfer::URI file;
        Graphics::Entity *mesh;
        String name;
        bool loaded;
        float32 priority;
        ProxyObjectPtr proxy;

        class Hasher {
        public:
            size_t operator() (const Resource& r) const {
                return std::tr1::hash<String>()(r.name);
            }
        };

        struct MaxHeapComparator {
            bool operator()(Resource* lhs, Resource* rhs) {
                return lhs->priority < rhs->priority;
            }
        };
        struct MinHeapComparator {
            bool operator()(Resource* lhs, Resource* rhs) {
                return lhs->priority > rhs->priority;
            }
        };

    };

    typedef std::tr1::unordered_set<String> ResourceSet;
    typedef std::tr1::unordered_map<String, Resource*> ResourceMap;
    // The full list
    ResourceMap mResources;
    // Loading has started for these
    ResourceMap mLoadedResources;
    // Waiting to be important enough to load
    ResourceMap mWaitingResources;

    // Heap storage for Resources. Choice between min/max heap is at call time.
    typedef std::vector<Resource*> ResourceHeap;


    typedef std::vector<WebView*> WebMaterialList;

    // Assets represent a single graphical asset that needs to be downloaded
    // from the CDN and loaded into memory. Since a single asset can be loaded
    // many times by different 'Resources' (i.e. objects in the world) we track
    // them separately and make sure we only issue single requests for them.
    struct Asset {
        Transfer::URI uri;
        AssetDownloadTaskPtr downloadTask;
        // Resources that want this asset to be loaded and are waiting for it
        ResourceSet waitingResources;
        // Resources that are using this asset
        ResourceSet usingResources;
        // Filled in by the loader with the name of the asset that's actually
        // used when creating an instance (unique name for mesh, billboard
        // texture, etc).
        String ogreAssetName;

        TextureBindingsMapPtr textureFingerprints;
        std::set<String> animations;

        WebMaterialList webMaterials;

        Asset(const Transfer::URI& name);
        ~Asset();
    };
    typedef std::tr1::unordered_map<Transfer::URI, Asset*, Transfer::URI::Hasher> AssetMap;
    AssetMap mAssets;

    // These are a sequence of async operations that take a URI for a
    // resource/asset pair and gets it loaded. Some paths will terminate early
    // since multiple resources that share an asset can share many of these
    // steps.
    void requestAssetForResource(Resource*);
    void downloadAsset(Asset* asset, Resource* forResource);
    void loadAsset(Transfer::URI asset_uri);
    void finishLoadAsset(Asset* asset, bool success);

    void loadMeshdata(Asset* asset, const Mesh::MeshdataPtr& mdptr, bool usingDefault);
    void loadBillboard(Asset* asset, const Mesh::BillboardPtr& bbptr, bool usingDefault);
    void loadDependentTextures(Asset* asset, bool usingDefault);

    // Removes the resource's need for the asset, potentially allowing it to be
    // unloaded.
    void unrequestAssetForResource(Resource*);

    // Helper to check if it's safe to remove an asset and does so if
    // possible. Properly handles current
    void checkRemoveAsset(Asset* asset);

    bool mActiveCDNArchive;
    unsigned int mCDNArchive;
};

} // namespace Graphics
} // namespace Sirikata

#endif
