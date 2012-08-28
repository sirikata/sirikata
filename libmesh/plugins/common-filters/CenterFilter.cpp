/*  Sirikata
 *  CenterFilter.cpp
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

#include "CenterFilter.hpp"
#include <boost/lexical_cast.hpp>
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/Billboard.hpp>

namespace Sirikata {
namespace Mesh {

Filter* CenterFilter::create(const String& args) {
    return new CenterFilter();
}

FilterDataPtr CenterFilter::apply(FilterDataPtr input) {
    // To apply the transform, we just add a new node and put all root nodes as
    // children of it.

    for(FilterData::const_iterator md_it = input->begin(); md_it != input->end(); md_it++) {
        VisualPtr vis = *md_it;

        MeshdataPtr mesh( std::tr1::dynamic_pointer_cast<Meshdata>(vis) );
        if (mesh) {
            // Compute the bounds
            BoundingBox3f3f bbox = BoundingBox3f3f::null();
            Meshdata::GeometryInstanceIterator geoinst_it = mesh->getGeometryInstanceIterator();
            uint32 geoinst_idx;
            Matrix4x4f pos_xform;
            while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
                BoundingBox3f3f inst_bnds;
                mesh->instances[geoinst_idx].computeTransformedBounds(mesh, pos_xform, &inst_bnds, NULL);
                if (bbox == BoundingBox3f3f::null())
                    bbox = inst_bnds;
                else
                    bbox.mergeIn(inst_bnds);
            }

            Vector3f center = bbox.center();

			Vector3f across = bbox.across() / 2;
			double radius = sqrt(across.x * across.x + across.y * across.y + across.z * across.z);
			double scale = 1.0;
			if(radius != 0) scale = 1.0 / radius;

            // We to add a transformation. We can't just insert a root node because
            // globalTransform might have other adjustments (very importantly,
            // changing which dimension is up). Instead, we shift globalTransform to
            // a new root node and put our transformation in globalTransform.

            Node new_root(NullNodeIndex, mesh->globalTransform);
            new_root.children = mesh->rootNodes;

            NodeIndex new_root_index = mesh->nodes.size();
            mesh->nodes.push_back(new_root);

            mesh->rootNodes.clear();
            mesh->rootNodes.push_back(new_root_index);

            mesh->globalTransform =  Matrix4x4f::translate(-center);

            if ( !mesh->mInstanceControllerTransformList.empty() ) {
                mesh->mInstanceControllerTransformList.insert(mesh->mInstanceControllerTransformList.begin(),
                    mesh->globalTransform);
            }

            continue;
        }

        BillboardPtr bboard( std::tr1::dynamic_pointer_cast<Billboard>(vis) );
        // Billboards should already be centered, noop
        if (bboard) continue;

        SILOG(center-filter, warn, "Unhandled visual type in CenterFilter: " << vis->type() << ". Leaving it alone.");
    }

    return input;
}

} // namespace Mesh
} // namespace Sirikata
