/*  Sirikata
 *  SquashInstancedGeometryFilter.cpp
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

#include "SquashInstancedGeometryFilter.hpp"
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include <boost/lexical_cast.hpp>

namespace Sirikata {
namespace Mesh {

namespace {

// An ordered list of materials used to render an object. This is essentially a
// resolved and flattened version of the material binding map.
class MaterialList : public std::vector<uint32> {
public:
    MaterialList(const GeometryInstance::MaterialBindingMap& mbm) {
        // The way mat binding maps works is kind of dumb since they aren't
        // ordered and starting at 0. We need to first get a sorted list of
        // input indices, then sort it, then generate the list of used
        // materials.

        std::vector<uint32> source_ids;
        for(GeometryInstance::MaterialBindingMap::const_iterator mbm_it = mbm.begin(); mbm_it != mbm.end(); mbm_it++)
            source_ids.push_back(mbm_it->first);
        std::sort(source_ids.begin(), source_ids.end());

        for(int i = 0; i < source_ids.size(); i++)
            this->push_back( mbm.find(source_ids[i])->second );
    }

    bool operator<(const MaterialList& rhs) {
        if (size() < rhs.size()) return true;
        for(int i = 0; i < size(); i++) {
            if ((*this)[i] == rhs[i]) continue;
            return ((*this)[i] < rhs[i]);
        }
        return false;
    }
};

}

SquashInstancedGeometryFilter::SquashInstancedGeometryFilter(const String& args) {
}

FilterDataPtr SquashInstancedGeometryFilter::apply(FilterDataPtr input) {
    using namespace Sirikata::Transfer;

    MutableFilterDataPtr output(new FilterData());

    for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        MeshdataPtr md = *md_it;

        // Our basic approach for each Meshdata is to generate one
        // SubMeshGeometry for each set of materials. We scan through all
        // instanced geometry and determine which group it belongs to based on
        // its material. Then, we take the original SubMeshGeometry, apply the
        // necessary transformation, and add the transformed version to the end
        // of the new merged SubMeshGeometry.

        // This will track our new, merged SubMeshGeometries. These are unique
        // based on the materials used to render them.
        typedef std::map<MaterialList, SubMeshGeometry> MergedMeshMap;
        MergedMeshMap merged_meshes;
        // And this will track the new material binding map for each merged
        // SubMeshGeometry
        typedef std::map<MaterialList, GeometryInstance::MaterialBindingMap> MergedMaterialMapMap;
        MergedMaterialMapMap merged_material_maps;

        // This will hold our new, generated geometry. A few fields are copied
        // over directly since they won't change.
        MeshdataPtr new_md(new Meshdata());
        new_md->textures = md->textures;
        //new_md->lights = md->lights;
        new_md->materials = md->materials;
        new_md->uri = md->uri;
        new_md->hash = md->hash;
        new_md->id = md->id;
        //new_md->lightInstances = md->lightInstances;
        // Old globalTransform will have already been applied
        new_md->globalTransform = Matrix4x4f::identity();
        //new_md->joints = md->joints;

        // Scan through all instanced geometry, building up the new models
        Meshdata::GeometryInstanceIterator geoinst_it = md->getGeometryInstanceIterator();
        uint32 geoinst_idx;
        Matrix4x4f pos_xform;
        while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
            GeometryInstance& geo_inst = md->instances[geoinst_idx];
            SubMeshGeometry& geo = md->geometry[ geo_inst.geometryIndex ];
            MaterialList matlist(geo_inst.materialBindingMap);

            if (merged_meshes.find(matlist) == merged_meshes.end()) {
                merged_meshes[matlist] = SubMeshGeometry();
                merged_meshes[matlist].name = "mesh" + boost::lexical_cast<String>(merged_meshes.size()) + "-geometry";

                merged_material_maps[matlist] = GeometryInstance::MaterialBindingMap();
            }
            SubMeshGeometry& merged_mesh = merged_meshes[matlist];
            GeometryInstance::MaterialBindingMap& merged_material_map = merged_material_maps[matlist];

            // Tack transformed info onto the end of this primitive. We need to
            // transform each one according to this instances positioning.
            merged_mesh.append(geo, pos_xform);
            // Add new material mapping info
            for(GeometryInstance::MaterialBindingMap::iterator mm_it = geo_inst.materialBindingMap.begin(); mm_it != geo_inst.materialBindingMap.end(); mm_it++) {
                if ( merged_material_map.find(mm_it->first) == merged_material_map.end())
                    merged_material_map[mm_it->first] = mm_it->second;
                assert(merged_material_map[mm_it->first] == mm_it->second);
            }
        }

        // Create one root node with no transformation to hold these
        // pre-transformed aggregate objects
        Node rn(Matrix4x4f::identity());
        NodeIndex root_node_idx = new_md->nodes.size();
        new_md->nodes.push_back(rn);
        new_md->rootNodes.push_back(root_node_idx);

        // Set up new geometries, instance geometries, and nodes
        for(MergedMeshMap::iterator merged_it = merged_meshes.begin(); merged_it != merged_meshes.end(); merged_it++) {
            SubMeshGeometry& merged_mesh = merged_it->second;
            merged_mesh.recomputeBounds();

            int geo_idx = new_md->geometry.size();
            new_md->geometry.push_back(merged_mesh);

            GeometryInstance geo_inst;
            geo_inst.materialBindingMap = merged_material_maps[merged_it->first];
            geo_inst.geometryIndex = geo_idx;
            geo_inst.parentNode = root_node_idx;
            geo_inst.recomputeBounds(new_md, rn.transform);
            new_md->instances.push_back(geo_inst);
        }

        output->push_back(new_md);
    }

    return output;
}

} // namespace Mesh
} // namespace Sirikata
