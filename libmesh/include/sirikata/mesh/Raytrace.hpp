/*  Sirikata
 *  Raytrace.hpp
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

#ifndef _SIRIKATA_MESH_RAYTRACE_HPP_
#define _SIRIKATA_MESH_RAYTRACE_HPP_

#include <sirikata/mesh/Meshdata.hpp>

namespace Sirikata {
namespace Mesh {

/** Traces a ray an returns information about the first point it hits on an the
 *  mesh.  Ray tracing is performed in mesh space, so if you want to perform the
 *  operation in world space, you need to transform the ray into object space,
 *  run the raytrace, and transform the results back.
 *
 *  \param mesh the mesh to test the ray against
 *  \param ray_start the starting position of the ray to trace
 *  \param ray_dir the direction of the ray to trace
 *  \param t_out the parametric value for the collision, in terms of ray_start
 *  and ray_dir
 *  \param hit_out the point of collision, if one was found
 *  \returns true if a collision was found, false otherwise
 */
SIRIKATA_MESH_FUNCTION_EXPORT bool Raytrace(MeshdataPtr mesh, const Vector3f& ray_start, const Vector3f& ray_dir, float32* t_out, Vector3f* hit_out);

} // namespace Mesh
} // namespace Sirikata

#endif //_SIRIKATA_MESH_RAYTRACE_HPP_
