// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/ogre/Util.hpp>
#include <boost/lexical_cast.hpp>


namespace Sirikata {
namespace Graphics {

String ogreMaterialName(const String& meshname, int32 mat_idx) {
    return meshname + "_mat_" + boost::lexical_cast<String>(mat_idx);
}

String ogreSkeletonName(const String& meshname) {
    return meshname+"_skeleton";
}

String ogreLightName(const String& entityname, const String& meshname, int32 light_idx) {
    return entityname + "_light_" + meshname + boost::lexical_cast<String>(light_idx);
}

} // namespace Graphics
} // namespace Sirikata
