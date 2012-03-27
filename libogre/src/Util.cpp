// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/ogre/Util.hpp>
#include <boost/lexical_cast.hpp>
#include <sirikata/core/transfer/URL.hpp>

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

String ogreMaterialName(const Mesh::MaterialEffectInfo& mat, const Transfer::URI& parent_uri, TextureBindingsMapPtr bindings) {
    std::ostringstream data;

    // We have to deal with all the properties of the material.
    // A couple of things are
    data << "shininess_" << mat.shininess << "_";
    data << "reflectivity_" << mat.reflectivity << "_";

    for(uint32 texn = 0; texn < mat.textures.size(); texn++) {
        data << "tex" << texn << "_(";
        const Mesh::MaterialEffectInfo::Texture& tex = mat.textures[texn];
        data << "affecting_" << (int)tex.affecting << "_";
        if (tex.uri.length() == 0) {
            data << "color_" << tex.color << "_";
        }
        else {
            Transfer::URL url(parent_uri);
            assert(!url.empty());
            Transfer::URI mat_uri( Transfer::URL(url.context(), tex.uri).toString() );
            TextureBindingsMap::const_iterator where = bindings->find(mat_uri.toString());
            if (where != bindings->end())
                data << "tex_" << where->second << "_";

            data << "texCoord_" << tex.texCoord << "_";
            // filter, sampler type?
            data << "wrapS_" << (int)tex.wrapS << "_";
            data << "wrapT_" << (int)tex.wrapT << "_";
            data << "wrapU_" << (int)tex.wrapU << "_";
            data << "maxMipLevel_" << tex.maxMipLevel << "_";
            data << "mipBias_" << tex.mipBias << "_";
        }

        data << ")_";
    }
    return String("mat_") + SHA256::computeDigest(data.str()).toString();
}

String ogreBillboardMaterialName(TextureBindingsMapPtr bindings) {
    // Billboard materials only need to deal with textures.
    String data = "";
    for(TextureBindingsMap::const_iterator tex_it = bindings->begin(); tex_it != bindings->end(); tex_it++)
        data += tex_it->second;
    return String("matbb_") + SHA256::computeDigest(data).toString();
}

String ogreSkeletonName(Mesh::MeshdataPtr md, TextureBindingsMapPtr bindings) {
    // TODO(ewencp) this should really be some hash of the skeleton data, but
    // this works fine for now. We probably won't have a ton of reuse of
    // skeletons across different meshes anyway...
    return ogreVisualName(md, bindings) + "_skeleton";
}

String ogreLightName(const String& entityname, const String& meshname, int32 light_idx) {
    return entityname + "_light_" + meshname + boost::lexical_cast<String>(light_idx);
}

} // namespace Graphics
} // namespace Sirikata
