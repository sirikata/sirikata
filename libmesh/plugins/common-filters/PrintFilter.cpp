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
#include <stack>

namespace Sirikata {
namespace Mesh {

PrintFilter::PrintFilter(const String& args) {
	if(args == "textures") {
		mTexturesOnly = true;
	} else {
		mTexturesOnly = false;
	}
}

namespace {
struct NodeState {
    NodeState(NodeIndex idx) : node(idx), curChild(-1) {}

    NodeIndex node;
    int32 curChild;
};

const char* PrimitiveTypeToString(SubMeshGeometry::Primitive::PrimitiveType type) {
    switch (type) {
      case SubMeshGeometry::Primitive::TRIANGLES: return "Triangles"; break;
      case SubMeshGeometry::Primitive::TRISTRIPS: return "Triangle Strips"; break;
      case SubMeshGeometry::Primitive::TRIFANS: return "Triangle Fans"; break;
      case SubMeshGeometry::Primitive::LINESTRIPS: return "LineStrips"; break;
      case SubMeshGeometry::Primitive::LINES: return "Lines"; break;
      case SubMeshGeometry::Primitive::POINTS: return "Points"; break;
      default: return "Unknown"; break;
    }
}

const char* AffectingToString(MaterialEffectInfo::Texture::Affecting type) {
    switch (type) {
      case MaterialEffectInfo::Texture::DIFFUSE: return "Diffuse"; break;
      case MaterialEffectInfo::Texture::SPECULAR: return "Specular"; break;
      case MaterialEffectInfo::Texture::EMISSION: return "Emission"; break;
      case MaterialEffectInfo::Texture::AMBIENT: return "Ambient"; break;
      case MaterialEffectInfo::Texture::REFLECTIVE: return "Reflective"; break;
      case MaterialEffectInfo::Texture::OPACITY: return "Opacity"; break;
      default: return "Unknown"; break;
    }
}

const char* SamplerTypeToString(MaterialEffectInfo::Texture::SamplerType type) {
    switch (type) {
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_UNSPECIFIED: return "Unspecified"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_1D: return "1D"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_2D: return "2D"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_3D: return "3D"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_CUBE: return "Cube"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_RECT: return "Rect"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_DEPTH: return "Depth"; break;
      case MaterialEffectInfo::Texture::SAMPLER_TYPE_STATE: return "State"; break;
      default: return "Unknown"; break;
    }
}

const char* SamplerFilterToString(MaterialEffectInfo::Texture::SamplerFilter type) {
    switch (type) {
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_UNSPECIFIED: return "Unspecified"; break;
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_NONE: return "None"; break;
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_NEAREST: return "Nearest"; break;
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST: return "NearestMipmapNearest"; break;
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST: return "LinearMipmapNearest"; break;
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR: return "NearestMipmapLinear"; break;
      case MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR: return "LinearMipmapLinear"; break;
      default: return "Unknown"; break;
    }
}

const char* WrapModeToString(MaterialEffectInfo::Texture::WrapMode type) {
    switch (type) {
      case MaterialEffectInfo::Texture::WRAP_MODE_UNSPECIFIED: return "Unspecified"; break;
      case MaterialEffectInfo::Texture::WRAP_MODE_NONE: return "None"; break;
      case MaterialEffectInfo::Texture::WRAP_MODE_WRAP: return "Wrap"; break;
      case MaterialEffectInfo::Texture::WRAP_MODE_MIRROR: return "Mirror"; break;
      case MaterialEffectInfo::Texture::WRAP_MODE_CLAMP: return "Clamp"; break;
      case MaterialEffectInfo::Texture::WRAP_MODE_BORDER: return "Border"; break;
      default: return "Unknown"; break;
    }
}

}

FilterDataPtr PrintFilter::apply(FilterDataPtr input) {
    assert(input->single());

    MeshdataPtr md = input->get();

    if(mTexturesOnly) {
        for(TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
            printf("%s\n", it->c_str());
        }
        return input;
    }

    printf("URI: %s\n", md->uri.c_str());
    printf("ID: %ld\n", md->id);
    printf("Hash: %s\n", md->hash.toString().c_str());

    printf("Texture List:\n");
    for(TextureList::const_iterator it = md->textures.begin(); it != md->textures.end(); it++) {
        printf("   %s\n", it->c_str());
    }

    printf("Submesh Geometry List:\n");
    for(SubMeshGeometryList::const_iterator it = md->geometry.begin(); it != md->geometry.end(); it++) {
        printf("   Name: %s, Positions: %d Normals: %d Primitives: %d, UVs: %d (sets) x %d (stride) x %d (count)\n", it->name.c_str(),
            (int)it->positions.size(), (int)it->normals.size(), (int)it->primitives.size(),
            (int)it->texUVs.size(), (int)( it->texUVs.size() ? it->texUVs[0].stride : 0), (int)( it->texUVs.size() ? it->texUVs[0].uvs.size() : 0));

        for(std::vector<SubMeshGeometry::Primitive>::const_iterator p = it->primitives.begin(); p != it->primitives.end(); p++) {
            printf("      Primitive: material: %d, indices: %d, type: %s\n", (int)p->materialId, (int)p->indices.size(), PrimitiveTypeToString(p->primitiveType));
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
            printf("     Texture: %s, color = %s, affects %s, %s, min: %s, mag: %s, wrap = (%s, %s, %s), max_mip = %d, mip_bias = %f\n",
                t_it->uri.c_str(),
                t_it->color.toString().c_str(),
                AffectingToString(t_it->affecting),
                SamplerTypeToString(t_it->samplerType),
                SamplerFilterToString(t_it->minFilter),
                SamplerFilterToString(t_it->magFilter),
                WrapModeToString(t_it->wrapS),
                WrapModeToString(t_it->wrapT),
                WrapModeToString(t_it->wrapU),
                t_it->maxMipLevel,
                t_it->mipBias
            );
    }

    printf("Geometry Instances: (%d in list, %d instanced)\n", (int)md->instances.size(), (int)md->getInstancedGeometryCount());
    for(GeometryInstanceList::const_iterator it = md->instances.begin(); it != md->instances.end(); it++) {
        printf("   Index: %d MapSize: %d\n", it->geometryIndex, (int)it->materialBindingMap.size());
        for(GeometryInstance::MaterialBindingMap::const_iterator m = it->materialBindingMap.begin(); m != it->materialBindingMap.end(); m++) {
            printf("      map from: %d to: %d\n", (int)m->first, (int)m->second);
        }
    }

    printf("Light Instances: (%d in list, %d instanced)\n", (int)md->lightInstances.size(), (int)md->getInstancedLightCount());
    for(LightInstanceList::const_iterator it = md->lightInstances.begin(); it != md->lightInstances.end(); it++) {
        printf("   Index: %d Matrix: %s\n", it->lightIndex, md->getTransform(it->parentNode).toString().c_str());
    }

    printf("Material Effect size: %d\n", (int)md->materials.size());

    printf("Nodes size: %d (%d roots)\n", (int)md->nodes.size(), (int)md->rootNodes.size());
    for(uint32 ri = 0; ri < md->rootNodes.size(); ri++) {
        std::stack<NodeState> node_stack;
        node_stack.push( NodeState(md->rootNodes[ri]) );
        String indent = "";

        while(!node_stack.empty()) {
            NodeState& curnode = node_stack.top();

            if (curnode.curChild == -1) {
                // First time we've seen this node, print info and move it
                // forward to start procesing children
                printf("%s %d\n", indent.c_str(), curnode.node);
                curnode.curChild++;
                indent += " ";
            }
            else if (curnode.curChild >= (int)md->nodes[curnode.node].children.size()) {
                // We finished with this node
                node_stack.pop();
                indent = indent.substr(1); // reduce indent
            }
            else {
                // Normal iteration, process next child
                int32 childindex = curnode.curChild;
                curnode.curChild++;
                node_stack.push( NodeState(md->nodes[curnode.node].children[childindex]) );
            }
        }
    }

    // Compute the expected number of draw calls assuming no smart
    // transformation is occuring. This should be:
    // Number of instances * number of primitives in instance
    // This really should trace from the root to make sure that all instances
    // are actually drawn...
    uint32 draw_calls = 0;

    Meshdata::GeometryInstanceIterator geoinst_it = md->getGeometryInstanceIterator();
    uint32 geoinst_idx;
    Matrix4x4f pos_xform;
    while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
        draw_calls += md->geometry[ md->instances[geoinst_idx].geometryIndex ].primitives.size();
    }
    printf("Estimated draw calls: %d\n", draw_calls);

    return input;
}

}
}
