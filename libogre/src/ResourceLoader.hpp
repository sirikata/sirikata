// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_RESOURCE_LOADER_HPP_
#define _SIRIKATA_OGRE_RESOURCE_LOADER_HPP_

#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/core/transfer/URI.hpp>
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

    ResourceLoader();

    void loadMaterial(const String& name, Mesh::MeshdataPtr mesh, const Mesh::MaterialEffectInfo& mat, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);
    void loadBillboardMaterial(const String& name, const String& texuri, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);
    void unloadMaterial(const String& name);

    void loadSkeleton(const String& name, Mesh::MeshdataPtr mesh, const std::set<String>& animationList, LoadedCallback cb);
    void unloadSkeleton(const String& name);

    void loadMesh(const String& name, Mesh::MeshdataPtr mesh, const String& skeletonName, LoadedCallback cb);
    void unloadMesh(const String& name);

    void loadTexture(const String& name, LoadedCallback cb);
    void unloadTexture(const String& name);

    void tick();

private:
    void loadMaterialWork(const String& name, Mesh::MeshdataPtr mesh, const Mesh::MaterialEffectInfo& mat, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadBillboardMaterialWork(const String& name, const String& texuri, const Transfer::URI& uri, TextureBindingsMapPtr textureFingerprints, LoadedCallback cb);

    void loadSkeletonWork(const String& name, Mesh::MeshdataPtr mesh, const std::set<String>& animationList, LoadedCallback cb);

    void loadMeshWork(const String& name, Mesh::MeshdataPtr mesh, const String& skeletonName, LoadedCallback cb);

    void loadTextureWork(const String& name, LoadedCallback cb);

    // This is just a task queue where tick() makes sure we stop when we've
    // passed our time threshold.
    typedef std::tr1::function<void()> Task;
    std::queue<Task> mTasks;
};

} // namespace Graphics
} // namespace Sirikata


#endif //_SIRIKATA_OGRE_RESOURCE_LOADER_HPP_
