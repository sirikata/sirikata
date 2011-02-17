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
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace Sirikata {
namespace Mesh {

SingleMaterialGeometryFilter::SingleMaterialGeometryFilter(const String& args) {
}

FilterDataPtr SingleMaterialGeometryFilter::apply(FilterDataPtr input) {
    using namespace Sirikata::Transfer;

    for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        MeshdataPtr md = *md_it;

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

            // This mesh needs to be split into several new meshes, by material.
            typedef std::map<SubMeshGeometry::Primitive::MaterialId, SubMeshGeometry> SingleMatMeshMap;
            SingleMatMeshMap new_meshes;
            for(uint32 pi = 0; pi < geo.primitives.size(); pi++) {
                SubMeshGeometry::Primitive& prim = geo.primitives[pi];
                // Create new submesh if there isn't one for this material id yet
                if (new_meshes.find(prim.materialId) == new_meshes.end()) {
                    // Copy all data
                    new_meshes[prim.materialId] = geo;
                    // And clear the prim list
                    new_meshes[prim.materialId].primitives.clear();
                }
                // Add this primitive
                new_meshes[prim.materialId].primitives.push_back(prim);
                // Reset material id -- all should be 0 since all will only have
                // 1 material id
                new_meshes[prim.materialId].primitives.back().materialId = 0;
            }
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
