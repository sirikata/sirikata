/*  Sirikata
 *  SquashInstancedGeometryFilter.hpp
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

#ifndef _SIRIKATA_MESH_COMMON_FILTERS_SQUASH_INSTANCED_GEOMETRY_HPP_
#define _SIRIKATA_MESH_COMMON_FILTERS_SQUASH_INSTANCED_GEOMETRY_HPP_

#include <sirikata/mesh/Filter.hpp>

namespace Sirikata {
namespace Mesh {

/** Takes all instanced geometry in a mesh and squashes it to as few
 *  SubMeshGeometry/InstanceGeometries as possible. In the
 *  ideal case, all SubMeshGeometries use the same single material,
 *  resulting in 1 material, 1 SubMeshGeometry with 1 Primitive, and 1
 *  InstanceGeometry referring to it, resulting in only 1 draw call.
 *  Note that this causes all useful information about components in
 *  the model to be lost -- they all appear as a single, unified
 *  object -- so its mainly useful for generating an efficient model
 *  for display.
 */
class SquashInstancedGeometryFilter : public Filter {
public:
    static Filter* create(const String& args) { return new SquashInstancedGeometryFilter(args); }

    SquashInstancedGeometryFilter(const String& args);
    virtual ~SquashInstancedGeometryFilter() {}

    virtual FilterDataPtr apply(FilterDataPtr input);
}; // class SquashInstancedGeometryFilter

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_COMMON_FILTERS_SQUASH_INSTANCED_GEOMETRY_HPP_
