// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OGRE_UTIL_HPP_
#define _SIRIKATA_OGRE_UTIL_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/core/util/Logging.hpp>

#define OGRE_LOG(lvl, msg) SILOG(ogre, lvl, msg)

namespace Sirikata {
namespace Graphics {

typedef std::map<String, String> TextureBindingsMap;
typedef std::tr1::shared_ptr<TextureBindingsMap> TextureBindingsMapPtr;

String ogreMaterialName(const String& meshname, int32 mat_idx);
String ogreSkeletonName(const String& meshname);
String ogreLightName(const String& entityname, const String& meshname, int32 light_idx);

} // namespace Graphics
} // namespace Sirikata


#endif //_SIRIKATA_OGRE_UTIL_HPP_
