/*  Sirikata
 *  TextureAtlasFilter.cpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#include "TextureAtlasFilter.hpp"
#include "FreeImage.hpp"
#include <nvtt/nvtt.h>

namespace Sirikata {
namespace Mesh {

namespace {
struct Rect;
struct Region;

// Rect is used to define the size in pixels of a rectangular region
struct Rect {
    int32 min_x;
    int32 min_y;
    int32 max_x;
    int32 max_y;

    int32 width() const { return max_x - min_x + 1; }
    int32 height() const { return max_y - min_y + 1; }
    int32 x() const { return min_x; }
    int32 y() const { return min_y; }

    static Rect fromBaseOffset(int32 _min_x, int32 _min_y, int32 _width, int32 _height) {
        Rect ret;
        ret.min_x = _min_x;
        ret.min_y = _min_y;
        ret.max_x = _min_x + _width - 1;
        ret.max_y = _min_y + _height - 1;
        return ret;
    }
    static Rect fromBounds(int32 _min_x, int32 _min_y, int32 _max_x, int32 _max_y) {
        Rect ret;
        ret.min_x = _min_x;
        ret.min_y = _min_y;
        ret.max_x = _max_x;
        ret.max_y = _max_y;
        return ret;
    }

    Region region(const Rect& subrect);
    Rect region(const Region& subreg);
};

// Region is used to define a region of a Rect. It's values are always in the
// range 0 to 1
struct Region {
    float min_x;
    float min_y;
    float max_x;
    float max_y;

    static Region fromBaseOffset(float _min_x, float _min_y, float _width, float _height) {
        Region ret;
        ret.min_x = _min_x;
        ret.min_y = _min_y;
        ret.max_x = _min_x + _width;
        ret.max_y = _min_y + _height;
        return ret;
    }
    static Region fromBounds(float _min_x, float _min_y, float _max_x, float _max_y) {
        Region ret;
        ret.min_x = _min_x;
        ret.min_y = _min_y;
        ret.max_x = _max_x;
        ret.max_y = _max_y;
        return ret;
    }

    float width() const { return max_x - min_x; }
    float height() const { return max_y - min_y; }
    float x() const { return min_x; }
    float y() const { return min_y; }

    // Convert a (u,v) in this region's coordinates (u and v between 0 and 1) to
    // parent coordinates.
    void convert(float u, float v, float* u_out, float* v_out) {
        float u_o = min_x + u * width();
        float v_o = min_y + v * height();
        *u_out = min_x + u * width();
        *v_out = min_y + v * height();
    }
};

Region Rect::region(const Rect& subrect) {
    return Region::fromBounds(
        (subrect.min_x/(float)width()), (subrect.min_y/(float)height()),
        ((subrect.max_x+1)/(float)width()), ((subrect.max_y+1)/(float)height())
    );
}

Rect Rect::region(const Region& subreg) {
    return Rect::fromBaseOffset(
        (int32)(subreg.min_x * width()), (int32)(subreg.min_y * height()),
        (int32)(subreg.width() * width()), (int32)(subreg.height() * height())
    );
}

// Track some information about the texture that we need as we process it.
struct TexInfo {
    String url;
    Rect orig_size;
    FIBITMAP* image;
    Region atlas_region;
};

// Compute a hash value for texture sets in a Meshdata.
int TextureSetHash(MeshdataPtr md, int sub_mesh_idx, int tex_set_idx) {
    return (tex_set_idx * md->geometry.size()) + sub_mesh_idx;
}

} // namespace

TextureAtlasFilter::TextureAtlasFilter(const String& args) {
}

MeshdataPtr TextureAtlasFilter::apply(MeshdataPtr md) {
    // Currently we use a very simple approach: take all textures and
    // tile them into equal size regions, tiling in 2D.

    typedef std::map<String, TexInfo> TexInfoMap;
    TexInfoMap tex_info;

    // We only know how to handle local files
    if (md->uri.size() < 7 || md->uri.substr(0, 7) != "file://") return md;
    String uri_dir = md->uri.substr(7);
    size_t slash_pos = uri_dir.rfind("/");
    if (slash_pos == std::string::npos || slash_pos == uri_dir.size()-1)
        uri_dir = "./";
    else
        uri_dir = uri_dir.substr(0, slash_pos+1);

    // If the same texUVs in a SubMeshGeometry are referenced by instances with
    // different materials, then this code doesn't know how to handle them (we'd
    // probably have to duplicate texUVs, and in doing so, decide whether the
    // memory increase is worth it).
    std::map<int, int> tex_set_materials; // Unique tex coord hash -> material
                                          // idx
    {
        // We need to run through all instances and look for conflicts. A
        // texture coordinate set is uniquely identified by SubMeshGeometry
        // index and the index in SubMeshGeometry::texUVs. Since we know the
        // number of SubMeshGeometries, we use
        //   (tex coord index * num sub mesh geometries) + sub mesh geo index
        // to generate a unique index. Many instances may map to one of these
        // and we just want to check that no two instances use different textures
        Meshdata::GeometryInstanceIterator geo_inst_it = md->getGeometryInstanceIterator();
        uint32 geo_inst_idx;
        Matrix4x4f pos_xform;
        bool conflicts = false;
        while( geo_inst_it.next(&geo_inst_idx, &pos_xform) ) {
            GeometryInstance& geo_inst = md->instances[geo_inst_idx];
            int sub_mesh_idx = geo_inst.geometryIndex;
            for(GeometryInstance::MaterialBindingMap::iterator mat_it = geo_inst.materialBindingMap.begin(); mat_it != geo_inst.materialBindingMap.end(); mat_it++) {
                int texset_idx = mat_it->first;
                int mat_idx = mat_it->second;

                int tex_set_hash = TextureSetHash(md, sub_mesh_idx, texset_idx);
                if (tex_set_materials.find(tex_set_hash) != tex_set_materials.end() &&
                    tex_set_materials[tex_set_hash] != mat_idx)
                {
                    // Conflict, can't handle, pass on without any processing
                    SILOG(textureatlas, error, "[TEXTUREATLAS] Found two instances using different materials referencing the same geometry -- can't safely generate atlas.");
                    return md;
                }
                tex_set_materials[tex_set_hash] = mat_idx;
            }
        }
    }

    // First compute some stats.
    for(uint32 tex_i = 0; tex_i < md->textures.size(); tex_i++) {
        String tex_url = md->textures[tex_i];
        String texfile = uri_dir + tex_url;

        tex_info[tex_url] = TexInfo();
        tex_info[tex_url].url = tex_url;

        // Load the data from the original file
        FIBITMAP* input_image = GenericLoader(texfile.c_str(), 0);
        if (input_image == NULL) { // Couldn't figure out how to load it
            tex_info[tex_url].image = NULL;
            continue;
        }

        // Make sure we get the data in BGRA 8UB and properly packed
        FIBITMAP* input_image_32 = FreeImage_ConvertTo32Bits(input_image);
        FreeImage_Unload(input_image);
        tex_info[tex_url].image = input_image_32;

        int32 width = FreeImage_GetWidth(input_image_32);
        int32 height = FreeImage_GetHeight(input_image_32);

        tex_info[tex_url].orig_size = Rect::fromBaseOffset(0, 0, width, height);
    }

    // Select the block size and layout, create target image
    int ntextures = tex_info.size();
    int ntextures_side = (int)(sqrtf((float)ntextures) + 1.f);

    // FIXME
    uint32 atlas_element_width = 256, atlas_element_height = 256;

    uint32 atlas_width = ntextures_side * atlas_element_width,
        atlas_height = ntextures_side * atlas_element_height;

    FIBITMAP* atlas = FreeImage_Allocate(atlas_width, atlas_height, 32);

    // Scale images to desired size, copy data into the target image
    int idx = 0;
    Rect atlas_rect = Rect::fromBaseOffset(0, 0, atlas_width, atlas_height);
    for(TexInfoMap::iterator tex_it = tex_info.begin(); tex_it != tex_info.end(); tex_it++) {
        TexInfo& tex = tex_it->second;
        // Resize
        uint32 new_width = atlas_element_width, new_height = atlas_element_height; // FIXME
        FIBITMAP* resized = FreeImage_Rescale(tex.image, new_width, new_height, FILTER_LANCZOS3);
        // Copy into place
        int x_idx = idx % ntextures_side;
        int y_idx = idx / ntextures_side;

        Rect tex_sub_rect = Rect::fromBaseOffset(
            x_idx * atlas_element_width, y_idx * atlas_element_height,
            atlas_element_width, atlas_element_height
        );

        tex.atlas_region = atlas_rect.region(tex_sub_rect);
        FreeImage_Paste(atlas, resized, tex_sub_rect.min_x, tex_sub_rect.min_y, 256);

        FreeImage_Unload(resized);
        FreeImage_Unload(tex_it->second.image);
        tex_it->second.image = NULL;

        idx++;
    }

    // Generate the texture atlas
    String atlas_url = uri_dir + "atlas.png";
    FreeImage_Save(FIF_PNG, atlas, atlas_url.c_str());
    FreeImage_Unload(atlas);

    // Now we need to run through and fix up texture references and texture
    // coordinates
    // 1. Replace old textures with new one
    TextureList orig_texture_list = md->textures;
    md->textures.clear();
    md->textures.push_back(atlas_url);
    // 2. Update texture coordinates. Since we've stored texture set hash ->
    // material indices, we can easily work from this to quickly iterate over
    // items and transform the coordinates.
    for(uint32 geo_idx = 0; geo_idx < md->geometry.size(); geo_idx++) {
        // Each submesh has texUVs that need updating
        SubMeshGeometry& submesh = md->geometry[geo_idx];

        // We need to modify each texture coordinate set in this geometry. We
        // put this as the outer loop so we replace the entire uv set with a
        // new, filtered copy so we make sure we don't double-transform any of
        // the coordinates.
        for(int tex_set_idx = 0; tex_set_idx < submesh.texUVs.size(); tex_set_idx++) {
            SubMeshGeometry::TextureSet& tex_set = submesh.texUVs[tex_set_idx];
            std::vector<float> new_uvs = tex_set.uvs;

            // Each prim defines a material mapping, so we need to split the
            // texture coordinates up by prim.
            for(int prim_idx = 0; prim_idx < submesh.primitives.size(); prim_idx++) {
                SubMeshGeometry::Primitive& prim = submesh.primitives[prim_idx];
                int mat_id = prim.materialId;

                int tex_set_hash = TextureSetHash(md, geo_idx, mat_id);
                // We stored up the correct material index to find it in the
                // Meshdata in the tex_set_materials when we were sanity checking
                // that there were no conflicts.
                int mat_idx = tex_set_materials[tex_set_hash];

                // Finally, having extracted all the material mapping info, we
                // can loop through referenced indices and transform them.
                MaterialEffectInfo& mat = md->materials[mat_idx];

                // Do transformation for each texture that has a URI. Some may
                // be duplicates, some may not have a URI, but doing them all
                // ensures we catch everything.
                for(int mat_tex_idx = 0; mat_tex_idx < mat.textures.size(); mat_tex_idx++) {
                    MaterialEffectInfo::Texture& real_tex = mat.textures[mat_tex_idx];
                    if (tex_info.find(real_tex.uri) == tex_info.end()) continue;
                    TexInfo& final_tex_info = tex_info[real_tex.uri];

                    for(int index_idx = 0; index_idx < prim.indices.size(); index_idx++) {
                        int index = prim.indices[index_idx];
                        float new_u, new_v;
                        assert(tex_set.stride >= 2);
                        final_tex_info.atlas_region.convert(
                            tex_set.uvs[index * tex_set.stride],
                            tex_set.uvs[index * tex_set.stride + 1],
                            &new_u, &new_v);
                        new_uvs[ index * tex_set.stride ] = new_u;
                        new_uvs[ index * tex_set.stride + 1 ] = new_v;
                    }
                }
            }
            tex_set.uvs = new_uvs;
        }
    }
    // 3. Replace references in materials to old URI's with new one
    for(MaterialEffectInfoList::iterator mat_it = md->materials.begin(); mat_it != md->materials.end(); mat_it++) {
        MaterialEffectInfo& mat_info = *mat_it;
        for(MaterialEffectInfo::TextureList::iterator tex_it = mat_info.textures.begin(); tex_it != mat_info.textures.end(); tex_it++) {
            tex_it->uri = atlas_url;
        }
    }

    return md;
}

FilterDataPtr TextureAtlasFilter::apply(FilterDataPtr input) {
    using namespace nvtt;

    InitFreeImage();

    MutableFilterDataPtr output(new FilterData());
    for(FilterData::const_iterator mesh_it = input->begin(); mesh_it != input->end(); mesh_it++) {
        MeshdataPtr mesh = *mesh_it;
        MeshdataPtr ta_mesh = apply(mesh);
        output->push_back(ta_mesh);
    }
    return output;
}

} // namespace Mesh
} // namespace Sirikata
