/*  Sirikata
 *  Raytrace.cpp
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

#include <sirikata/mesh/Platform.hpp>
#include <sirikata/mesh/Bounds.hpp>

namespace Sirikata {
namespace Mesh {

namespace {

// These methods are largely taken from Ogre, though modified for our
// needs. Ogre is licensed under MIT and is compatible with our code. See
// OgreMain/src/OgreMath.cpp in the Ogre code. Original license:
//
// -----------------------------------------------------------------------------
// This source file is part of OGRE
//     (Object-oriented Graphics Rendering Engine)
// For the latest info, see http://www.ogre3d.org/
//
// Copyright (c) 2000-2009 Torus Knot Software Ltd
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// -----------------------------------------------------------------------------

Vector3f calculateBasicFaceNormal(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3)
{
    Vector3f normal = (v2 - v1).cross(v3 - v1);
    normal.normalizeThis();
    return normal;
}

Vector4f calculateFaceNormal(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3)
{
    Vector3f normal = calculateBasicFaceNormal(v1, v2, v3);
    // Now set up the w (distance of tri from origin
    return Vector4f(normal.x, normal.y, normal.z, -(normal.dot(v1)));
}

Vector3f calculateBasicFaceNormalWithoutNormalize(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3)
{
    Vector3f normal = (v2 - v1).cross(v3 - v1);
    return normal;
}

Vector4f calculateFaceNormalWithoutNormalize(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3)
{
    Vector3f normal = calculateBasicFaceNormalWithoutNormalize(v1, v2, v3);
    // Now set up the w (distance of tri from origin)
    return Vector4f(normal.x, normal.y, normal.z, -(normal.dot(v1)));
}

}

/** Raytraces a single triangle. Assumes that t_out is initialized and will only
 *  update output values and return true if the hit is at t < t_orig.
 *  \param v1 First vertex of triangle
 *  \param v2 Second vertex of triangle
 *  \param v3 Third vertex of triangle
 *  \param ray_start the starting position of the ray to trace
 *  \param ray_dir the direction of the ray to trace
 *  \param normal the normal for this triangle
 *  \param positive_side use collisions with the positive side of the triangle
 *  \param negative_side use collisions with the positive side of the triangle
 *  \param t_out the point of the collision, if one was found
 *  \param returns true if the triangle was hit and it was before the existing hit
 */
bool RaytraceTriangle(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3,
    const Vector3f& normal,
    const Vector3f& ray_start, const Vector3f& ray_dir,
    bool positive_side, bool negative_side,
    float32* t_out)
{
    //
    // Calculate intersection with plane.
    //
    float32 t;
    {
        float32 denom = normal.dot(ray_dir);

        // Check intersect side
        if (denom > + std::numeric_limits<float32>::epsilon())
        {
            if (!negative_side)
                return false;
        }
        else if (denom < - std::numeric_limits<float32>::epsilon())
        {
            if (!positive_side)
                return false;
        }
        else
        {
            // Parallel or triangle area is close to zero when
            // the plane normal not normalised.
            return false;
        }

        t = normal.dot(v1 - ray_start) / denom;

        if (t < 0)
        {
            // Intersection is behind origin
            return false;
        }
    }

    //
    // Calculate the largest area projection plane in X, Y or Z.
    //
    size_t i0, i1;
    {
        float32 n0 = fabs(normal[0]);
        float32 n1 = fabs(normal[1]);
        float32 n2 = fabs(normal[2]);

        i0 = 1; i1 = 2;
        if (n1 > n2)
        {
            if (n1 > n0) i0 = 0;
        }
        else
        {
            if (n2 > n0) i1 = 0;
        }
    }

    //
    // Check the intersection point is inside the triangle.
    //
    {
        float32 u1 = v2[i0] - v1[i0];
        float32 w1 = v2[i1] - v1[i1];
        float32 u2 = v3[i0] - v1[i0];
        float32 v2 = v3[i1] - v1[i1];
        float32 u0 = t * ray_dir[i0] + ray_start[i0] - v1[i0];
        float32 v0 = t * ray_dir[i1] + ray_start[i1] - v1[i1];

        float32 alpha = u0 * v2 - u2 * v0;
        float32 beta  = u1 * v0 - u0 * w1;
        float32 area  = u1 * v2 - u2 * w1;

        // epsilon to avoid float precision error
        const float32 EPSILON = 1e-6f;

        float32 tolerance = - EPSILON * area;

        if (area > 0)
        {
            if (alpha < tolerance || beta < tolerance || alpha+beta > area-tolerance)
                return false;
        }
        else
        {
            if (alpha > tolerance || beta > tolerance || alpha+beta < area-tolerance)
                return false;
        }
    }

    // Intersected, now compare t values
    if (t_out != NULL) {
        if (t > *t_out) return false;
        *t_out = t;
    }
    return true;
}

/** Raytraces a single triangle. Assumes that t_out is initialized and will only
 *  update output values and return true if the hit is at t < t_orig.
 *  \param v1 First vertex of triangle
 *  \param v2 Second vertex of triangle
 *  \param v3 Third vertex of triangle
 *  \param ray_start the starting position of the ray to trace
 *  \param ray_dir the direction of the ray to trace
 *  \param t_out the point of the collision, if one was found
 *  \param returns true if the triangle was hit and it was before the existing hit
 */
bool RaytraceTriangle(const Vector3f& v1, const Vector3f& v2, const Vector3f& v3,
    const Vector3f& ray_start, const Vector3f& ray_dir,
    float32* t_out)
{
    Vector3f normal = calculateBasicFaceNormalWithoutNormalize(v1, v2, v3);
    return RaytraceTriangle(v1, v2, v3, normal, ray_start, ray_dir, true, true, t_out);
}

bool SIRIKATA_MESH_FUNCTION_EXPORT Raytrace(MeshdataPtr mesh, const Vector3f& ray_start, const Vector3f& ray_dir, float32* t_out, Vector3f* hit_out) {
    bool found_hit = false;

    bool have_hit = false;
    float32 t = 1000000.0;

    // For each instanced geometry
    Meshdata::GeometryInstanceIterator geoIter = mesh->getGeometryInstanceIterator();
    uint32 indexInstance; Matrix4x4f transformInstance;
    while(geoIter.next(&indexInstance, &transformInstance)) {
        GeometryInstance& geoInst = mesh->instances[indexInstance];
        const SubMeshGeometry& geo = mesh->geometry[ geoInst.geometryIndex ];

        // We would transform all vertex data up front, but if we only have line
        // or point data there's no point. Do it on demand, but transform
        // everything at once when we know we need it.
        // FIXME its probably better to invert transformInstance and transform
        // the ray instead of the triangles.
        std::vector<Vector3f> pos;

        // Check each primitive in this geometry
        for(uint32 pi = 0; pi < geo.primitives.size(); pi++) {
            const SubMeshGeometry::Primitive& prim = geo.primitives[pi];
            if (prim.primitiveType == SubMeshGeometry::Primitive::LINES ||
                prim.primitiveType == SubMeshGeometry::Primitive::POINTS ||
                prim.primitiveType == SubMeshGeometry::Primitive::LINESTRIPS)
                continue;

            // Transform all the positions on demand the first time we encounter
            // a real need for them.
            if (pos.empty()) {
                pos.resize(geo.positions.size());
                for(uint32 i = 0; i < geo.positions.size(); i++)
                    pos[i] = transformInstance * geo.positions[i];
            }

            // Now we actually perform checks against transformed
            // triangles. Each of these will just generate a list of triangle
            // and defer to a helper to check for intersection and update the
            // state.
            switch(prim.primitiveType) {
              case SubMeshGeometry::Primitive::TRIANGLES:
                for(uint32 ii = 0; ii < prim.indices.size()/3; ii++) {
                    have_hit = have_hit ||
                        RaytraceTriangle(
                            pos[prim.indices[3*ii]],
                            pos[prim.indices[3*ii+1]],
                            pos[prim.indices[3*ii+2]],
                            ray_start, ray_dir,
                            &t
                        );
                }
                break;
              case SubMeshGeometry::Primitive::TRISTRIPS:
                for(uint32 ii = 0; ii < prim.indices.size()-2; ii++) {
                    uint32 i1 = (ii % 2 == 0) ? ii : ii+1;
                    uint32 i2 = (ii % 2 == 0) ? ii+1 : ii;
                    uint32 i3 = ii+2;
                    have_hit = have_hit ||
                        RaytraceTriangle(
                            pos[prim.indices[i1]],
                            pos[prim.indices[i2]],
                            pos[prim.indices[i3]],
                            ray_start, ray_dir,
                            &t
                        );
                }
                break;
              case SubMeshGeometry::Primitive::TRIFANS:
                for(uint32 ii = 1; ii < prim.indices.size()-1; ii++) {
                    have_hit = have_hit ||
                        RaytraceTriangle(
                            pos[prim.indices[0]],
                            pos[prim.indices[ii]],
                            pos[prim.indices[ii+1]],
                            ray_start, ray_dir,
                            &t
                        );
                }
                break;
              case SubMeshGeometry::Primitive::LINES:
              case SubMeshGeometry::Primitive::POINTS:
              case SubMeshGeometry::Primitive::LINESTRIPS:
                assert(false && "Shouldn't reach these cases since they were handled earlier.");
                break;
            }
        }
    }

    // Provide output
    if (have_hit) {
        if (t_out != NULL) *t_out = t;
        if (hit_out != NULL) *hit_out = ray_start + ray_dir * t;
    }
    return have_hit;
}

} // namespace Mesh
} // namespace Sirikata
