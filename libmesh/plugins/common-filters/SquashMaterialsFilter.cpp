/*  Sirikata
 *  SquashMaterialsFilter.cpp
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

#include "SquashMaterialsFilter.hpp"
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace Sirikata {
namespace Mesh {

SquashMaterialsFilter::SquashMaterialsFilter(const String& args) {
}

FilterDataPtr SquashMaterialsFilter::apply(FilterDataPtr input) {
    using namespace Sirikata::Transfer;

    assert(input->single());
    MeshdataPtr md = input->get();

    // This keeps track of our new set of unique materials and will replace
    // Meshdata::materials.
    MaterialEffectInfoList new_materials;
    // And this keeps track of how the indices are remapped. This is used to
    // update all material indices to use indices for new_materials.
    typedef std::map<uint32, uint32> MaterialReindexMap;
    MaterialReindexMap reindexes;

    // First, scan through and generate unique materials and the map necessary
    // to map old material indices to new material indices
    for(uint32 mat_idx = 0; mat_idx < md->materials.size(); mat_idx++) {
        MaterialEffectInfo& orig_mat = md->materials[mat_idx];

        // First, try to find an identical material
        bool found_match = false;
        for(uint32 nmi = 0; nmi < new_materials.size(); nmi++) {
            if (orig_mat == new_materials[nmi]) {
                reindexes[mat_idx] = nmi;
                found_match = true;
                break;
            }
        }
        if (found_match) continue;

        // If we can't find one, use this as a new material in the new list
        uint32 new_mat_idx = new_materials.size();
        new_materials.push_back( orig_mat );
        reindexes[mat_idx] = new_mat_idx;
    }

    // Now, replace materials and run through all the data referencing materials
    // and replace indices
    md->materials = new_materials;
    // Materials are referenced in the SubMeshGeometry::Primitive, but those are
    // then bound to actual materials by the
    // GeometryInstance::MaterialBindingMap. To fix up bindings, we just need to
    // remap all the values in all MaterialBindingMaps.
    for(GeometryInstanceList::iterator geo_inst_it = md->instances.begin(); geo_inst_it != md->instances.end(); geo_inst_it++) {
        GeometryInstance& geo_inst = *geo_inst_it;

        for(GeometryInstance::MaterialBindingMap::iterator mat_binding_it = geo_inst.materialBindingMap.begin(); mat_binding_it != geo_inst.materialBindingMap.end(); mat_binding_it++) {
            mat_binding_it->second = reindexes[ mat_binding_it->second ];
        }
    }

    // FIXME we could also try to run through and squash references to materials
    // into a single reference, i.e. so that MaterialBindingMaps don't refer to
    // the same one twice. This is more complicated because of the level of
    // indirection and the fact that multiple GeometryInstances could refer to
    // the same SubMeshGeometry -- we would need to make sure that all
    // GeometryInstances referring to each SubMeshGeometry could squash in the
    // same way.

    // We munged input directly, return same FilterDataPtr
    return input;
}

} // namespace Mesh
} // namespace Sirikata
