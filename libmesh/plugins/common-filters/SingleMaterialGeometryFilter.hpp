/*  Sirikata
 *  SingleMaterialGeometryFilter.hpp
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

#ifndef _SIRIKATA_MESH_COMMON_FILTERS_SINGLE_MATERIAL_GEOMETRY_HPP_
#define _SIRIKATA_MESH_COMMON_FILTERS_SINGLE_MATERIAL_GEOMETRY_HPP_

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

/** Splits any SubMeshGeometries (and corresponding GeometryInstances) into
 *  multiple parts, such that each SubMeshGeometry only has a single material
 *  (and the GeometryInstance has only 1 entry in its MaterialBindingMap).  This
 *  structure is a prerequisite for some other filters to be be successfully
 *  applied, e.g. material reduction.
 *
 *  Note that this comes at the cost of duplicating data if a SubMeshGeometry
 *  is split. In many cases, this cost can be recovered by also applying a
 *  filter to remove unused values from the SubMeshGeometry since many times
 *  indices in the SubMeshGeometry::Primitives do not overlap.
 */
class SingleMaterialGeometryFilter : public Filter {
public:
    static Filter* create(const String& args) { return new SingleMaterialGeometryFilter(args); }

    SingleMaterialGeometryFilter(const String& args);
    virtual ~SingleMaterialGeometryFilter() {}

    virtual FilterDataPtr apply(FilterDataPtr input);
}; // class SingleMaterialGeometryFilter

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_COMMON_FILTERS_SINGLE_MATERIAL_GEOMETRY_HPP_
