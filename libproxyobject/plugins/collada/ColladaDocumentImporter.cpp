/*  Sirikata libproxyobject -- COLLADA Document Importer
 *  ColladaDocumentImporter.cpp
 *
 *  Copyright (c) 2009, Mark C. Barnes
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

#include "ColladaDocumentImporter.hpp"

#include <cassert>
#include <iostream>
#include <stack>
#include <sirikata/proxyobject/Meshdata.hpp>

#include "COLLADAFWScene.h"
#include "COLLADAFWVisualScene.h"
#include "COLLADAFWInstanceVisualScene.h"
#include "COLLADAFWLibraryNodes.h"
#include "COLLADAFWLight.h"
#include "COLLADAFWEffect.h"

#define COLLADA_LOG(lvl,msg) SILOG(collada, lvl, "[COLLADA] " << msg);

namespace Sirikata { namespace Models {


ColladaDocumentImporter::ColladaDocumentImporter ( Transfer::URI const& uri, std::tr1::weak_ptr<ProxyMeshObject>pp )
    :   mDocument ( new ColladaDocument ( uri ) ),
        mState ( IDLE ),
        mProxyPtr(pp)
{
    assert((std::cout << "MCB: ColladaDocumentImporter::ColladaDocumentImporter() entered, uri: " << uri << std::endl,true));

//    SHA256 hash = SHA256::computeDigest(uri.toString());    /// rest of system uses hash
//    lastURIString = hash.convertToHexString();

//    lastURIString = uri.toString();
    mMesh = new Meshdata();
    mMesh->uri = uri.toString();
}

ColladaDocumentImporter::~ColladaDocumentImporter ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::~ColladaDocumentImporter() entered" << std::endl,true));

}

/////////////////////////////////////////////////////////////////////

ColladaDocumentPtr ColladaDocumentImporter::getDocument () const
{
    if(mState != FINISHED) {
        SILOG(collada,fatal,"STATE != Finished reached: malformed COLLADA document");
    }
    return mDocument;
}

String ColladaDocumentImporter::documentURI() const {
    return mDocument->getURI().toString();
}

void ColladaDocumentImporter::postProcess ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::postProcess() entered" << std::endl,true));

}

/////////////////////////////////////////////////////////////////////
// overrides from COLLADAFW::IWriter

void ColladaDocumentImporter::cancel ( COLLADAFW::String const& errorMessage )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::cancel(" << errorMessage << ") entered" << std::endl,true));

    mState = CANCELLED;
}

void ColladaDocumentImporter::start ()
{
    assert((std::cout << "MCB: ColladaDocumentImporter::start() entered" << std::endl,true));

    mState = STARTED;
}

// We need this to be a non-local definition, so hide it in a namespace
namespace Collada {
struct NodeState {
    enum Modes {
        Geo = 1, // Geometry (and lights, cameras, etc)
        InstNodes = 2, // Instance nodes (requires indirection)
        Nodes = 3 // Normal child nodes
    };

    NodeState(const COLLADAFW::Node* _node, COLLADABU::Math::Matrix4 _matrix, int _child = -1, Modes _mode = Geo)
     : node(_node), matrix(_matrix), child(_child), mode(_mode)
    {}

    const COLLADAFW::Node* node;
    COLLADABU::Math::Matrix4 matrix;
    unsigned int child;
    Modes mode;
};
} // namespace Collada

void ColladaDocumentImporter::finish ()
{
    using namespace Sirikata::Models::Collada;

    assert((std::cout << "MCB: ColladaDocumentImporter::finish() entered" << std::endl,true));

    postProcess ();

    // Generate the Meshdata from all our parsed data

    // Add geometries
    // FIXME only store the geometries we need


    mMesh->geometry.swap(mGeometries);
    mMesh->materials.swap(mEffects);
    mMesh->lights.swap(mLights);


    // Try to find the instanciated VisualScene
    VisualSceneMap::iterator vis_scene_it = mVisualScenes.find(mVisualSceneId);
    assert(vis_scene_it != mVisualScenes.end()); // FIXME
    const COLLADAFW::VisualScene* vis_scene = vis_scene_it->second;
    // Iterate through nodes. Currently we'll only output anything for nodes
    // with <instance_node> elements.
    for(size_t i = 0; i < vis_scene->getRootNodes().getCount(); i++) {
        const COLLADAFW::Node* rn = vis_scene->getRootNodes()[i];

        std::stack<NodeState> node_stack;
        node_stack.push( NodeState(rn, COLLADABU::Math::Matrix4::IDENTITY) );

        while(!node_stack.empty()) {
            NodeState curnode = node_stack.top();
            node_stack.pop();

            if (curnode.mode == NodeState::Geo) {
                // Transformation
                COLLADABU::Math::Matrix4 additional_xform = curnode.node->getTransformationMatrix();
                curnode.matrix = curnode.matrix * additional_xform;

                // Instance Geometries
                for(size_t geo_idx = 0; geo_idx < curnode.node->getInstanceGeometries().getCount(); geo_idx++) {
                    const COLLADAFW::InstanceGeometry* geo_inst = curnode.node->getInstanceGeometries()[geo_idx];
                    // FIXME handle child nodes, such as materials
                    IndicesMap::const_iterator geo_it = mGeometryMap.find(geo_inst->getInstanciatedObjectId());
                    if (geo_it == mGeometryMap.end()) {
                        continue;
                    }
                    GeometryInstance new_geo_inst;
                    new_geo_inst.geometryIndex = geo_it->second;
                    new_geo_inst.transform = Matrix4x4f(curnode.matrix, Matrix4x4f::ROW_MAJOR());
                    new_geo_inst.radius=0;
                    new_geo_inst.aabb=BoundingBox3f3f::null();
                    if (geo_it->second<mMesh->geometry.size()) {
                        const SubMeshGeometry & geometry = mMesh->geometry[geo_it->second];
                        for (size_t i=0;i<geometry.primitives.size();++i) {
                            const SubMeshGeometry::Primitive & prim=geometry.primitives[i];
                            size_t indsize=prim.indices.size();
                            for (size_t j=0;j<indsize;++j) {
                                Vector3f untransformed_pos = geometry.positions[prim.indices[j]];
                                Matrix4x4f trans = new_geo_inst.transform;
                                Vector4f pos4= trans*Vector4f(untransformed_pos.x,
                                                             untransformed_pos.y,
                                                             untransformed_pos.z,
                                                             1.0f);
                                Vector3f pos (pos4.x/pos4.w,pos4.y/pos4.w,pos4.z/pos4.w);
                                if (j==0&&i==0) {
                                    new_geo_inst.aabb=BoundingBox3f3f(pos,0);
                                    new_geo_inst.radius = pos.lengthSquared();
                                }else {
                                    new_geo_inst.aabb=new_geo_inst.aabb.merge(pos);
                                    double rads=pos.lengthSquared();
                                    if (rads> new_geo_inst.radius)
                                        new_geo_inst.radius=rads;
                                }
                            }
                        }
                        new_geo_inst.radius=sqrt(new_geo_inst.radius);
                        mMesh->instances.push_back(new_geo_inst);
                        
                    }
                }

                // Instance Lights
                for(size_t light_idx = 0; light_idx < curnode.node->getInstanceLights().getCount(); light_idx++) {
                    const COLLADAFW::InstanceLight* light_inst = curnode.node->getInstanceLights()[light_idx];
                    // FIXME handle child nodes, such as materials
                    IndicesMap::const_iterator light_it = mLightMap.find(light_inst->getInstanciatedObjectId());
                    if (light_it == mLightMap.end()) {
                        COLLADA_LOG(warn, "Couldn't find original of instantiated light; was probably ambient.");
                        continue;
                    }
                    LightInstance new_light_inst;
                    new_light_inst.lightIndex = light_it->second;
                    new_light_inst.transform = Matrix4x4f(curnode.matrix, Matrix4x4f::ROW_MAJOR());
                    mMesh->lightInstances.push_back(new_light_inst);
                }

                // Instance Cameras

                // Tell the remaining code to start processing children nodes
                curnode.child = 0;
                curnode.mode = NodeState::InstNodes;
            }
            if (curnode.mode == NodeState::InstNodes) {
                // Instance Nodes
                if ((size_t)curnode.child >= (size_t)curnode.node->getInstanceNodes().getCount()) {
                    curnode.child = 0;
                    curnode.mode = NodeState::Nodes;
                }
                else {
                    // Lookup the instanced node
                    COLLADAFW::UniqueId node_uniq_id = curnode.node->getInstanceNodes()[curnode.child]->getInstanciatedObjectId();
                    NodeMap::const_iterator node_it = mLibraryNodes.find(node_uniq_id);
                    assert(node_it != mLibraryNodes.end());
                    const COLLADAFW::Node* instanced_node = node_it->second;
                    // updated version of this node
                    node_stack.push( NodeState(curnode.node, curnode.matrix, curnode.child+1, curnode.mode) );
                    // And the child node
                    node_stack.push( NodeState(instanced_node, curnode.matrix) );
                }
            }
            if (curnode.mode == NodeState::Nodes) {
                // Process the next child if there are more
                if ((size_t)curnode.child < (size_t)curnode.node->getChildNodes().getCount()) {
                    // updated version of this node
                    node_stack.push( NodeState(curnode.node, curnode.matrix, curnode.child+1, curnode.mode) );
                    // And the child node
                    node_stack.push( NodeState(curnode.node->getChildNodes()[curnode.child], curnode.matrix) );
                }
            }
        }
    }


    // Finally, if we actually have anything for the user, ship the parsed mesh
    if (mMesh->instances.size() > 0 || mMesh->lightInstances.size()) {
    //    std::tr1::shared_ptr<ProxyMeshObject>(mProxyPtr).get()->meshParsed( mDocument->getURI().toString(),
    //                                          meshstore[mDocument->getURI().toString()] );
        std::tr1::shared_ptr<ProxyMeshObject>(spp)(mProxyPtr);
        spp->meshParsed( mDocument->getURI().toString(), mMesh );
    }
    mState = FINISHED;
}

bool ColladaDocumentImporter::writeGlobalAsset ( COLLADAFW::FileInfo const* asset )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGLobalAsset(" << asset << ") entered" << std::endl,true));
    bool ok = mDocument->import ( *this, *asset );
    mMesh->up_axis=asset->getUpAxisType();
    return ok;
}

bool ColladaDocumentImporter::writeScene ( COLLADAFW::Scene const* scene )
{
    const COLLADAFW::InstanceVisualScene* inst_vis_scene = scene->getInstanceVisualScene();
    mVisualSceneId = inst_vis_scene->getInstanciatedObjectId();

    return true;
}


bool ColladaDocumentImporter::writeVisualScene ( COLLADAFW::VisualScene const* visualScene )
{
    mVisualScenes[visualScene->getUniqueId()] = visualScene;
    return true;
}


bool ColladaDocumentImporter::writeLibraryNodes ( COLLADAFW::LibraryNodes const* libraryNodes )
{
    for(size_t idx = 0; idx < libraryNodes->getNodes().getCount(); idx++) {
        const COLLADAFW::Node* node = libraryNodes->getNodes()[idx];
        mLibraryNodes[node->getUniqueId()] = node;
    }
    return true;
}

struct IndexSet{
    unsigned int positionIndices;
    unsigned int normalIndices;
    unsigned int colorIndices;
    std::vector<unsigned int> uvIndices;
    IndexSet() {
        positionIndices=normalIndices=colorIndices=0;
    }
    struct IndexSetHash {
        size_t operator() (const IndexSet&indset)const{
            size_t retval=indset.positionIndices;
            retval^=indset.normalIndices*65535;
            retval^=indset.colorIndices*16711425;
            retval^=(indset.uvIndices.size()?indset.uvIndices[0]*255:0);
            return retval;
        };
    };
    bool operator==(const IndexSet&other)const {
        bool same= (positionIndices==other.positionIndices&&
            normalIndices==other.normalIndices&&
                    colorIndices==other.colorIndices);
        if (same&&uvIndices.size()==other.uvIndices.size()) {
            for (size_t i=0;i<uvIndices.size();++i) {
                if (uvIndices[i]!=other.uvIndices[i]) return false;
            }
        }
        return same;
    }
};
bool ColladaDocumentImporter::writeGeometry ( COLLADAFW::Geometry const* geometry )
{
    String uri = mDocument->getURI().toString();
    assert((std::cout << "MCB: ColladaDocumentImporter::writeGeometry(" << geometry << ") entered" << std::endl,true));

    COLLADAFW::Mesh const* mesh = dynamic_cast<COLLADAFW::Mesh const*>(geometry);
    if (!mesh) {
        std::cerr << "ERROR: we only support collada Mesh\n";
        return true;
        assert(false);
    }
    mGeometryMap[geometry->getUniqueId()]=mGeometries.size();
    mGeometries.push_back(SubMeshGeometry());
    SubMeshGeometry* submesh = &mGeometries.back();
    submesh->radius=0;
    submesh->aabb=BoundingBox3f3f::null();
    submesh->name = mesh->getName();

    COLLADAFW::MeshVertexData const& verts((mesh->getPositions()));
    COLLADAFW::MeshVertexData const& norms((mesh->getNormals()));
    COLLADAFW::MeshVertexData const& UVs((mesh->getUVCoords()));
    std::tr1::unordered_map<IndexSet,unsigned short,IndexSet::IndexSetHash> indexSetMap;

    COLLADAFW::FloatArray const* vdata = verts.getFloatValues();
    COLLADAFW::FloatArray const* ndata = norms.getFloatValues();
    COLLADAFW::FloatArray const* uvdata = UVs.getFloatValues();

    COLLADAFW::DoubleArray const* vdatad = verts.getDoubleValues();
    COLLADAFW::DoubleArray const* ndatad = norms.getDoubleValues();
    COLLADAFW::DoubleArray const* uvdatad = UVs.getDoubleValues();

    COLLADAFW::MeshPrimitiveArray const& primitives((mesh->getMeshPrimitives()));
    SubMeshGeometry::Primitive *outputPrim=NULL;
    for(size_t prim_index=0;prim_index<primitives.getCount();++prim_index) {
        COLLADAFW::MeshPrimitive * prim = primitives[prim_index];
        if (prim->getPrimitiveType()==COLLADAFW::MeshPrimitive::POLYLIST||
            prim->getPrimitiveType()==COLLADAFW::MeshPrimitive::POLYGONS) {
            COLLADA_LOG(error,"ERROR: we do not support COLLADA POLYGONS\n");
            continue;
        }
        size_t groupedVertexElementCount;
        bool multiPrim;
        switch (prim->getPrimitiveType()) {
          case COLLADAFW::MeshPrimitive::POLYLIST:
          case COLLADAFW::MeshPrimitive::POLYGONS:
          case COLLADAFW::MeshPrimitive::TRIANGLE_FANS:
          case COLLADAFW::MeshPrimitive::TRIANGLE_STRIPS:
          case COLLADAFW::MeshPrimitive::LINE_STRIPS:
            groupedVertexElementCount = prim->getGroupedVertexElementsCount();
            multiPrim=true;
            break;
          default:
            groupedVertexElementCount = 1;
            multiPrim=false;
            break;
        }
        size_t offset=0;
        for (size_t i=0;i<groupedVertexElementCount;++i) {
            submesh->primitives.push_back(SubMeshGeometry::Primitive());
            outputPrim=&submesh->primitives.back();
            size_t faceCount=prim->getGroupedVerticesVertexCount(i);
            if (!multiPrim)
                faceCount *= prim->getGroupedVertexElementsCount();
            for (size_t j=0;j<faceCount;++j) {
                size_t whichIndex = offset+j;
                IndexSet uniqueIndexSet;

                //gather the indices from the previous set
                uniqueIndexSet.positionIndices=prim->getPositionIndices()[whichIndex];
                uniqueIndexSet.normalIndices=prim->hasNormalIndices()?prim->getNormalIndices()[whichIndex]:uniqueIndexSet.positionIndices;

                for (size_t uvSet=0;uvSet < prim->getUVCoordIndicesArray().getCount();++uvSet) {
                    uniqueIndexSet.uvIndices.push_back(prim->getUVCoordIndices(uvSet)->getIndex(whichIndex));
                }

                //now that we know what the indices are, find them in the indexSetMap...if this is the first time we see the indices, we must gather the data and place it
                //into our output list

                std::tr1::unordered_map<IndexSet,unsigned short>::iterator where =  indexSetMap.find(uniqueIndexSet);
                int vertStride = 3;//verts.getStride(0);<-- OpenCollada returns bad values for this
                int normStride = 3;//norms.getStride(0);<-- OpenCollada returns bad values for this
                if (where==indexSetMap.end()) {
                    indexSetMap[uniqueIndexSet]=submesh->positions.size();
                    outputPrim->indices.push_back(submesh->positions.size());
                    if (vdata||vdatad) {
                        if (vdata) {
                            submesh->positions.push_back(Vector3f(vdata->getData()[uniqueIndexSet.positionIndices*vertStride],//FIXME: is stride 3 or 3*sizeof(float)
                                                                  vdata->getData()[uniqueIndexSet.positionIndices*vertStride+1],
                                                                  vdata->getData()[uniqueIndexSet.positionIndices*vertStride+2]));
                        }else if (vdatad) {
                            submesh->positions.push_back(Vector3f(vdatad->getData()[uniqueIndexSet.positionIndices*vertStride],//FIXME: is stride 3 or 3*sizeof(float)
                                                                  vdatad->getData()[uniqueIndexSet.positionIndices*vertStride+1],
                                                                  vdatad->getData()[uniqueIndexSet.positionIndices*vertStride+2]));
                        }
                        if (submesh->aabb==BoundingBox3f3f::null())
                            submesh->aabb=BoundingBox3f3f(submesh->positions.back(),0);
                        else
                            submesh->aabb=submesh->aabb.merge(submesh->positions.back());
                        double l2=submesh->positions.back().lengthSquared();
                        if (l2>submesh->radius)
                            submesh->radius=l2;

                    }else {
                        COLLADA_LOG(error,"SubMesh without position index data\n");
                    }
                    if (ndata) {
                        submesh->normals.push_back(Vector3f(ndata->getData()[uniqueIndexSet.normalIndices*normStride],//FIXME: is stride 3 or 3*sizeof(float)
                                                            ndata->getData()[uniqueIndexSet.normalIndices*normStride+1],
                                                            ndata->getData()[uniqueIndexSet.normalIndices*normStride+2]));
                    }else if (ndatad) {
                        submesh->normals.push_back(Vector3f(ndatad->getData()[uniqueIndexSet.normalIndices*normStride],//FIXME: is stride 3 or 3*sizeof(float)
                                                            ndatad->getData()[uniqueIndexSet.normalIndices*normStride+1],
                                                            ndatad->getData()[uniqueIndexSet.normalIndices*normStride+2]));
                    }


                    if (submesh->texUVs.size()<uniqueIndexSet.uvIndices.size())
                        submesh->texUVs.resize(uniqueIndexSet.uvIndices.size());
                    if (uvdata) {
                        for (size_t uvSet=0;uvSet<uniqueIndexSet.uvIndices.size();++uvSet) {
                            unsigned int stride=UVs.getStride(uvSet);
                            submesh->texUVs.back().stride=stride;
                            for (unsigned int s=0;s<stride;++s) {
                                submesh->texUVs[uvSet].uvs.push_back(uvdata->getData()[uniqueIndexSet.uvIndices[uvSet]*stride+s]);//FIXME: is stride k or k*sizeof(float)
                            }
                        }
                    }else if (uvdatad) {
                        for (size_t uvSet=0;uvSet<uniqueIndexSet.uvIndices.size();++uvSet) {
                            unsigned int stride=UVs.getStride(uvSet);
                            submesh->texUVs.back().stride=stride;
                            for (unsigned int s=0;s<stride;++s) {
                                submesh->texUVs[uvSet].uvs.push_back(uvdatad->getData()[uniqueIndexSet.uvIndices[uvSet]*stride+s]);//FIXME: is stride k or k*sizeof(float)
                            }
                        }
                    }
                }else {
                    outputPrim->indices.push_back(where->second);
                }

            }
            offset+=faceCount;
        }

    }
    submesh->radius=sqrt(submesh->radius);
    bool ok = mDocument->import ( *this, *geometry );

    return ok;
}


bool ColladaDocumentImporter::writeMaterial ( COLLADAFW::Material const* material )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeMaterial(" << material << ") entered" << std::endl,true));

    mMaterialMap[material->getUniqueId()]=material->getInstantiatedEffect();

    return true;
}
MaterialEffectInfo::Texture ColladaDocumentImporter::makeTexture 
                         (MaterialEffectInfo::Texture::Affecting type,
                          const COLLADAFW::MaterialBinding *binding,
                          const COLLADAFW::EffectCommon * effectCommon, 
                          const COLLADAFW::ColorOrTexture & color) {
    using namespace COLLADAFW;
    MaterialEffectInfo::Texture retval;
    if (color.isColor()) {
        retval.color.x=color.getColor().getRed();
        retval.color.y=color.getColor().getGreen();
        retval.color.z=color.getColor().getBlue();
        retval.color.w=color.getColor().getAlpha();
    }else {
        // retval.uri  = mTextureMap[color.getTexture().getTextureMapId()];
        TextureMapId tid =color.getTexture().getTextureMapId();
        size_t tbindcount = binding->getTextureCoordinateBindingArray().getCount();
        for (size_t i=0;i<tbindcount;++i) {
            const TextureCoordinateBinding& b=binding->getTextureCoordinateBindingArray()[i];
            if (b.getTextureMapId()==tid) {
                retval.texCoord = b.getSetIndex();//is this correct!?
                break;
            }
        }
        retval.affecting = type;
        const Sampler * sampler =effectCommon->getSamplerPointerArray()[color.getTexture().getSamplerId()];
#define FIX_ENUM(var,val) case Sampler::val: var = MaterialEffectInfo::Texture::val; break
        switch (sampler->getMinFilter()) {
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_NEAREST);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_LINEAR);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR);
          default:
            retval.minFilter = MaterialEffectInfo::Texture::SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR;
        }
        switch (sampler->getMipFilter()) {
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST);
            FIX_ENUM(retval.minFilter,SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR);
          default:break;
        }
        switch (sampler->getMagFilter()) {
            FIX_ENUM(retval.magFilter,SAMPLER_FILTER_NEAREST);
            FIX_ENUM(retval.magFilter,SAMPLER_FILTER_LINEAR);
          default:
            retval.minFilter = MaterialEffectInfo::Texture::SAMPLER_FILTER_LINEAR;
        }
        
        switch (const_cast<Sampler*>(sampler)->getSamplerType()) {//<-- bug in constness
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_1D);
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_2D);
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_3D);
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_CUBE);
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_RECT);
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_DEPTH);
            FIX_ENUM(retval.samplerType,SAMPLER_TYPE_STATE);
          default:
            retval.samplerType = MaterialEffectInfo::Texture::SAMPLER_TYPE_2D;
        }
        switch (sampler->getWrapS()) {
            FIX_ENUM(retval.wrapS,WRAP_MODE_WRAP);
            FIX_ENUM(retval.wrapS,WRAP_MODE_MIRROR);
            FIX_ENUM(retval.wrapS,WRAP_MODE_CLAMP);
          default:
            retval.wrapS = MaterialEffectInfo::Texture::WRAP_MODE_CLAMP;
        }
        switch (sampler->getWrapT()) {
            FIX_ENUM(retval.wrapT,WRAP_MODE_WRAP);
            FIX_ENUM(retval.wrapT,WRAP_MODE_MIRROR);
            FIX_ENUM(retval.wrapT,WRAP_MODE_CLAMP);
          default:
            retval.wrapT = MaterialEffectInfo::Texture::WRAP_MODE_CLAMP;
        }
        switch (sampler->getWrapP()) {
            FIX_ENUM(retval.wrapU,WRAP_MODE_WRAP);
            FIX_ENUM(retval.wrapU,WRAP_MODE_MIRROR);
            FIX_ENUM(retval.wrapU,WRAP_MODE_CLAMP);
          default:
            retval.wrapU = MaterialEffectInfo::Texture::WRAP_MODE_CLAMP;
        }
        retval.mipBias = sampler->getMipmapBias();
        retval.maxMipLevel = sampler->getMipmapMaxlevel();
        retval.uri = mTextureMap[sampler->getSourceImage()];
    }
    return retval;
}

bool ColladaDocumentImporter::writeEffect ( COLLADAFW::Effect const* effect )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeImage(" << effect << ") entered" << std::endl,true));
    mColladaEffects[effect->getUniqueId()]=effect;
    return true;
}
size_t ColladaDocumentImporter::finishEffect(const COLLADAFW::MaterialBinding *binding) {
    using namespace COLLADAFW;
    size_t retval=mEffects.size();
    mEffects.push_back(MaterialEffectInfo());
    const Effect *effect=NULL;
    {
        const UniqueId & refmat= binding->getReferencedMaterial();
        IdMap::iterator matwhere=mMaterialMap.find(refmat);
        if (matwhere!=mMaterialMap.end()) {
            ColladaEffectMap::iterator effectwhere = mColladaEffects.find(matwhere->second);
            if (effectwhere!=mColladaEffects.end()) {
                effect=effectwhere->second;
            }else return retval;
        }else return retval;
    }
    MaterialEffectInfo&mat = mEffects.back();
    CommonEffectPointerArray commonEffects = effect->getCommonEffects();
    if (commonEffects.getCount()) {
        EffectCommon* commonEffect = commonEffects[0];
        switch (commonEffect->getShaderType()) {
          case EffectCommon::SHADER_BLINN:
          case EffectCommon::SHADER_PHONG:
            mat.textures.push_back(makeTexture(MaterialEffectInfo::Texture::SPECULAR, binding, commonEffect,commonEffect->getSpecular()));
          case EffectCommon::SHADER_LAMBERT:
            mat.textures.push_back(makeTexture(MaterialEffectInfo::Texture::DIFFUSE, binding, commonEffect,commonEffect->getDiffuse()));
            mat.textures.push_back(makeTexture(MaterialEffectInfo::Texture::AMBIENT, binding, commonEffect,commonEffect->getAmbient()));
            
          case EffectCommon::SHADER_CONSTANT:
            mat.textures.push_back(makeTexture(MaterialEffectInfo::Texture::EMISSION, binding, commonEffect,commonEffect->getEmission()));
            mat.textures.push_back(makeTexture(MaterialEffectInfo::Texture::OPACITY, binding, commonEffect,commonEffect->getOpacity()));
            mat.textures.push_back(makeTexture(MaterialEffectInfo::Texture::REFLECTIVE,binding, commonEffect,commonEffect->getReflective()));
            break;
          default:
            break;
        }
        mat.shininess= commonEffect->getShininess().getType()==FloatOrParam::FLOAT
            ? commonEffect->getShininess().getFloatValue()
             : 1.0;
        mat.reflectivity = commonEffect->getReflectivity().getType()==FloatOrParam::FLOAT
            ? commonEffect->getReflectivity().getFloatValue()
             : 1.0;
        
    }else {
        mat.textures.push_back(MaterialEffectInfo::Texture());
        mat.textures.back().color.x=effect->getStandardColor().getRed();
        mat.textures.back().color.y=effect->getStandardColor().getGreen();
        mat.textures.back().color.z=effect->getStandardColor().getBlue();
        mat.textures.back().color.w=effect->getStandardColor().getAlpha();
    }
    assert((std::cout << "MCB: ColladaDocumentImporter::writeEffect(" << effect << ") entered" << std::endl,true));
    return retval;
}


bool ColladaDocumentImporter::writeCamera ( COLLADAFW::Camera const* camera )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeCamera(" << camera << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeImage ( COLLADAFW::Image const* image )
{
    std::string imageUri = image->getImageURI().getURIString();
    assert((std::cout << "MCB: ColladaDocumentImporter::writeImage(" << imageUri << ") entered" << std::endl,true));
    mTextureMap[image->getUniqueId()]=imageUri;
    mMesh->textures.push_back(imageUri);
    return true;
}


bool ColladaDocumentImporter::writeLight ( COLLADAFW::Light const* light )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeLight(" << light << ") entered" << std::endl,true));
    mLightMap[light->getUniqueId()] = mLights.size();
    mLights.push_back(LightInfo());
    LightInfo *sublight = &mLights.back();

    Color lcol( light->getColor().getRed(), light->getColor().getGreen(), light->getColor().getBlue() );
    sublight->setLightDiffuseColor(lcol);
    sublight->setLightSpecularColor(lcol);

    double const_att = light->getConstantAttenuation();
    double lin_att = light->getLinearAttenuation();
    double quad_att = light->getQuadraticAttenuation();
    sublight->setLightFalloff((float)const_att, (float)lin_att, (float)quad_att);

    // Type
    switch (light->getLightType()) {
      case COLLADAFW::Light::AMBIENT_LIGHT:
        sublight->setLightAmbientColor(lcol);
        sublight->setLightDiffuseColor(Color(0,0,0));
        sublight->setLightSpecularColor(Color(0,0,0));
        sublight->setLightType(LightInfo::POINT);//just make it a point light for now
        break;
      case COLLADAFW::Light::DIRECTIONAL_LIGHT:
        sublight->setLightType(LightInfo::DIRECTIONAL);
        break;
      case COLLADAFW::Light::POINT_LIGHT:
        sublight->setLightType(LightInfo::POINT);
        break;
      case COLLADAFW::Light::SPOT_LIGHT:
        sublight->setLightType(LightInfo::SPOTLIGHT);
        break;
      default:
        mLights.pop_back();
    }



    return true;
}


bool ColladaDocumentImporter::writeAnimation ( COLLADAFW::Animation const* animation )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeAnimation(" << animation << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeAnimationList ( COLLADAFW::AnimationList const* animationList )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::ColladaDocumentImporter() entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeSkinControllerData ( COLLADAFW::SkinControllerData const* skinControllerData )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeSkinControllerData(" << skinControllerData << ") entered" << std::endl,true));
    return true;
}


bool ColladaDocumentImporter::writeController ( COLLADAFW::Controller const* controller )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeController(" << controller << ") entered" << std::endl,true));
    return true;
}

bool ColladaDocumentImporter::writeFormulas ( COLLADAFW::Formulas const* formulas )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeFormulas(" << formulas << ") entered" << std::endl,true));
    return true;
}

bool ColladaDocumentImporter::writeKinematicsScene ( COLLADAFW::KinematicsScene const* kinematicsScene )
{
    assert((std::cout << "MCB: ColladaDocumentImporter::writeKinematicsScene(" << kinematicsScene << ") entered" << std::endl,true));
    return true;
}


} // namespace Models
} // namespace Sirikata
