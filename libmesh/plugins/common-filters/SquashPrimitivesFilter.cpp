/*  Sirikata
 *  SquashPrimitivesFilter.cpp
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

#include "SquashPrimitivesFilter.hpp"
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace Sirikata {
namespace Mesh {

SquashPrimitivesFilter::SquashPrimitivesFilter(const String& args) {
}

FilterDataPtr SquashPrimitivesFilter::apply(FilterDataPtr input) {
    using namespace Sirikata::Transfer;

    assert(input->single());
    MeshdataPtr md = input->get();

    // We need to look at each SubMeshGeometry and decide if its constituent
    // primitives can be squashed. Currently we require the following:
    //  1. All primitives will be squashed into a single new primitive. (Could
    //     be loosened).
    //  2. We only support squashing triangles, points, and lines. (Could be
    //     loosened.)
    //  3. For each GeometryInstance referring to the SubMeshGeometry, all
    //     primitives (in each squashed group) must use the same material so the
    //     MaterialBindingMaps can be rewritten.
    for(uint32 geo_idx = 0; geo_idx < md->geometry.size(); geo_idx++) {
        SubMeshGeometry& submesh = md->geometry[geo_idx];
        if (submesh.primitives.size() < 2) continue;

        // First check that all primitives are triangles
        bool valid_squash = true;
        SubMeshGeometry::Primitive::PrimitiveType combined_type = (SubMeshGeometry::Primitive::PrimitiveType)-1;
        for(uint32 prim_idx = 0; prim_idx < submesh.primitives.size(); prim_idx++) {
            SubMeshGeometry::Primitive& prim = submesh.primitives[prim_idx];
            if (combined_type == -1) {
                combined_type = prim.primitiveType;
                if (prim.primitiveType != SubMeshGeometry::Primitive::TRIANGLES &&
                    prim.primitiveType != SubMeshGeometry::Primitive::LINES &&
                    prim.primitiveType != SubMeshGeometry::Primitive::POINTS)
                {
                    valid_squash = false;
                    break;
                }
            }
            if (prim.primitiveType != combined_type) {
                valid_squash = false;
                break;
            }
        }
        if (!valid_squash) continue;

        // Next check that each GeometryInstance uses the same material for all Primitives
        for(uint32 geo_inst_idx = 0; geo_inst_idx < md->instances.size(); geo_inst_idx++) {
            GeometryInstance& geo_inst = md->instances[geo_inst_idx];
            if (geo_inst.geometryIndex != geo_idx) continue;

            int32 matched_mat_idx = -1;
            for(GeometryInstance::MaterialBindingMap::iterator mat_binding_it = geo_inst.materialBindingMap.begin(); mat_binding_it != geo_inst.materialBindingMap.end(); mat_binding_it++) {
                if (matched_mat_idx == -1) {
                    matched_mat_idx = mat_binding_it->second;
                }
                else if (matched_mat_idx != mat_binding_it->second) {
                    valid_squash = false;
                    break;
                }
            }
            if (!valid_squash) break;
        }
        if (!valid_squash) continue;


        // We have a valid squash. Since most of the data is already merged, we
        // really just need to make one big indices array and fix up material
        // references and MaterialBindingMaps.

        // Make the big index list / one big Primitive
        SubMeshGeometry::Primitive combined_primitive;
        combined_primitive.materialId = 0; // Only one material in squashed version
        combined_primitive.primitiveType = combined_type;
        for(uint32 prim_idx = 0; prim_idx < submesh.primitives.size(); prim_idx++) {
            SubMeshGeometry::Primitive& prim = submesh.primitives[prim_idx];
            combined_primitive.indices.insert(combined_primitive.indices.end(), prim.indices.begin(), prim.indices.end());
        }
        submesh.primitives.clear();
        submesh.primitives.push_back(combined_primitive);

        // Fix up each GeometryInstance. It used to map many materialIDs to a
        // single materialID. We need to squash this to just have 0 ->
        // singelMaterialID.
        for(uint32 geo_inst_idx = 0; geo_inst_idx < md->instances.size(); geo_inst_idx++) {
            GeometryInstance& geo_inst = md->instances[geo_inst_idx];
            assert(!geo_inst.materialBindingMap.empty());
            // We should only need a single material ID for squashed primitives.
            uint32 single_mat_id = geo_inst.materialBindingMap.begin()->second;
            geo_inst.materialBindingMap.clear();
            geo_inst.materialBindingMap[0] = single_mat_id;
        }
    }

    return input;
}

} // namespace Mesh
} // namespace Sirikata
