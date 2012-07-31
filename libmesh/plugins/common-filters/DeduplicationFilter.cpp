/*  Sirikata
 *  DeduplicationFilter.cpp
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

#include "DeduplicationFilter.hpp"
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/Billboard.hpp>

namespace Sirikata {
namespace Mesh {

Filter* DeduplicationFilter::create(const String& args) {
    return new DeduplicationFilter();
}

bool comp(Vector3f point1, Vector3f point2) {
	if(point1.x == point2.x) {
		if(point1.y == point2.y) {
			return point1.z < point2.z;
		} else return point1.y < point2.y;
	} else return point1.x < point2.x;
}

FilterDataPtr DeduplicationFilter::apply(FilterDataPtr input) {
	for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        VisualPtr vis = *md_it;

        MeshdataPtr mesh( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
        if (mesh) {
			//mixing smg's and increasing # of primitives
			//general idea: we take two geometries, sort points, see if they are equal (because they should perfectly line up!)
			//genearlize - should it do partial deduplication? if two meshes are only somewhat similar?
			//look at containers - efficiency
			//unordered set? unordered map? 
			if(mesh->geometry.size() >= 2) {
				std::vector<std::vector<Vector3f> > sortedPositions;
				for(int i = 0; i < mesh->geometry.size(); i++) {
					std::vector<Vector3f> v = mesh->geometry[i].positions;
					std::sort(v.begin(), v.begin() + v.size(), comp);
					sortedPositions.push_back(v);
				}
				for(int i = 0; i < mesh->geometry.size(); i++) {
					for(int j = i + 1; j < mesh->geometry.size(); j++) {
						bool same = true;
						if(sortedPositions[i].size() == sortedPositions[j].size()) {
							for(int k = 0; k < sortedPositions[i].size(); k++) {
								if(sortedPositions[i][i] != sortedPositions[j][i]) same = false;
							}
						} else same = false;
						if(same) {
							int add = mesh->geometry[i].positions.size();

							//if same, we make a new primitive
							Mesh::SubMeshGeometry::Primitive p;
							p.primitiveType = mesh->geometry[i].primitives[0].primitiveType;
							mesh->geometry[i].primitives.push_back(p);

							for(int k = 0; k < mesh->geometry[j].primitives[0].indices.size(); k++)
								mesh->geometry[i].primitives[mesh->geometry[i].primitives.size() - 1].indices.push_back(mesh->geometry[j].primitives[0].indices[k] + add);
							for(int k = 0; k < mesh->geometry[j].positions.size(); k++)
								mesh->geometry[i].positions.push_back(mesh->geometry[j].positions[k]);
							//materials 
							mesh->geometry[i].primitives[mesh->geometry[i].primitives.size() - 1].materialId = mesh->instances[i].materialBindingMap.size() + 1;

							//submeshgeometry
							mesh->geometry.erase(mesh->geometry.begin() + j);
							
							//geometryinstance
							mesh->instances[i].materialBindingMap[mesh->instances[i].materialBindingMap.size() + 1] = mesh->instances[j].materialBindingMap[1];

							mesh->instances.erase(mesh->instances.begin() + j);
							for(int k = j; k < mesh->instances.size(); k++)
								mesh->instances[k].geometryIndex = k;

							//sortedPositions!
							sortedPositions.erase(sortedPositions.begin() + j);
							

						}
					}
				}
			}
			continue;
		}

		BillboardPtr bboard( std::tr1::dynamic_pointer_cast<Billboard>(vis) );
        // Won't work for billboards...
        if (bboard) continue;
        SILOG(deduplication-filter, warn, "Unhandled visual type in DeduplicationFilter: " << vis->type() << ". Leaving it alone.");
	}

    return input;
}

} // namespace Mesh
} // namespace Sirikata
