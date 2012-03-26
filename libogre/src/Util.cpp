// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/ogre/Util.hpp>
#include <boost/lexical_cast.hpp>


namespace Sirikata {
namespace Graphics {

// Utility to compute full hash of the content of a visual. This is needed
// because the hash of some date (e.g. a collada file) may be the same, but
// some textures may differ (the relative names are the same, but the data
// the absolute names point to differs).
String ogreVisualName(Mesh::VisualPtr vis, TextureBindingsMapPtr bindings) {
    // To compute a hash of the entire visual, we just combine the hashes of the
    // original and all the dependent resources, then hash the result. The
    // approach right now uses strings and could do somethiing like xor'ing
    // everything instead, but this is easy and shouldn't happen all that often.
    String data = vis->hash.toString();

    // This assumes we're using a map or some sorted container so we get
    // consistent ordering.
    for(TextureBindingsMap::const_iterator tex_it = bindings->begin(); tex_it != bindings->end(); tex_it++)
        data += tex_it->second;

    return String("vis_") + SHA256::computeDigest(data).toString();
}

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
