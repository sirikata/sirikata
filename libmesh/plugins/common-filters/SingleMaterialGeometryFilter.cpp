/*  Sirikata
 *  SingleMaterialGeometryFilter.cpp
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

#include "SingleMaterialGeometryFilter.hpp"
#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {
namespace Mesh {

SingleMaterialGeometryFilter::SingleMaterialGeometryFilter(const String& args) {
}

namespace {

/** Clone this SubMeshGeometry, taking only a subset of its elements. Because
 *  copying requires duplicating the underlying data, this also removes
 *  now-unused data from the underlying vertex data in the copy. NOTE: Currently
 *  only supports a single material, i.e. all copied primitives should use the
 *  same material. Also, it *doesn't* recompute bounds.
 *
 *  \param prims the indices of the primitives to keep
 *  \param output a bare SubMeshGeometry to output to
 */
// This would be nice to put into SubMeshGeometry directly, but right now it
// doesn't handle all cases well enough to do that -- multiple materials,
// animations, etc.
void copySubset(const SubMeshGeometry& orig, const std::vector<int>& prims, SubMeshGeometry* output) {
    // Make sure we have a fresh start
    *output = SubMeshGeometry();
    output->name = orig.name;
    // Initialize texUVs for new mesh. Faster to initialize once here than check
    // every time through one of the data copying loops belo
    for(std::vector<SubMeshGeometry::TextureSet>::const_iterator tex_set_it = orig.texUVs.begin(); tex_set_it != orig.texUVs.end(); tex_set_it++) {
        output->texUVs.push_back(SubMeshGeometry::TextureSet());
        output->texUVs.back().stride = tex_set_it->stride;
    }

    // Keep track of which data is referenced
    std::vector<bool> referenced_indices(orig.positions.size(), false);

    // Iterate through all copied prims, marking indices as used
    for(std::vector<int>::const_iterator prim_it = prims.begin(); prim_it != prims.end(); prim_it++) {
        const SubMeshGeometry::Primitive& prim = orig.primitives[*prim_it];
        for(std::vector<unsigned short>::const_iterator ind_it = prim.indices.begin(); ind_it != prim.indices.end(); ind_it++)
            referenced_indices[*ind_it] = true;
    }

    // With them all marked, form a map to translate from old indices -> new
    // indices. Copy data over as we go.
    std::map<unsigned short, unsigned short> index_map;
    uint32 new_index_source = 0;
    for(int orig_index = 0; orig_index < (int)referenced_indices.size(); orig_index++) {
        if (referenced_indices[orig_index] == false) continue;
        index_map[orig_index] = new_index_source;
        new_index_source++;

        output->positions.push_back( orig.positions[orig_index] );
        if ((int)orig.normals.size() > orig_index)
            output->normals.push_back( orig.normals[orig_index] );
        if ((int)orig.tangents.size() > orig_index)
            output->tangents.push_back( orig.tangents[orig_index] );
        if ((int)orig.colors.size() > orig_index)
            output->colors.push_back( orig.colors[orig_index] );
        for(int tex_idx = 0; tex_idx < (int)orig.texUVs.size(); tex_idx++) {
            for(int stride_i = 0; stride_i < (int)orig.texUVs[tex_idx].stride; stride_i++)
                output->texUVs[tex_idx].uvs.push_back(
                    orig.texUVs[tex_idx].uvs[orig_index*orig.texUVs[tex_idx].stride + stride_i]
                );
        }
    }

    // Copy primitives over, adjusting indices
    for(std::vector<int>::const_iterator prim_it = prims.begin(); prim_it != prims.end(); prim_it++) {
        output->primitives.push_back(SubMeshGeometry::Primitive());
        const SubMeshGeometry::Primitive& orig_prim = orig.primitives[*prim_it];
        SubMeshGeometry::Primitive& new_prim = output->primitives.back();
        new_prim.primitiveType = orig_prim.primitiveType;
        // NOTE: This is a special case for the transformation we want -- there
        // should only be one material so they all get mapped to 0.
        new_prim.materialId = 0;
        // Resize appropriately and copy new indices
        new_prim.indices.resize(orig_prim.indices.size());
        for(int ind = 0; ind < (int)orig_prim.indices.size(); ind++)
            new_prim.indices[ind] = index_map[ orig_prim.indices[ind] ];
    }

    // Copy animations
    for(SkinControllerList::const_iterator skin_it = orig.skinControllers.begin(); skin_it != orig.skinControllers.end(); skin_it++) {
        const SkinController& orig_skin = *skin_it;
        output->skinControllers.push_back(SkinController());
        SkinController& new_skin = output->skinControllers.back();

        // Some things aren't affected by the vertex data reduction
        new_skin.joints = orig_skin.joints;
        new_skin.bindShapeMatrix = orig_skin.bindShapeMatrix;
        new_skin.inverseBindMatrices = orig_skin.inverseBindMatrices;

        // weightStartIndices, weights, and jointIndices need to be
        // decimated based on the reduced set of vertices
        for(int orig_vert_idx = 0; orig_vert_idx < (int)referenced_indices.size(); orig_vert_idx++) {
            if (referenced_indices[orig_vert_idx] == false) continue;

            new_skin.weightStartIndices.push_back( new_skin.weights.size() );
            for(int influence_idx = orig_skin.weightStartIndices[orig_vert_idx];
                influence_idx < (int)orig_skin.weightStartIndices[orig_vert_idx+1];
                influence_idx++) {
                new_skin.weights.push_back( orig_skin.weights[influence_idx] );
                new_skin.jointIndices.push_back( orig_skin.jointIndices[influence_idx] );
            }
        }
        // One last entry in weightStartIndices because the extra
        // entry allows simple subtraction to determine #influences
        new_skin.weightStartIndices.push_back( new_skin.weights.size() );
    }
}

} // namespace

FilterDataPtr SingleMaterialGeometryFilter::apply(FilterDataPtr input) {
    for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        VisualPtr vis = *md_it;
        // Meshdata only currently
        MeshdataPtr md( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
        if (!md) continue;

        // Our approach is to run through all SubMeshGeometries generating
        // versions that are split. By maintaining a map from old indices to a
        // list of new indices, we can then run through all GeometryInstances,
        // duplicating them for each part of the original instance.

        // This map stores geometryIndex -> list of new geometry indices,
        // allowing the old instances to be split into a set of new instances
        typedef std::map<uint32, std::vector<uint32> > OldToNewIndexMap;
        OldToNewIndexMap new_geo_indices;

        // Our output should be a new set of SubMeshGeometries and a new set of
        // GeometryInstances referring to them.
        SubMeshGeometryList new_geometry;
        GeometryInstanceList new_instances;

        for(uint32 geo_idx = 0; geo_idx < md->geometry.size(); geo_idx++) {
            SubMeshGeometry& geo = md->geometry[geo_idx];

            // This mesh needs to be split into several new meshes, by
            // material. First, collect the primitives to be put under the copy
            // for each material.
            typedef std::map<SubMeshGeometry::Primitive::MaterialId, std::vector<int> > SingleMatPrimSetMap;
            SingleMatPrimSetMap new_meshes_prims;
            for(uint32 pi = 0; pi < geo.primitives.size(); pi++) {
                SubMeshGeometry::Primitive& prim = geo.primitives[pi];
                // Add this primitive
                new_meshes_prims[prim.materialId].push_back(pi);
            }

            // Then copy the submesh, using only the primitives for each item.
            typedef std::map<SubMeshGeometry::Primitive::MaterialId, SubMeshGeometry> SingleMatMeshMap;
            SingleMatMeshMap new_meshes;
            for(SingleMatPrimSetMap::iterator mat_it = new_meshes_prims.begin(); mat_it != new_meshes_prims.end(); mat_it++)
                copySubset(geo, mat_it->second, &new_meshes[mat_it->first]);
            new_meshes_prims.clear();

            // Recompute summary info for these new submeshes, add them to the
            // new set, and store their indices for use by the instance
            // geometries
            typedef std::map<SubMeshGeometry::Primitive::MaterialId, uint32> NewSubMeshIndexMap;
            NewSubMeshIndexMap new_indices_by_material;
            for(SingleMatMeshMap::iterator nm_it = new_meshes.begin(); nm_it != new_meshes.end(); nm_it++) {
                nm_it->second.recomputeBounds();
                uint32 new_index = new_geometry.size();
                new_geometry.push_back(nm_it->second);
                new_indices_by_material[nm_it->first] = new_index;
            }

            // Now, run through and find GeometryInstances which are using the
            // original mesh and make new ones using all the split ones
            for(uint32 ii = 0; ii < md->instances.size(); ii++) {
                GeometryInstance& geo_inst = md->instances[ii];
                if (geo_inst.geometryIndex != geo_idx) continue;

                for(NewSubMeshIndexMap::iterator sm_it = new_indices_by_material.begin(); sm_it != new_indices_by_material.end(); sm_it++) {
                    new_instances.push_back(geo_inst);
                    // Point to new index
                    new_instances.back().geometryIndex = sm_it->second;
                    // Reset material binding map, using only the material bound
                    // for this material from the earlier version.
                    new_instances.back().materialBindingMap.clear();
                    new_instances.back().materialBindingMap[0] = geo_inst.materialBindingMap[sm_it->first];
                }
            }
        }

        // After all the translation, just swapping out the list of
        // SubMeshGeometries and GeometryInstances should be safe
        md->geometry = new_geometry;
        md->instances = new_instances;
    }

    return input;
}

} // namespace Mesh
} // namespace Sirikata
