/*  Sirikata
 *  Meshdata.hpp
 *
 *  Copyright (c) 2010, Daniel B. Miller
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

#ifndef _MESHDATA_HPP_

extern long Meshdata_counter;

namespace Sirikata {

struct SubMeshGeometry {
    std::string name;
    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<Sirikata::Vector2f> texUVs;
    std::vector<int> position_indices;
    std::vector<int> normal_indices;
    std::vector<int> texUV_indices;
};
struct GeometryInstance {
    int geometryIndex; // Index in SubMeshGeometryList
    Matrix4x4f transform;
};

typedef std::vector<SubMeshGeometry*> SubMeshGeometryList;
typedef std::vector<std::string> TextureList;
typedef std::vector<GeometryInstance> GeometryInstanceList;

struct Meshdata {
    SubMeshGeometryList geometry;
    TextureList textures;

    std::string uri;
    int up_axis;
    long id;

    GeometryInstanceList instances;

    Meshdata() {
        id=Meshdata_counter++;
    }
};

} // namespace Sirikata

#define _MESHDATA_HPP_ true
#endif
