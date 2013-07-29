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

#include <sirikata/mesh/Platform.hpp>

#include <sirikata/core/util/Vector4.hpp>
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
 
    Rect() {
      min_x = min_y = max_x = max_y = 0;
    }

    int32 width() const { return max_x - min_x + 1; }
    int32 height() const { return max_y - min_y + 1; }
    int32 x() const { return min_x; }
    int32 y() const { return min_y; }

    static Rect fromBaseOffset(int32 _min_x, int32 _min_y, int32 _width, int32 _height) {
        if (_width == 0) _width = 1;
        if (_height == 0) _height = 1;
       
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

    void printString() {
      SILOG(textureatlas, detailed, "[" << min_x << ", " << min_y << "] [" << max_x << ", " << max_y << "]");
    }    
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

    void printString() {
      SILOG(textureatlas, detailed,  "[" << min_x << ", " << min_y << "] [" << max_x << ", " << max_y << "]");
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

    //Crude way to deal with UV wrap_around (make copies of the texture)
    //just for the case
    //where the charted texture is not available.
    Rect num_copies;

    TexInfo() {
      num_copies.min_x = 0;
      num_copies.min_y = 0;
      num_copies.max_x = 1;
      num_copies.max_y = 1;
    }
};

// Compute a hash value for texture sets in a Meshdata.
int TextureSetHash(MeshdataPtr md, int sub_mesh_idx, int tex_set_idx) {
    return (tex_set_idx * md->geometry.size()) + sub_mesh_idx;
}


void getColors(MeshdataPtr md, std::vector<Vector4f>& colors) {
  std::tr1::unordered_set<String> colorSet;

  for (uint32 i = 0; i < md->materials.size(); i++) {
    for (uint32 j = 0; j < md->materials[i].textures.size(); j++) {
      const MaterialEffectInfo::Texture& texture = md->materials[i].textures[j];
      if (texture.uri == "" && texture.affecting == texture.DIFFUSE) {
	char colorEncodingStr[512];
	sprintf(colorEncodingStr, "%f %f %f %f %d", texture.color.x, texture.color.y,
		texture.color.z, texture.color.w, texture.affecting);
	String colorEncoding = colorEncodingStr;

        if (colorSet.find(colorEncoding) == colorSet.end()) {
	  colorSet.insert(colorEncoding);
	  colors.push_back(texture.color);
	}
      }
    }
  }
}

} // namespace

TextureAtlasFilter::TextureAtlasFilter(const String& args) {
}

MeshdataPtr TextureAtlasFilter::apply(MeshdataPtr md) {
    typedef std::map<String, TexInfo> TexInfoMap;
    TexInfoMap tex_info;


    // We only know how to handle local files
    if (md->uri.size() < 7 || md->uri.substr(0, 7) != "file://") return md;
    String uri_dir = md->uri.substr(7);
    String uri_sans_protocol = uri_dir;
    size_t slash_pos = uri_dir.rfind("/");

    String file_name="";
    if (slash_pos == std::string::npos || slash_pos == uri_dir.size()-1) {
        uri_dir = "./";
        file_name = uri_sans_protocol;
    }
    else {
        uri_dir = uri_dir.substr(0, slash_pos+1);
        file_name = uri_sans_protocol.substr(slash_pos+1);
    }

    for (uint32 i=0; i< md->geometry.size(); i++) {
          SubMeshGeometry& smg = md->geometry[i];
          if (smg.texUVs.size() == 0) {
            SubMeshGeometry::TextureSet ts;
            ts.stride = 2;
            for (uint32 posi = 0 ; posi < smg.positions.size(); posi++) {
              ts.uvs.push_back(0);ts.uvs.push_back(0);
            }
            smg.texUVs.push_back(ts);
          }

    }

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
                    SILOG(textureatlas, error, "Found two instances using different materials referencing the same geometry -- can't safely generate atlas.");
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

    for(uint32 geo_idx = 0; geo_idx < md->geometry.size(); geo_idx++) {
      // Each submesh has texUVs that need updating                                                      
      SubMeshGeometry& submesh = md->geometry[geo_idx];

      // We need to modify each texture coordinate set in this geometry. We
      // put this as the outer loop so we replace the entire uv set with a                                 
      // new, filtered copy so we make sure we don't double-transform any of          
      // the coordinates.                                                                                
      for(uint32 tex_set_idx = 0; tex_set_idx < submesh.texUVs.size(); tex_set_idx++) {
        SubMeshGeometry::TextureSet& tex_set = submesh.texUVs[tex_set_idx];
        std::vector<float> new_uvs = tex_set.uvs;

        // Each prim defines a material mapping, so we need to split the                                     
        // texture coordinates up by prim.                                                             
        for(uint32 prim_idx = 0; prim_idx < submesh.primitives.size(); prim_idx++) {
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
          for(uint32 mat_tex_idx = 0; mat_tex_idx < mat.textures.size(); mat_tex_idx++) {
            MaterialEffectInfo::Texture& real_tex = mat.textures[mat_tex_idx];
            if (tex_info.find(real_tex.uri) == tex_info.end()) continue;
            TexInfo& final_tex_info = tex_info[real_tex.uri];

            for(uint32 index_idx = 0; index_idx < prim.indices.size(); index_idx++) {
              int index = prim.indices[index_idx]; assert (index_idx < prim.indices.size());

              float new_u, new_v;
              assert(tex_set.stride >= 2);
              float old_u = tex_set.uvs[index * tex_set.stride];
              float old_v = 1.f - tex_set.uvs[index * tex_set.stride + 1]; //inverted vcoord
              assert(index * tex_set.stride + 1 < tex_set.uvs.size());

              if ( ceil(old_u) > final_tex_info.num_copies.max_x)
                final_tex_info.num_copies.max_x = ceil(old_u);
              if (floor(old_u) < final_tex_info.num_copies.min_x)
                final_tex_info.num_copies.min_x =floor(old_u);

              if ( ceil(old_v) > final_tex_info.num_copies.max_y)
                final_tex_info.num_copies.max_y =ceil(old_v);
              if (floor(old_v) < final_tex_info.num_copies.min_y)
                final_tex_info.num_copies.min_y =floor(old_v);
            }
          }
        }
      }
    }

    std::vector<Vector4f> colorList;
    getColors(md, colorList);

    // Select the block size and layout, create target image
    int ntextures = tex_info.size()+colorList.size();
    int ntextures_side = (int)(sqrtf((float)ntextures) + 1.f);

    // Figure out how large the overall atlas should be.
    uint32 minHeight = 256, minWidth = 256;
    uint32 maxRowWidth = 0, curRowWidth = 0, totalHeight = 0, curRowHeight = 0;
    uint32 counter = 0;
    for(TexInfoMap::iterator tex_it = tex_info.begin(); tex_it != tex_info.end(); tex_it++) {
      TexInfo& tex = tex_it->second;
      if (counter % ntextures_side == 0) {
        //New row starting... do some book-keeping.
        if (curRowWidth > maxRowWidth) {
          maxRowWidth = curRowWidth;
        }
        curRowWidth = 0;
        totalHeight += curRowHeight;
        curRowHeight = 0;
      }
      counter++;

      float32 tex_width = tex.orig_size.width(), tex_height = tex.orig_size.height();
      if (tex_width < minWidth && tex_height < minHeight) {
        // Textures that are really small 
        // should be enlarged, so that they can be indexed
        // into more accurately.
        tex_height = tex_height / tex_width * minWidth;
        tex_width = minWidth;
      }
      float32 num_copies_x = ( tex.num_copies.max_x - tex.num_copies.min_x );
      float32 num_copies_y = ( tex.num_copies.max_y - tex.num_copies.min_y ) ;

      if (tex_width * num_copies_x * tex_height * num_copies_y > 256*256) {
        float32 factor = sqrtf(tex_width * num_copies_x * tex_height * num_copies_y / (256.0 * 256.0));
        tex_width = tex_width/factor * num_copies_x;
        tex_height = tex_height/factor * num_copies_y;
      }
      else {
        tex_width *= num_copies_x;
        tex_height *= num_copies_y;
      }
  
      curRowWidth += tex_width;  
      if (tex_height > curRowHeight)
        curRowHeight = tex_height;
    }
    //Include colors in the atlas... find out how many color elements are needed.
    /*for (uint32 i=0; i < colorList.size() ; i++) {
      if (counter % ntextures_side == 0) {
        //New row starting... do some book-keeping.
        if (curRowWidth > maxRowWidth) {
          maxRowWidth = curRowWidth;
        }
        curRowWidth = 0;
        totalHeight += curRowHeight;
        curRowHeight = 0;
      }
      counter++;

      float32 tex_width = 32, tex_height = 32;
      curRowWidth += tex_width;
      if (tex_height > curRowHeight)
        curRowHeight = tex_height;
    } */
    
    if (curRowWidth > maxRowWidth) {
      maxRowWidth = curRowWidth;
    }
    totalHeight += curRowHeight;
    

    // OK, create the atlas!!
    float32 atlas_width = maxRowWidth, atlas_height = totalHeight;
    
    // The number of pixels we want per atlas. 32768 px at 8 bit RGBA
    // corresponds to 128 KB of texture RAM,
    // 65536 px corresponds to 256 KB of texture RAM and so on.
    float32 scaled_atlas_pixels = 32768/2.0;
    float32 largeFactor = 1.0;
    if (atlas_width * atlas_height > scaled_atlas_pixels)
      largeFactor = sqrtf(atlas_width * atlas_height/scaled_atlas_pixels);
    
    float32 scaled_atlas_width = atlas_width/largeFactor, 
            scaled_atlas_height= atlas_height/largeFactor;

    FIBITMAP* scaled_atlas = 
              FreeImage_Allocate(scaled_atlas_width, scaled_atlas_height, 32);

    uint32 xLoc = 0, yLoc = 0;
    counter = 0;
    curRowHeight = 0;
    curRowWidth = 0;
    Rect atlas_rect = Rect::fromBaseOffset(0, 0, scaled_atlas_width, scaled_atlas_height);

    //Copy and paste the re-scaled textures first
    for(TexInfoMap::iterator tex_it = tex_info.begin(); tex_it != tex_info.end(); tex_it++) {
      TexInfo& tex = tex_it->second;
      if (counter % ntextures_side == 0) {
        //Book-keeping for a new row.
        xLoc = 0;
        yLoc += curRowHeight;
        curRowHeight = 0;
        curRowWidth = 0;
      }
      counter++;

      //First create the individual tiled texture.
      float32 tex_width = tex.orig_size.width(), tex_height = tex.orig_size.height();
      if (tex_width < minWidth && tex_height < minHeight) {
        // Textures that are really small 
        // should be enlarged, so that they can be indexed
        // into more accurately.
        tex_height = tex_height / tex_width * minWidth;
        tex_width = minWidth;
      }
      int32 num_copies_x = ( tex.num_copies.max_x - tex.num_copies.min_x );
      int32 num_copies_y = ( tex.num_copies.max_y - tex.num_copies.min_y );

      //Create a single texture multiple tiled copies of the original texture
      //to deal with texture wrapping. Only works for repeating wrap mode.

      float32 tiled_texture_width = tex_width * num_copies_x;
      float32 tiled_texture_height = tex_height * num_copies_y; 
      if (tiled_texture_width * tiled_texture_height > 256.0 * 256.0) {
        float32 factor = sqrtf(tiled_texture_width * tiled_texture_height / (256.0 * 256.0));
        tex_width = tex_width/factor;
        tex_height = tex_height/factor;
        tiled_texture_width = tex_width * num_copies_x;
        tiled_texture_height = tex_height * num_copies_y;
      }
      FIBITMAP* resized = FreeImage_Rescale(tex.image, tex_width, tex_height, FILTER_LANCZOS3);
      FIBITMAP* tiled_texture =
              FreeImage_Allocate(tiled_texture_width, tiled_texture_height, 32);
      for (uint32 xcopy = 0; xcopy < num_copies_x; xcopy++) {
         for (uint32 ycopy = 0; ycopy < num_copies_y; ycopy++) {
           FreeImage_Paste(tiled_texture, resized, xcopy * tex_width, ycopy * tex_height, 256);
         }
      }
      FreeImage_Unload(resized);

      FIBITMAP* resized_tiled = FreeImage_Rescale(tiled_texture, (int)(tiled_texture_width/largeFactor), (int)(tiled_texture_height/largeFactor), FILTER_LANCZOS3);

      //Copy the texture image into the atlas
      Rect tex_sub_rect = Rect::fromBaseOffset(xLoc/largeFactor, yLoc/largeFactor, tiled_texture_width/largeFactor, tiled_texture_height/largeFactor);
      FreeImage_Paste(scaled_atlas, resized_tiled, tex_sub_rect.min_x, tex_sub_rect.min_y, 256);

      tex_sub_rect = Rect::fromBaseOffset( (xLoc - tex.num_copies.min_x*tex_width)/largeFactor, (yLoc - tex.num_copies.min_y*tex_height)/largeFactor, tex_width/largeFactor, tex_height/largeFactor);
      tex.atlas_region = atlas_rect.region(tex_sub_rect);

      FreeImage_Unload(resized_tiled);
      FreeImage_Unload(tiled_texture);
      FreeImage_Unload(tex_it->second.image);
      tex_it->second.image = NULL;

      //more book-keeping.
      curRowWidth += tiled_texture_width;
      if (tiled_texture_height > curRowHeight)
        curRowHeight = tiled_texture_height;
      xLoc = curRowWidth;
    }

    //Now include the colors in the atlas. Commented out currently since this
    //increases pressure on texture RAM.
    std::tr1::unordered_map<Vector4f, Region, Vector4f::Hasher> colorRegionMap;
    /*for (uint32 i=0; i < colorList.size(); i++) {
      if (counter % ntextures_side == 0) {
        //New row starting... do some book-keeping.
        xLoc = 0;
        yLoc += curRowHeight;
        curRowWidth = 0;
        curRowHeight = 0;
      }
      counter++;

      float32 tex_width = 32, tex_height = 32;
      FIBITMAP* color_texture = FreeImage_Allocate(tex_width, tex_height, 32);
      Vector4f color = colorList[i];
      SILOG(textureatlas, detailed, colorList[i] << " : colorList[i]");
      RGBQUAD rgbColor;  
      rgbColor.rgbRed =   color.x*255.f;
      rgbColor.rgbGreen = color.y*255.f;
      rgbColor.rgbBlue = color.z*255.f;
      rgbColor.rgbReserved =  color.w*255.f;
      for (uint32 w = 0; w < tex_width; w++) {
        for (uint32 h = 0; h < tex_height; h++)
          FreeImage_SetPixelColor(color_texture, w, h, &rgbColor);
      }
   
      FIBITMAP* resized_color = FreeImage_Rescale(color_texture, (int)(tex_width/largeFactor), (int)(tex_height/largeFactor), FILTER_LANCZOS3);
      //copy the texture image into the atlas
      Rect tex_sub_rect = Rect::fromBaseOffset(xLoc/largeFactor, yLoc/largeFactor, tex_width/largeFactor, tex_height/largeFactor);
      SILOG(textureatlas, detailed, "Copying into: "); tex_sub_rect.printString();
      FreeImage_Paste(scaled_atlas, resized_color, tex_sub_rect.min_x, tex_sub_rect.min_y, 256);

      colorRegionMap[color] = atlas_rect.region(tex_sub_rect);
      SILOG(textureatlas, detailed,  "Setting uv range to: "); tex_sub_rect.printString();
      colorRegionMap[color].printString();

      FreeImage_Unload(color_texture);      
      FreeImage_Unload(resized_color);

      curRowWidth += tex_width;
      if (tex_height > curRowHeight)
        curRowHeight = tex_height;
      xLoc = curRowWidth;
    } */

    // Generate the final, scaled-to-fit-on-GPU-memory texture atlas
    String atlas_url = uri_dir + file_name + ".atlas.png";
    FreeImage_Save(FIF_PNG, scaled_atlas, atlas_url.c_str());
    FreeImage_Unload(scaled_atlas);

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
        for(uint32 tex_set_idx = 0; tex_set_idx < submesh.texUVs.size(); tex_set_idx++) {
            SubMeshGeometry::TextureSet& tex_set = submesh.texUVs[tex_set_idx];
            std::vector<float> new_uvs = tex_set.uvs;

            // Each prim defines a material mapping, so we need to split the
            // texture coordinates up by prim.
            for(uint32 prim_idx = 0; prim_idx < submesh.primitives.size(); prim_idx++) {
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
                for(uint32 mat_tex_idx = 0; mat_tex_idx < mat.textures.size(); mat_tex_idx++) {
                    MaterialEffectInfo::Texture& real_tex = mat.textures[mat_tex_idx];
                    Region& final_tex_info_region = (real_tex.uri.empty()) ? colorRegionMap[real_tex.color] : tex_info[real_tex.uri].atlas_region;
                    if (tex_info.find(real_tex.uri) == tex_info.end()) continue;
                    bool colorRegion = false;
                    if (real_tex.uri.empty()) { 
                      continue;
                      if (real_tex.affecting != real_tex.DIFFUSE) {
                        continue;
                      }
                      //SILOG(textureatlas, detailed, real_tex.color << " : real_tex.color");
                      //final_tex_info_region.printString();
                      colorRegion = true;
                    }
                    

                    for(uint32 index_idx = 0; index_idx < prim.indices.size(); index_idx++) {
                        int index = prim.indices[index_idx]; 

                        float new_u, new_v;
                        assert(tex_set.stride >= 2);
                        float old_u = tex_set.uvs[index * tex_set.stride];
                        float old_v = tex_set.uvs[index * tex_set.stride + 1];

                        if (colorRegion) {
                           new_u = 0.5*(final_tex_info_region.min_x + final_tex_info_region.max_x);
                           new_v = 0.5*(final_tex_info_region.min_y + final_tex_info_region.max_y);
                        }
                        else {
                          final_tex_info_region.convert(
                              old_u,
                              (1.f-old_v), // inverted v coords
                              &new_u, &new_v);  
                        }
                        new_uvs[ index * tex_set.stride ] = new_u;
                        new_uvs[ index * tex_set.stride + 1 ] = 1.f - new_v; // inverted
                                                                             // v coords
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
            if (!tex_it->uri.empty()) {
                tex_it->uri = atlas_url;
                tex_it->affecting = tex_it->DIFFUSE;
            }
        }
    }

    return md;
}

FilterDataPtr TextureAtlasFilter::apply(FilterDataPtr input) {
    using namespace nvtt;

    InitFreeImage();

    MutableFilterDataPtr output(new FilterData());
    for(FilterData::const_iterator mesh_it = input->begin(); mesh_it != input->end(); mesh_it++) {
        VisualPtr vis = *mesh_it;
        MeshdataPtr mesh( std::tr1::dynamic_pointer_cast<Meshdata>(vis));
        if (!mesh) {
            // Unsupported
            output->push_back(vis);
        }
        else {
            MeshdataPtr ta_mesh = apply(mesh);
            output->push_back(ta_mesh);
        }
    }
    return output;
}

} // namespace Mesh
} // namespace Sirikata
