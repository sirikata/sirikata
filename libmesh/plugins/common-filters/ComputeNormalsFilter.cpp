// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ComputeNormalsFilter.hpp"
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/Billboard.hpp>

namespace Sirikata {
namespace Mesh {

Filter* ComputeNormalsFilter::create(const String& args) {
    return new ComputeNormalsFilter();
}

FilterDataPtr ComputeNormalsFilter::apply(FilterDataPtr input) {

    for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        VisualPtr vis = *md_it;

        MeshdataPtr mesh( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
        if (mesh) {

            for(SubMeshGeometryList::iterator sm_it = mesh->geometry.begin(); sm_it != mesh->geometry.end(); sm_it++) {
                SubMeshGeometry& submesh = *sm_it;
                uint32 non_zero_normals = submesh.normals.size() > 0;
                if (submesh.normals.size() == submesh.positions.size()) continue;

                // If we don't have matching normals, clear & compute
                submesh.normals.clear();

                submesh.normals.resize(submesh.positions.size(), Vector3f(0, 0, 0));

                for(std::vector<SubMeshGeometry::Primitive>::iterator prim_it = submesh.primitives.begin(); prim_it != submesh.primitives.end(); prim_it++) {
                    // Lines and points don't need normals
                    if (prim_it->primitiveType == SubMeshGeometry::Primitive::POINTS ||
                        prim_it->primitiveType == SubMeshGeometry::Primitive::LINES ||
                        prim_it->primitiveType == SubMeshGeometry::Primitive::LINESTRIPS) {
                        if (non_zero_normals)
                            SILOG(compute-normals-filter, warn, "Found mismatching number of normals and positions for points, lines, or linestrips.  Model is probably broken.");
                        continue;
                    }

                    if (prim_it->primitiveType != SubMeshGeometry::Primitive::TRIANGLES) {
                        SILOG(compute-normals-filter, warn, "Tried to apply ComputeNormalsFilter to non-triangles primitive.");
                        continue;
                    }

                    // Compute triangle normals, add to each vertex
                    for(uint32 tri_idx = 0; tri_idx != prim_it->indices.size()/3; tri_idx++) {
                        Vector3f leg_a = submesh.positions[prim_it->indices[tri_idx*3+1]] - submesh.positions[prim_it->indices[tri_idx*3+0]];
                        Vector3f leg_b = submesh.positions[prim_it->indices[tri_idx*3+2]] - submesh.positions[prim_it->indices[tri_idx*3+0]];

                        Vector3f nrm = leg_a.cross(leg_b).normal();

                        // Add the normal to each of three vertices
                        submesh.normals[prim_it->indices[tri_idx*3+0]] += nrm;
                        submesh.normals[prim_it->indices[tri_idx*3+1]] += nrm;
                        submesh.normals[prim_it->indices[tri_idx*3+2]] += nrm;
                    }
                }

                // Normalize each normal
                for(uint32 vidx = 0; vidx < submesh.normals.size(); vidx++)
                    if (submesh.normals[vidx] != Vector3f(0,0,0)) submesh.normals[vidx].normalizeThis();
            }

            // Since we processed as a mesh, no need to check other types
            continue;
        }

        BillboardPtr bboard( std::tr1::dynamic_pointer_cast<Billboard>(vis) );
        // Can't compute normals for billboards...
        if (bboard) continue;

        SILOG(compute-normals-filter, warn, "Unhandled visual type in ComputeNormalsFilter: " << vis->type() << ". Leaving it alone.");
    }

    return input;
}

} // namespace Mesh
} // namespace Sirikata
