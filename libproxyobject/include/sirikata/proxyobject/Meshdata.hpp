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

#ifndef _SIRIKATA_PROXYOBJECT_MESHDATA_HPP_
#define _SIRIKATA_PROXYOBJECT_MESHDATA_HPP_

#include "LightInfo.hpp"


namespace Sirikata {

struct SubMeshGeometry {
    std::string name;
    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<Sirikata::Vector3f> tangents;
    std::vector<Sirikata::Vector4f> colors;
    struct TextureSet {
        unsigned int stride;
        std::vector<float> uvs;
    };
    std::vector<TextureSet>texUVs;
    struct Primitive {
        std::vector<unsigned short> indices;
        enum PrimitiveType {
            TRIANGLES,
            LINES,
            POINTS,
            LINESTRIPS,
            TRISTRIPS,
            TRIFANS
        }primitiveType;
        unsigned int MaterialIndex;//FIXME
    };
    std::vector<Primitive> primitives;
};
struct GeometryInstance {
    unsigned int geometryIndex; // Index in SubMeshGeometryList
    Matrix4x4f transform;
};

struct LightInstance {
    int lightIndex; // Index in LightInfoList
    Matrix4x4f transform;
};

struct MaterialEffectInfo {
    
    

};




struct Meshdata {
    typedef std::vector<SubMeshGeometry> SubMeshGeometryList;
    typedef std::vector<LightInfo> LightInfoList;
    typedef std::vector<std::string> TextureList;
    typedef std::vector<GeometryInstance> GeometryInstanceList;
    typedef std::vector<LightInstance> LightInstanceList;
    typedef std::vector<MaterialEffectInfo> MaterialEffectInfoList;
    SubMeshGeometryList geometry;
    TextureList textures;
    LightInfoList lights;
    MaterialEffectInfoList materials;

    std::string uri;
    int up_axis;
    long id;

    GeometryInstanceList instances;
    LightInstanceList lightInstances;

};

} // namespace Sirikata

#endif //_SIRIKATA_PROXYOBJECT_MESHDATA_HPP_
