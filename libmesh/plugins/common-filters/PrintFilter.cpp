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
    for(TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
        printf("   %s\n", it->c_str());
    }

    printf("Submesh Geometry List:\n");
    for(SubMeshGeometryList::const_iterator it = md->geometry.begin(); it != md->geometry.end(); it++) {
        printf("   Name: %s, Positions: %d Normals: %d Primitives: %d\n", it->name.c_str(),
                (int)it->positions.size(), (int)it->normals.size(), (int)it->primitives.size());

        for(std::vector<SubMeshGeometry::Primitive>::const_iterator p = it->primitives.begin(); p != it->primitives.end(); p++) {
            printf("      Primitive id: %d, indices: %d\n", (int)p->materialId, (int)p->indices.size());
        }
    }

    printf("Lights:\n");
    for(LightInfoList::const_iterator it = md->lights.begin(); it != md->lights.end(); it++) {
        printf("   Type: %d Power: %f\n", it->mType, it->mPower);
    }

    printf("Material Effects:\n");
    for(MaterialEffectInfoList::const_iterator it = md->materials.begin(); it != md->materials.end(); it++) {
        printf("   Textures: %d Shininess: %f Reflectivity: %f\n", (int)it->textures.size(), it->shininess, it->reflectivity);
        for(MaterialEffectInfo::TextureList::const_iterator t_it = it->textures.begin(); t_it != it->textures.end(); t_it++)
            printf("     Texture: %s\n", t_it->uri.c_str());
    }

    printf("Geometry Instances:\n");
    for(GeometryInstanceList::const_iterator it = md->instances.begin(); it != md->instances.end(); it++) {
        printf("   Index: %d Radius: %f MapSize: %d\n", it->geometryIndex, it->radius, it->materialBindingMap.size());
        for(GeometryInstance::MaterialBindingMap::const_iterator m = it->materialBindingMap.begin(); m != it->materialBindingMap.end(); m++) {
            printf("      map from: %d to: %d\n", (int)m->first, (int)m->second);
        }
    }

    printf("Light Instances:\n");
    for(LightInstanceList::const_iterator it = md->lightInstances.begin(); it != md->lightInstances.end(); it++) {
        printf("   Index: %d Matrix: %s\n", it->lightIndex, md->getTransform(it->parentNode).toString().c_str());
    }

    printf("Material Effect size: %d\n", (int)md->materials.size());

    return input;
}

}
}
