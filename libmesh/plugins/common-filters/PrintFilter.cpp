/*  Sirikata
 *  PrintFilter.cpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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

#include "PrintFilter.hpp"

namespace Sirikata {
namespace Mesh {

PrintFilter::PrintFilter(const String& args) {

}

FilterDataPtr PrintFilter::apply(FilterDataPtr input) {
    assert(input->single());

    MeshdataPtr md = input->get();

    printf("URI: %s\n", md->uri.c_str());
    printf("ID: %ld\n", md->id);
    printf("Hash: %s\n", md->hash.toString().c_str());

    printf("Texture List:\n");
    for(Meshdata::TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
        printf("   %s\n", it->c_str());
    }

    printf("Submesh Geometry List:\n");
    for(Meshdata::SubMeshGeometryList::const_iterator it = md->geometry.begin(); it != md->geometry.end(); it++) {
        printf("   Name: %s, Positions: %d\n", it->name.c_str(), (int)it->positions.size());
    }

    printf("URI Map:\n");
    for(Meshdata::URIMap::const_iterator it = md->textureMap.begin(); it != md->textureMap.end(); it++) {
        printf("   From: %s To: %s\n", it->first.c_str(), it->second.c_str());
    }

    printf("Lights:\n");
    for(Meshdata::LightInfoList::const_iterator it = md->lights.begin(); it != md->lights.end(); it++) {
        printf("   Type: %d Power: %f\n", it->mType, it->mPower);
    }

    printf("Material Effects:\n");
    for(Meshdata::MaterialEffectInfoList::const_iterator it = md->materials.begin(); it != md->materials.end(); it++) {
        printf("   Textures: %d Shininess: %f Reflectivity: %f\n", (int)it->textures.size(), it->shininess, it->reflectivity);
    }

    printf("Geometry Instances:\n");
    for(Meshdata::GeometryInstanceList::const_iterator it = md->instances.begin(); it != md->instances.end(); it++) {
        printf("   Index: %d Radius: %f\n", it->geometryIndex, it->radius);
    }

    printf("Light Instances:\n");
    for(Meshdata::LightInstanceList::const_iterator it = md->lightInstances.begin(); it != md->lightInstances.end(); it++) {
        printf("   Index: %d Matrix: %s\n", it->lightIndex, it->transform.toString().c_str());
    }

    return input;
}

}
}
