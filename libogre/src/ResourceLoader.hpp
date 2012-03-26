// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_RESOURCE_LOADER_HPP_
#define _SIRIKATA_OGRE_RESOURCE_LOADER_HPP_

#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/core/transfer/URI.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/service/Context.hpp>
#include "ManualMaterialLoader.hpp"

namespace Sirikata {
namespace Graphics {

/** ResourceLoader manages the loading of resources into Ogre. It works
 *  asynchronously and handles scheduling loading of resources so they don't
 *  consume too much time, thereby attempting to maintain good frame
 *  rates. Since it coordinates all loading of resources for a single Ogre
 *  instance, it also makes sure each resources is loaded only once by
 *  deduplicating requests (some of which can't be known to be duplicates until
 *  we reach the point of loading, e.g. because two meshes use the same
 *  texture).
 *
 *  ResourceLoader also keeps track of usage so it can *unload* resources when
 *  they are no longer required.  This means that even if you know Ogre has
 *  loaded a resource, you should request it be loaded and unloaded at the
 *  appropriate times.
 *
 *  The ResourceLoader is created and driven by OgreRenderer. You shouldn't
 *  instantiate this class yourself.
 */
class ResourceLoader {
public:
    typedef std::tr1::function<void()> LoadedCallback;

    /** Create a ResourceLoader.
     *  \param per_frame_time maximum amount of time to spend loading
     *  resources. This isn't a guarantee, rather a best
     *  effort. Single resource loading tasks that exceed this time
     *  can't be avoided.
     */
    ResourceLoader(Context* ctx, const Duration& per_frame_time);
    ~ResourceLoader();

    void loadMaterial(const String& name, Mesh::MeshdataPtr mesh, const Mesh::MaterialEffectInfo& mat, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);
    void loadBillboardMaterial(const String& name, const String& texuri, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadSkeleton(const String& name, Mesh::MeshdataPtr mesh, const std::set<String>& animationList, LoadedCallback cb);

    void loadMesh(const String& name, Mesh::MeshdataPtr mesh, const String& skeletonName, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadTexture(const String& name, LoadedCallback cb);

    void unloadResource(const String& name);

    void tick();

private:
    enum ResourceType {
        ResourceTypeMaterial,
        ResourceTypeTexture,
        ResourceTypeSkeleton,
        ResourceTypeMesh
    };

    void loadMaterialWork(const String& name, Mesh::MeshdataPtr mesh, const Mesh::MaterialEffectInfo& mat, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadBillboardMaterialWork(const String& name, const String& texuri, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadSkeletonWork(const String& name, Mesh::MeshdataPtr mesh, const std::set<String>& animationList, LoadedCallback cb);

    void loadMeshWork(const String& name, Mesh::MeshdataPtr mesh, const String& skeletonName, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadTextureWork(const String& name, LoadedCallback cb);

    void unloadResourceWork(const String& name, ResourceType type);

    // Refcounting utilities:
    // Increment or start refcount at 1
    void incRefCount(const String& name, ResourceType type);
    // Decrement and possibly request unloading
    void decRefCount(const String& name);


    const Duration mPerFrameTime;
    TimeProfiler::Stage* mProfilerStage;

    // This is just a task queue where tick() makes sure we stop when we've
    // passed our time threshold.
    typedef std::tr1::function<void()> Task;
    std::queue<Task> mTasks;

    // Track ref counts for each object. To simplify management for
    // the users of this class, we track the type.
    struct ResourceData {
        ResourceData(ResourceType t)
         : type(t), refcount(1)
        {}

        ResourceType type;
        int32 refcount;
    };
    typedef std::map<String, ResourceData> RefCountMap;
    RefCountMap mRefCounts;


    // Ogre currently has a problem with removing materials
    // (specifically some internal state about Passes isn't properly
    // marked as dirty), which apparently has something to do with
    // multiple render systems (we have at least the main one and one
    // for overlays). When we remove a material we need to reset
    // render queues to force things back to a clean state.
    // (See http://www.ogre3d.org/forums/viewtopic.php?p=282082)
    bool mNeedRenderQueuesReset;
    void resetRenderQueues();

};

} // namespace Graphics
} // namespace Sirikata


#endif //_SIRIKATA_OGRE_RESOURCE_LOADER_HPP_
