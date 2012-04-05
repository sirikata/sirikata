// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_UTIL_HPP_
#define _SIRIKATA_OGRE_UTIL_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/core/util/Logging.hpp>
#include <sirikata/mesh/Visual.hpp>
#include <sirikata/mesh/Meshdata.hpp>

#define OGRE_LOG(lvl, msg) SILOG(ogre, lvl, msg)

namespace Sirikata {
namespace Graphics {

// Maps texture URIs -> hashes
typedef std::map<String, String> TextureBindingsMap;
typedef std::tr1::shared_ptr<TextureBindingsMap> TextureBindingsMapPtr;

// Get names for different components of meshes, where possible using hashes in
// order to enable reuse if the same component is encountered multiple times,
// even across different meshes. For example, mesh names combine the hash of
// their dae contents and hashes of textures they depend on to ensure uniqueness
// while enabling reuse.
String ogreVisualName(Mesh::VisualPtr vis, TextureBindingsMapPtr bindings);
String ogreMaterialName(const Mesh::MaterialEffectInfo& mat, const Transfer::URI& parent_uri, TextureBindingsMapPtr bindings);
// This version of ogreMaterialName doesn't take a MaterialEffectInfo because
// billboards have a fixed material setup.
String ogreBillboardMaterialName(TextureBindingsMapPtr bindings);
String ogreSkeletonName(Mesh::MeshdataPtr md, TextureBindingsMapPtr bindings);
String ogreLightName(const String& entityname, const String& meshname, int32 light_idx);

} // namespace Graphics
} // namespace Sirikata


#endif //_SIRIKATA_OGRE_UTIL_HPP_
