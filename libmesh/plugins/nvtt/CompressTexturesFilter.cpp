/*  Sirikata
 *  CompressTexturesFilter.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include "CompressTexturesFilter.hpp"
#include "FreeImage.hpp"
#include <nvtt/nvtt.h>

static bool freeimage_initialized = false;

namespace Sirikata {
namespace Mesh {

CompressTexturesFilter::CompressTexturesFilter(const String& args) {
}

FilterDataPtr CompressTexturesFilter::apply(FilterDataPtr input) {
    using namespace nvtt;

    InitFreeImage();

    for(FilterData::const_iterator mesh_it = input->begin(); mesh_it != input->end(); mesh_it++) {
        MeshdataPtr mesh = *mesh_it;

        // We only know how to handle local files
        if (mesh->uri.size() < 7 || mesh->uri.substr(0, 7) != "file://") continue;
        String uri_dir = mesh->uri.substr(7);
        uri_dir = uri_dir.substr(0, uri_dir.rfind("/")+1);

        for(TextureList::iterator tex_it = mesh->textures.begin(); tex_it != mesh->textures.end(); tex_it++) {
            String relative_texfile = *tex_it;
            String new_relative_texfile = relative_texfile + ".dds";
            String texfile = uri_dir + relative_texfile;
            String new_texfile = uri_dir + new_relative_texfile;

            // Load the data from the original file
            FIBITMAP* input_image = GenericLoader(texfile.c_str(), 0);
            if (input_image == NULL) // Couldn't figure out how to load it
                continue;

            // Make sure we get the data in BGRA 8UB and properly packed
            FIBITMAP* input_image_32 = FreeImage_ConvertTo32Bits(input_image);

            int32 width = FreeImage_GetWidth(input_image_32);
            int32 height = FreeImage_GetHeight(input_image_32);

            // Unfortunately, FreeImage chose just about the worst in memory
            // layout, and doesn't provide any flexibility. Convert to packed
            // BGRA which is all nvtt supports.
            uint8* data = new uint8[width*height*4];
            {
                for(int32 y = 0; y < height; y++) {
                    BYTE *bits = FreeImage_GetScanLine(input_image_32, y);
                    for(int32 x = 0; x < width; x++) {
                        // Set pixel color to
                        data[((height-y-1)*width + x)*4 + 0] = bits[FI_RGBA_BLUE];
                        data[((height-y-1)*width + x)*4 + 1] = bits[FI_RGBA_GREEN];
                        data[((height-y-1)*width + x)*4 + 2] = bits[FI_RGBA_RED];
                        data[((height-y-1)*width + x)*4 + 3] = bits[FI_RGBA_ALPHA];
                        bits += 4;
                    }
                }
            }

            // Get it into
            InputOptions inputOptions;
            inputOptions.setTextureLayout(TextureType_2D, width, height);
            inputOptions.setMipmapData(data, width, height);

            // Setup output
            OutputOptions outputOptions;
            outputOptions.setFileName(new_texfile.c_str());
            CompressionOptions compressionOptions;
            compressionOptions.setFormat(Format_DXT1);

            // Compress
            Compressor compressor;
            compressor.process(inputOptions, compressionOptions, outputOptions);

            // And replace the texture
            *tex_it = new_relative_texfile;
            for(MaterialEffectInfoList::iterator mat_it = mesh->materials.begin(); mat_it != mesh->materials.end(); mat_it++) {
                for(MaterialEffectInfo::TextureList::iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++) {
                    if (tex_it->uri == relative_texfile)
                        tex_it->uri = new_relative_texfile;
                }
            }

            // Cleanup
            FreeImage_Unload(input_image);
            FreeImage_Unload(input_image_32);
            delete data;
        }
    }

    return input;
}

} // namespace Mesh
} // namespace Sirikata
