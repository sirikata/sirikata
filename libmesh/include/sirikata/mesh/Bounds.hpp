/*  Sirikata
 *  Bounds.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#ifndef _SIRIKATA_MESH_BOUNDS_HPP_
#define _SIRIKATA_MESH_BOUNDS_HPP_

#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {
namespace Mesh {

/** Compute the bounds of the mesh. */
SIRIKATA_MESH_FUNCTION_EXPORT void ComputeBounds(MeshdataPtr mesh, BoundingBox3f3f* bbox = NULL, double* rad = NULL);

/** Compute the bounds of the mesh when trasnformed by xform. The bounding box
 *  is computed *after* transforming the mesh, so it is tighter than computing
 *  the bounding box of the transformed original bounding box.
 */
SIRIKATA_MESH_FUNCTION_EXPORT void ComputeBounds(MeshdataPtr mesh, const Matrix4x4f& xform, BoundingBox3f3f* bbox = NULL, double* rad = NULL);

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_BOUNDS_HPP_
