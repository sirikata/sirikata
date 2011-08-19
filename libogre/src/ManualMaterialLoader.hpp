// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_MANUAL_MATERIAL_LOADER_HPP_
#define _SIRIKATA_OGRE_MANUAL_MATERIAL_LOADER_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/Util.hpp>
#include <OgreResource.h>
#include <sirikata/mesh/Visual.hpp>
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/core/transfer/URI.hpp>

namespace Sirikata {
namespace Graphics {

typedef std::map<String, String> TextureBindingsMap;
typedef std::tr1::shared_ptr<TextureBindingsMap> TextureBindingsMapPtr;

class ManualMaterialLoader : public Ogre::ManualResourceLoader {
public:
    ManualMaterialLoader(
        Mesh::VisualPtr visptr,
        const String name,
        const Mesh::MaterialEffectInfo&mat,
        const Transfer::URI& uri,
        TextureBindingsMapPtr textureFingerprints);

    void prepareResource(Ogre::Resource*r){}
    void loadResource(Ogre::Resource *r);

private:
    Mesh::VisualPtr mVisualPtr;
    const Mesh::MaterialEffectInfo* mMat;
    String mName;
    Transfer::URI mURI;
    TextureBindingsMapPtr mTextureFingerprints;
};


} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_MANUAL_MATERIAL_LOADER_HPP_
