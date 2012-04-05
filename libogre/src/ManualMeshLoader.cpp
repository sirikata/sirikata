// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualMeshLoader.hpp"
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBufferManager.h>
#include <sirikata/ogre/OgreConversions.hpp>

namespace Sirikata {
namespace Graphics {

using namespace Sirikata::Mesh;


ManualMeshLoader::ManualMeshLoader(MeshdataPtr meshdata, TextureBindingsMapPtr textureFingerprints)
 : mdptr(meshdata),
   mTextureFingerprints(textureFingerprints)
{
}

void ManualMeshLoader::loadResource(Ogre::Resource *r) {
    size_t totalVertexCount;
    bool useSharedBuffer;

    getMeshStats(&useSharedBuffer, &totalVertexCount);
    traverseNodes(r, useSharedBuffer, totalVertexCount);
}

Ogre::VertexData* ManualMeshLoader::createVertexData(const SubMeshGeometry &submesh, int vertexCount, Ogre::HardwareVertexBufferSharedPtr&vbuf) {
    using namespace Ogre;
    Ogre::VertexData *vertexData = OGRE_NEW Ogre::VertexData();
    VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
    size_t currOffset = 0;

    vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_POSITION);
    currOffset += VertexElement::getTypeSize(VET_FLOAT3);
    if (submesh.normals.size()==submesh.positions.size()) {
        vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_NORMAL);
        currOffset += VertexElement::getTypeSize(VET_FLOAT3);
    }
    if (submesh.tangents.size()==submesh.positions.size()) {
        vertexDecl->addElement(0, currOffset, VET_FLOAT3, VES_TANGENT);
        currOffset += VertexElement::getTypeSize(VET_FLOAT3);
    }
    if (submesh.colors.size()==submesh.positions.size()) {
        vertexDecl->addElement(0, currOffset, VET_COLOUR_ARGB, VES_DIFFUSE);
        currOffset += VertexElement::getTypeSize(VET_COLOUR_ARGB);
    }
    unsigned int numUVs=submesh.texUVs.size();
    for (unsigned int tc=0; tc<numUVs; ++ tc) {
        VertexElementType vet;
        switch (submesh.texUVs[tc].stride) {
          case 1:
            vet=VET_FLOAT1;
            break;
          case 2:
            vet=VET_FLOAT2;
            break;
          case 3:
            vet=VET_FLOAT3;
            break;
          case 4:
            vet=VET_FLOAT4;
            break;
          default:
            vet=VET_FLOAT4;
        }
        vertexDecl->addElement(0, currOffset, vet, VES_TEXTURE_COORDINATES, tc);
        currOffset += VertexElement::getTypeSize(vet);
    }
    vertexData->vertexCount = vertexCount;
    HardwareBuffer::Usage vertexBufferUsage= HardwareBuffer::HBU_STATIC_WRITE_ONLY;
    bool vertexShadowBuffer = false;
    vbuf = HardwareBufferManager::getSingleton().
        createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount,
            vertexBufferUsage, vertexShadowBuffer);
    VertexBufferBinding* binding = vertexData->vertexBufferBinding;
    binding->setBinding(0, vbuf);
    return vertexData;
}

void ManualMeshLoader::getMeshStats(bool* useSharedBufferOut, size_t* totalVertexCountOut) {
    using namespace Ogre;

    const Meshdata& md = *mdptr;

    bool useSharedBuffer = true;
    for(GeometryInstanceList::const_iterator geoinst_it = md.instances.begin(); geoinst_it != md.instances.end(); geoinst_it++) {
        const GeometryInstance& geoinst = *geoinst_it;

        // Get the instanced submesh
        if (geoinst.geometryIndex >= md.geometry.size())
            continue;
        size_t i = geoinst.geometryIndex;

        if ((md.geometry[i].positions.size()==md.geometry[i].normals.size())
            != (md.geometry[0].positions.size()==md.geometry[0].normals.size())) {
            useSharedBuffer=false;
        }

        if ((md.geometry[i].positions.size()==md.geometry[i].tangents.size())
            != (md.geometry[0].positions.size()==md.geometry[0].tangents.size())) {
            useSharedBuffer=false;
        }

        if ((md.geometry[i].positions.size()==md.geometry[i].colors.size())
            != (md.geometry[0].positions.size()==md.geometry[0].colors.size())) {
            useSharedBuffer=false;
        }
        if (md.geometry[0].texUVs.size()!=md.geometry[i].texUVs.size()) {
            useSharedBuffer=false;
        } else {
            for (size_t j=0;j<md.geometry[0].texUVs.size();++j) {
                if (md.geometry[i].texUVs[j].stride!=md.geometry[i].texUVs[j].stride)
                    useSharedBuffer=false;
                if ((md.geometry[i].texUVs[j].uvs.size()==md.geometry[i].positions.size())!=
                    (md.geometry[0].texUVs[j].uvs.size()==md.geometry[0].positions.size())) {
                    useSharedBuffer=false;
                }
            }
        }
    }

    // Be sure to compute over instantiated geometries. Otherwise
    // we would count extra for uninstantiated ones and compute
    // wrong for multiply instantiated geometries.
    size_t totalVertexCount=0;
    Meshdata::GeometryInstanceIterator geoinst_it = md.getGeometryInstanceIterator();
    uint32 geoinst_idx;
    Matrix4x4f pos_xform;
    while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
        totalVertexCount += md.geometry[ md.instances[geoinst_idx].geometryIndex ].positions.size();
    }

    if (totalVertexCount>65535)
        useSharedBuffer=false;

    *useSharedBufferOut = useSharedBuffer;
    *totalVertexCountOut = totalVertexCount;
}


void ManualMeshLoader::traverseNodes(Ogre::Resource* r, const bool useSharedBuffer, const size_t totalVertexCount) {
    using namespace Ogre;
    const Meshdata& md = *mdptr;

    Ogre::Mesh* mesh = dynamic_cast <Ogre::Mesh*> (r);

    if (totalVertexCount==0 || mesh==NULL)
        return;
    char * pData  = NULL;
    Ogre::HardwareVertexBufferSharedPtr vbuf;
    unsigned short sharedVertexOffset=0;
    unsigned int totalVerticesCopied=0;

    Ogre::VertexData* vertexData;

    bool hasBones = false;

    if (useSharedBuffer) {
        mesh->sharedVertexData = createVertexData(md.geometry[md.instances[0].geometryIndex],totalVertexCount, vbuf);
        pData=(char*)vbuf->lock(HardwareBuffer::HBL_DISCARD);
        vertexData = mesh->sharedVertexData;
    }

    Meshdata::GeometryInstanceIterator geoinst_it = md.getGeometryInstanceIterator();
    uint32 geoinst_idx;
    Matrix4x4f pos_xform;
    BoundingBox3f3f mesh_aabb = BoundingBox3f3f::null();
    double mesh_rad = 0.0;
    while( geoinst_it.next(&geoinst_idx, &pos_xform) ) {
        assert(geoinst_idx < md.instances.size());
        const GeometryInstance& geoinst = md.instances[geoinst_idx];

        Matrix3x3f normal_xform = pos_xform.extract3x3().inverseTranspose();

        // Get the instanced submesh
        if (geoinst.geometryIndex >= md.geometry.size())
            continue;
        const SubMeshGeometry& submesh = md.geometry[geoinst.geometryIndex];

        if (submesh.skinControllers.size() > 1) {
            SILOG(ogre,error,"Don't know how to handle multiple skin controllers, leaving in T-pose.");
        }
        else if (submesh.skinControllers.size() == 1) {
            hasBones = true;
            const SkinController& skin = submesh.skinControllers[0];
            // Set weights. This maps directly from our
            // (vert_idx,jointIndex,weight) pairs (at least as we
            // decode them) into Ogre.
            // FIXME this can be done on a per-submesh (rather
            // than per-mesh) basis. Do we ever need that?
            Ogre::VertexBoneAssignment vba;

            for(uint32 vidx = 0;  vidx < submesh.positions.size(); vidx++) {
                vba.vertexIndex = vidx;

                for(uint32 ass_idx = skin.weightStartIndices[vidx];  ass_idx < skin.weightStartIndices[vidx+1]; ass_idx++) {
                    vba.boneIndex = skin.joints[ skin.jointIndices[ass_idx] ]+1;
                    vba.weight = skin.weights[ass_idx];

                    mesh->addBoneAssignment(vba);
                }
            }

            mesh->_compileBoneAssignments();
        }

        BoundingBox3f3f submeshaabb;
        double rad=0;
        geoinst.computeTransformedBounds(md, pos_xform, &submeshaabb, &rad);
        mesh_aabb = (mesh_aabb == BoundingBox3f3f::null() ? submeshaabb : mesh_aabb.merge(submeshaabb));
        mesh_rad = std::max(mesh_rad, rad);

        const SkinController* skin = (submesh.skinControllers.size() > 0) ? (&(submesh.skinControllers[0])) : NULL;
        if (skin != NULL && skin->bindShapeMatrix != Matrix4x4f::identity() && skin->bindShapeMatrix != Matrix4x4f::zero()) {
            rad = 0;
            submeshaabb = BoundingBox3f3f::null();
            geoinst.computeTransformedBounds(md, pos_xform*(skin->bindShapeMatrix), &submeshaabb, &rad);
            mesh_aabb = (mesh_aabb == BoundingBox3f3f::null() ? submeshaabb : mesh_aabb.merge(submeshaabb));
            mesh_rad = std::max(mesh_rad, rad);
        }

        int vertcount = submesh.positions.size();
        int normcount = submesh.normals.size();
        for (size_t primitive_index = 0; primitive_index<submesh.primitives.size(); ++primitive_index) {
            const SubMeshGeometry::Primitive& prim=submesh.primitives[primitive_index];

            // FIXME select proper texture/material
            std::string matname = "baseogremat";
            GeometryInstance::MaterialBindingMap::const_iterator whichMaterial = geoinst.materialBindingMap.find(prim.materialId);
            if (whichMaterial == geoinst.materialBindingMap.end())
                SILOG(ogre, error, "[OGRE] Invalid MaterialBindingMap: couldn't find " << prim.materialId << " for " << md.uri);
            else if (whichMaterial->second >= md.materials.size())
                SILOG(ogre, error, "[OGRE] Invalid MaterialBindingMap: " << prim.materialId << " in " << md.uri << " references material " << whichMaterial->second << " which doesn't exist.");
            else
                matname = ogreMaterialName(md.materials[whichMaterial->second], Transfer::URI(md.uri), mTextureFingerprints);

            Ogre::SubMesh *osubmesh = mesh->createSubMesh(submesh.name);

            osubmesh->setMaterialName(matname);
            if (useSharedBuffer) {
                osubmesh->useSharedVertices=true;
            }else {
                osubmesh->useSharedVertices=false;
                osubmesh->vertexData = createVertexData(submesh,(int)submesh.positions.size(),vbuf);
                pData=(char*)vbuf->lock(HardwareBuffer::HBL_DISCARD);
                vertexData = osubmesh->vertexData;
            }

            if (primitive_index==0||useSharedBuffer==false) {
                if (useSharedBuffer) {
                    sharedVertexOffset=totalVerticesCopied;
                    assert(totalVerticesCopied<65536||vertcount==0);//should be checked by other code
                }
                totalVerticesCopied+=vertcount;
                bool warn_texcoords = false;
                for (int i=0;i<vertcount; ++i) {
                    Vector3f v = submesh.positions[i];
                    Vector4f v_xform =  pos_xform * Vector4f(v[0], v[1], v[2], 1.f);
                    v = Vector3f(v_xform[0], v_xform[1], v_xform[2]);
                    memcpy(pData,&v.x,sizeof(float));
                    memcpy(pData+sizeof(float),&v.y,sizeof(float));
                    memcpy(pData+2*sizeof(float),&v.z,sizeof(float));
                    pData+=VertexElement::getTypeSize(VET_FLOAT3);
                    if (submesh.normals.size()==submesh.positions.size()) {
                        Vector3f normal = submesh.normals[i];
                        normal = (normal_xform * normal).normal();
                        memcpy(pData,&normal.x,sizeof(float));
                        memcpy(pData+sizeof(float),&normal.y,sizeof(float));
                        memcpy(pData+2*sizeof(float),&normal.z,sizeof(float));
                        pData+=VertexElement::getTypeSize(VET_FLOAT3);
                    }
                    if (submesh.tangents.size()==submesh.positions.size()) {
                        Vector3f tangent = submesh.tangents[i];
                        tangent = normal_xform * tangent;
                        memcpy(pData,&tangent.x,sizeof(float));
                        memcpy(pData+sizeof(float),&tangent.y,sizeof(float));
                        memcpy(pData+2*sizeof(float),&tangent.z,sizeof(float));
                        pData+=VertexElement::getTypeSize(VET_FLOAT3);
                    }
                    if (submesh.colors.size()==submesh.positions.size()) {
                        unsigned char r = (unsigned char)(submesh.colors[i].x*255);
                        unsigned char g = (unsigned char)(submesh.colors[i].y*255);
                        unsigned char b = (unsigned char)(submesh.colors[i].z*255);
                        unsigned char a = (unsigned char)(submesh.colors[i].w*255);
                        pData[0]=a;
                        pData[1]=r;
                        pData[2]=g;
                        pData[3]=b;
                        pData+=VertexElement::getTypeSize(VET_COLOUR_ARGB);
                    }
                    unsigned int numUVs= submesh.texUVs.size();
                    for (unsigned int tc=0;tc<numUVs;++tc) {
                        VertexElementType vet;
                        int stride=0;
                        switch (submesh.texUVs[tc].stride) {
                          case 1:
                            stride=1;
                            vet=VET_FLOAT1;
                            break;
                          case 2:
                            stride=2;
                            vet=VET_FLOAT2;
                            break;
                          case 3:
                            stride=3;
                            vet=VET_FLOAT3;
                            break;
                          case 4:
                          default:
                            stride=4;
                            vet=VET_FLOAT4;
                            break;
                        }

                        // This should be:
                        //assert( i*stride < submesh.texUVs[tc].uvs.size() );
                        // but so many models seem to get this
                        // wrong that we need to hack around it.
                        if ( i*stride < (int)submesh.texUVs[tc].uvs.size() )
                            memcpy(pData,&submesh.texUVs[tc].uvs[i*stride],sizeof(float)*stride);
                        else { // The hack: just zero out the data
                            warn_texcoords = true;
                            memset(pData, 0, sizeof(float)*stride);
                        }

                        if (submesh.texUVs[tc].stride > 1) {
                            // Apparently we need to invert the V
                            // coordinate... perhaps somebody could document
                            // why we need this.
                            float UVHACK = submesh.texUVs[tc].uvs[i*stride+1];
                            UVHACK=1.0-UVHACK;
                            memcpy(pData+sizeof(float),&UVHACK,sizeof(float));
                        }

                        pData += VertexElement::getTypeSize(vet);
                    }
                }
                if (warn_texcoords) {
                    SILOG(ogre,warn,"Out of bounds texture coordinates on " << md.uri);
                }
                if (!useSharedBuffer) {
                    vbuf->unlock();

                    if (hasBones) {
                        Ogre::VertexDeclaration* newDecl = osubmesh->vertexData->vertexDeclaration->getAutoOrganisedDeclaration(true, false);
                        osubmesh->vertexData->reorganiseBuffers(newDecl);
                    }
                }
            }

            unsigned int indexcount = prim.indices.size();
            osubmesh->indexData->indexCount = indexcount;
            HardwareBuffer::Usage indexBufferUsage= HardwareBuffer::HBU_STATIC_WRITE_ONLY;
            bool indexShadowBuffer = false;

            osubmesh->indexData->indexBuffer = HardwareBufferManager::getSingleton().
                createIndexBuffer(HardwareIndexBuffer::IT_16BIT,
                    indexcount, indexBufferUsage, indexShadowBuffer);
            unsigned short * iData = (unsigned short*)osubmesh->indexData->indexBuffer->lock(HardwareBuffer::HBL_DISCARD);
            if (useSharedBuffer) {
                for (unsigned int i=0;i<indexcount;++i) {
                    iData[i]=prim.indices[i]+sharedVertexOffset;
                    assert(iData[i]<totalVertexCount);
                }
            }else {
                memcpy(iData,&prim.indices[0],indexcount*sizeof(unsigned short));
            }
            osubmesh->indexData->indexBuffer->unlock();
            switch (prim.primitiveType) {
              case SubMeshGeometry::Primitive::TRIANGLES:
                osubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
                break;
              case SubMeshGeometry::Primitive::TRISTRIPS:
                osubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_STRIP;
                break;
              case SubMeshGeometry::Primitive::TRIFANS:
                osubmesh->operationType = Ogre::RenderOperation::OT_TRIANGLE_FAN;
                break;
              case SubMeshGeometry::Primitive::LINESTRIPS:
                osubmesh->operationType = Ogre::RenderOperation::OT_LINE_STRIP;
                break;
              case SubMeshGeometry::Primitive::LINES:
                osubmesh->operationType = Ogre::RenderOperation::OT_LINE_LIST;
                break;
              case SubMeshGeometry::Primitive::POINTS:
                osubmesh->operationType = Ogre::RenderOperation::OT_POINT_LIST;
                break;
              default:
                break;
            }
        }
    }

    AxisAlignedBox ogremeshaabb(
        Graphics::toOgre(mesh_aabb.min()),
        Graphics::toOgre(mesh_aabb.max())
    );
    mesh->_setBounds(ogremeshaabb);
    mesh->_setBoundingSphereRadius(mesh_rad);

    if (useSharedBuffer) {
        assert(totalVerticesCopied==totalVertexCount);

        vbuf->unlock();

        if (hasBones) {
            Ogre::VertexDeclaration* newDecl = vertexData->vertexDeclaration->getAutoOrganisedDeclaration(true, false);
            vertexData->reorganiseBuffers(newDecl);
        }
    }
}

} // namespace Graphics
} // namespace Sirikata
