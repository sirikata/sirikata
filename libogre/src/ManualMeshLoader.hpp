// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_MANUAL_MESH_LOADER_HPP_
#define _SIRIKATA_OGRE_MANUAL_MESH_LOADER_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/Util.hpp>
#include <sirikata/mesh/Meshdata.hpp>
#include <OgreResource.h>
#include <OgreHardwareVertexBuffer.h>

namespace Sirikata {
namespace Graphics {

class ManualMeshLoader : public Ogre::ManualResourceLoader {
public:
    ManualMeshLoader(Mesh::MeshdataPtr meshdata, TextureBindingsMapPtr textureFingerprints);

    void prepareResource(Ogre::Resource*r) {}
    void loadResource(Ogre::Resource *r);

private:
    Ogre::VertexData* createVertexData(const Mesh::SubMeshGeometry &submesh, int vertexCount, Ogre::HardwareVertexBufferSharedPtr&vbuf);
    void getMeshStats(bool* useSharedBufferOut, size_t* totalVertexCountOut);
    void traverseNodes(Ogre::Resource* r, const bool useSharedBuffer, const size_t totalVertexCount);

    Mesh::MeshdataPtr mdptr;
    TextureBindingsMapPtr mTextureFingerprints;
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_MANUAL_MESH_LOADER_HPP_
