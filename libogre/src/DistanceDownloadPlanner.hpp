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
#include <sirikata/core/util/Liveness.hpp>
#include <sirikata/core/command/Commander.hpp>

namespace Sirikata {
namespace Graphics {

class WebView;

class DistanceDownloadPlanner : public ResourceDownloadPlanner,
                                public virtual Liveness
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
    bool mStopped;
    struct Object;

    void iUpdateObject(ProxyObjectPtr p,Liveness::Token lt);
    void iRemoveObject(const String& name, Liveness::Token alive);
    void iAddObject(Object* r, Liveness::Token alive);
    void addObject(Object* r);
    Object* findObject(const String& sporef);
    void removeObject(const String& sporef);

    virtual double calculatePriority(ProxyObjectPtr proxy);

    void commandGetData(
        const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);
    void commandGetStats(
        const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);

    void checkShouldLoadNewObject(Object* r);

    // Checks if changes just due to budgets are possible,
    // e.g. regardless of priorities, we have waiting objects and free
    // spots for them.
    bool budgetRequiresChange() const;

    void loadObject(Object* r);
    void unloadObject(Object* r);

    struct Object {
        Object(Graphics::Entity *m, const Transfer::URI& mesh_uri, ProxyObjectPtr _proxy = ProxyObjectPtr());
        virtual ~Object(){
        }

        const String& id() const { return name; }

        Transfer::URI file;
        Graphics::Entity *mesh;
        String name;
        bool loaded;
        float32 priority;
        ProxyObjectPtr proxy;

        class Hasher {
        public:
            size_t operator() (const Object& r) const {
                return std::tr1::hash<String>()(r.name);
            }
        };

        struct MaxHeapComparator {
            bool operator()(Object* lhs, Object* rhs) {
                return lhs->priority < rhs->priority;
            }
        };
        struct MinHeapComparator {
            bool operator()(Object* lhs, Object* rhs) {
                return lhs->priority > rhs->priority;
            }
        };

    };

    typedef std::tr1::unordered_set<String> ObjectSet;
    typedef std::tr1::unordered_map<String, Object*> ObjectMap;
    // The full list
    ObjectMap mObjects;
    // Loading has started for these
    ObjectMap mLoadedObjects;
    // Waiting to be important enough to load
    ObjectMap mWaitingObjects;


    // Heap storage for Objects. Choice between min/max heap is at call time.
    typedef std::vector<Object*> ObjectHeap;


    typedef std::vector<WebView*> WebMaterialList;

    // Assets represent a single graphical asset that needs to be downloaded
    // from the CDN and loaded into memory. Since a single asset can be loaded
    // many times by different 'Objects' (i.e. objects in the world) we track
    // them separately and make sure we only issue single requests for them.
    struct Asset : public Liveness
    {
        Transfer::URI uri;
        AssetDownloadTaskPtr downloadTask;
        // Objects that want this asset to be loaded and are waiting for it
        ObjectSet waitingObjects;
        // Objects that are using this asset
        ObjectSet usingObjects;
        // Filled in by the loader with the name of the asset that's actually
        // used when creating an instance (unique name for mesh, billboard
        // texture, etc).
        String ogreAssetName;


        //Can get into a situation where we fire a callback associated with a
        //transfer uri, then we delete the associated asset, then create a new
        //asset with the same uri.  The callback assumes that internal state
        //for the asset is valid and correct (in particular, downloadTask).
        //However, in these cases, the data wouldn't be valid.  Use internalId
        //to check that the callback that is being serviced corresponds to the
        //correct asset that we have in memory.
        uint64 internalId;

        TextureBindingsMapPtr textureFingerprints;
        std::set<String> animations;

        WebMaterialList webMaterials;

        // # of resources we're still waiting to finish loading
        uint16 loadingResources;
        // Sets of resources this Asset has loaded so we can get
        // ResourceLoader to unload them. Ordered list so we can
        // unload in reverse order we loaded in.
        typedef std::vector<String > ResourceNameList;
        ResourceNameList loadedResources;

        // Store a copy so we can release the download task but still
        // get at the data if another object uses this mesh.
        Mesh::VisualPtr visual;

        Asset(const Transfer::URI& name);
        ~Asset();
    };
    typedef std::tr1::unordered_map<Transfer::URI, Asset*, Transfer::URI::Hasher> AssetMap;


    AssetMap mAssets;


    // Because we aggregate all Asset requests so we only generate one
    // AssetDownloadTask, we need to aggregate some priorities
    // ourselves. Aggregation will still also be performed by other parts of the
    // system on a per-Resource basis (TransferPool for multiple requests by
    // different Assets, TransferMediator for requests across multiple Pools).
    Transfer::PriorityAggregationAlgorithm* mAggregationAlgorithm;

    // These are a sequence of async operations that take a URI for a
    // resource/asset pair and gets it loaded. Some paths will terminate early
    // since multiple resources that share an asset can share many of these
    // steps.
    void requestAssetForObject(Object*);
    void downloadAsset(Asset* asset, Object* forObject);
    void loadAsset(Transfer::URI asset_uri,uint64 assetId);
    void finishLoadAsset(Asset* asset, bool success);

    void loadMeshdata(Asset* asset, const Mesh::MeshdataPtr& mdptr, bool usingDefault);
    void loadBillboard(Asset* asset, const Mesh::BillboardPtr& bbptr, bool usingDefault);
    void loadDependentTextures(Asset* asset, bool usingDefault);

    // Helper, notifies when resource has finished loading allowing us
    // to figure out when the entire asset has loaded
    void handleLoadedResource(Asset* asset,Liveness::Token assetAlive);

    // Update the priority for an asset from all it's requestors
    void updateAssetPriority(Asset* asset);

    // Removes the resource's need for the asset, potentially allowing it to be
    // unloaded.
    void unrequestAssetForObject(Object*);

    // Helper to check if it's safe to remove an asset and does so if
    // possible. Properly handles current
    void checkRemoveAsset(Asset* asset,Liveness::Token lt);

    bool mActiveCDNArchive;
    unsigned int mCDNArchive;

    void iStop(Liveness::Token dpAlive);
    void iPoll(Liveness::Token dpAlive);
};

} // namespace Graphics
} // namespace Sirikata

#endif
