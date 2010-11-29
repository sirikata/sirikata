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

#ifndef _SIRIKATA_MESH_MESHDATA_HPP_
#define _SIRIKATA_MESH_MESHDATA_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Sha256.hpp>
#include "LightInfo.hpp"


namespace Sirikata {

struct SubMeshGeometry {
    std::string name;
    std::vector<Sirikata::Vector3f> positions;

  //used only during simplification
  std::vector< Matrix4x4f  > positionQs;




    std::vector<Sirikata::Vector3f> normals;
    std::vector<Sirikata::Vector3f> tangents;
    std::vector<Sirikata::Vector4f> colors;
    std::vector<unsigned int> influenceStartIndex;//a list of where a given position's joint weights start
    std::vector<unsigned int> jointindices;
    std::vector<float> weights;

    std::vector<Sirikata::Matrix4x4f> inverseBindMatrices;

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
        typedef size_t MaterialId;
        MaterialId materialId;
    };
    BoundingBox3f3f aabb;
    double radius;
    std::vector<Primitive> primitives;

};
struct GeometryInstance {
    typedef std::map<SubMeshGeometry::Primitive::MaterialId,size_t> MaterialBindingMap;
    MaterialBindingMap materialBindingMap;//maps materialIndex to offset in Meshdata's materials
    unsigned int geometryIndex; // Index in SubMeshGeometryList
    Matrix4x4f transform;
    BoundingBox3f3f aabb;//transformed aabb
    double radius;//transformed radius

};

struct LightInstance {
    int lightIndex; // Index in LightInfoList
    Matrix4x4f transform;
};

struct MaterialEffectInfo {
    struct Texture {
        std::string uri;
        Vector4f color;//color while the texture is pulled in, or if the texture is 404'd
        size_t texCoord;
        enum Affecting {
            DIFFUSE,
            SPECULAR,
            EMISSION,
            AMBIENT,
            REFLECTIVE,
            OPACITY,

        }affecting;
        enum SamplerType
        {
			SAMPLER_TYPE_UNSPECIFIED,
			SAMPLER_TYPE_1D,
			SAMPLER_TYPE_2D,
			SAMPLER_TYPE_3D,
			SAMPLER_TYPE_CUBE,
			SAMPLER_TYPE_RECT,
			SAMPLER_TYPE_DEPTH,
			SAMPLER_TYPE_STATE
		} samplerType;
		enum SamplerFilter
		{
			SAMPLER_FILTER_UNSPECIFIED,
			SAMPLER_FILTER_NONE,
			SAMPLER_FILTER_NEAREST,
			SAMPLER_FILTER_LINEAR,
			SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST,
			SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST,
			SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR,
			SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR
		};
        SamplerFilter minFilter;
        SamplerFilter magFilter;
		enum WrapMode
		{
			WRAP_MODE_UNSPECIFIED=0,
			// NONE == GL_CLAMP_TO BORDER The defined behavior for NONE is
			// consistent with decal texturing where the border is black.
			// Mapping this calculation to GL_CLAMP_TO_BORDER is the best
			// approximation of this.
			WRAP_MODE_NONE,
 			// WRAP == GL_REPEAT Ignores the integer part of texture coordinates,
			// using only the fractional part.
			WRAP_MODE_WRAP,
			// MIRROR == GL_MIRRORED_REPEAT First mirrors the texture coordinate.
			// The mirrored coordinate is then clamped as described for CLAMP_TO_EDGE.
			WRAP_MODE_MIRROR,
			// CLAMP == GL_CLAMP_TO_EDGE Clamps texture coordinates at all
			// mipmap levels such that the texture filter never samples a
			// border texel. Note: GL_CLAMP takes any texels beyond the
			// sampling border and substitutes those texels with the border
			// color. So CLAMP_TO_EDGE is more appropriate. This also works
			// much better with OpenGL ES where the GL_CLAMP symbol was removed
			// from the OpenGL ES specification.
			WRAP_MODE_CLAMP,
			// BORDER GL_CLAMP_TO_BORDER Clamps texture coordinates at all
			// MIPmaps such that the texture filter always samples border
			// texels for fragments whose corresponding texture coordinate
			// is sufficiently far outside the range [0, 1].
			WRAP_MODE_BORDER
		};
        WrapMode wrapS,wrapT,wrapU;
        unsigned int maxMipLevel;
        float mipBias;
    };
    typedef std::vector<Texture> TextureList;
    TextureList textures;
    float shininess;
    float reflectivity;
};




struct Meshdata {
    typedef std::vector<SubMeshGeometry> SubMeshGeometryList;
    typedef std::vector<LightInfo> LightInfoList;
    typedef std::vector<std::string> TextureList;
    typedef std::tr1::unordered_map<std::string, std::string> URIMap;

    typedef std::vector<GeometryInstance> GeometryInstanceList;
    typedef std::vector<LightInstance> LightInstanceList;
    typedef std::vector<MaterialEffectInfo> MaterialEffectInfoList;
    SubMeshGeometryList geometry;
    TextureList textures;
    URIMap textureMap;
    LightInfoList lights;
    MaterialEffectInfoList materials;

    std::string uri;
    SHA256 hash;
    long id;

    GeometryInstanceList instances;
    LightInstanceList lightInstances;



};

typedef std::tr1::shared_ptr<Meshdata> MeshdataPtr;

} // namespace Sirikata

#endif //_SIRIKATA_MESH_MESHDATA_HPP_
