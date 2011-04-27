/*  Sirikata
 *  OgreMeshRaytrace.cpp
 *
 *  Copyright (c) 2009, Patrick Horn
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

#include <sirikata/proxyobject/Platform.hpp>
#include "OgreHeaders.hpp"
#include "OgreMeshRaytrace.hpp"
#include "OgreSubMesh.h"
#include "OgreRay.h"
#include "OgreEntity.h"
#include "OgreNode.h"

//using Ogre::RenderOperation;
using Ogre::SubMesh;
//using Ogre::Mesh;
//using Ogre::ManualObject;
using Ogre::VertexBufferBinding;
using Ogre::VertexData;
using Ogre::VertexDeclaration;
using Ogre::VertexElement;
//using Ogre::VertexElementType;
//using Ogre::VertexElementSemantic;
using Ogre::HardwareBuffer;
//using Ogre::HardwareBufferManager;
using Ogre::HardwareVertexBuffer;
using Ogre::HardwareIndexBuffer;
//using Ogre::HardwareIndexBufferSharedPtr;
using Ogre::IndexData;
using Ogre::Real;
//using Ogre::uint32;
//using Ogre::uint16;
//using Ogre::AxisAlignedBox;

namespace Sirikata { namespace Graphics {
OgreMesh::OgreMesh(Ogre::SubMesh *subMesh, bool texcoord, std::vector<TriVertex>& sharedVertices)
{
    syncFromOgreMesh(subMesh, texcoord, sharedVertices);
}
int64 OgreMesh::size() const{
    return mTriangles.size()*sizeof(Triangle);
}
void OgreMesh::syncFromOgreMesh(Ogre::SubMesh*subMesh, bool texcoord, std::vector<TriVertex>& sharedVertices)
{
    VertexData *vertexData;
    std::vector<TriVertex> subVertices;
    std::vector<TriVertex> *lpvertices;

    if (subMesh->useSharedVertices) {
        vertexData = subMesh->parent->sharedVertexData;
        lpvertices=&sharedVertices;
    }else {
        vertexData = subMesh->vertexData;
        lpvertices=&subVertices;
    }
    std::vector<TriVertex>&lvertices=*lpvertices;

    if (vertexData)
    {
        VertexDeclaration *vertexDecl = vertexData->vertexDeclaration;

        // find the element for position
        const VertexElement *element = vertexDecl->findElementBySemantic(Ogre::VES_POSITION);
        const VertexElement *texelement = texcoord ? vertexDecl->findElementBySemantic(Ogre::VES_TEXTURE_COORDINATES) : NULL;

        // find and lock the buffer containing position information
        VertexBufferBinding *bufferBinding = vertexData->vertexBufferBinding;
        HardwareVertexBuffer *buffer = bufferBinding->getBuffer(element->getSource()).get();
        if (lvertices.empty()) {
            unsigned char *pVert = static_cast<unsigned char*>(buffer->lock(HardwareBuffer::HBL_READ_ONLY));
            HardwareVertexBuffer *texbuffer = texelement ? bufferBinding->getBuffer(texelement->getSource()).get() : NULL;
            unsigned char *pTexVert = texbuffer
                ? (texbuffer == buffer ? pVert : static_cast<unsigned char*>(texbuffer->lock(HardwareBuffer::HBL_READ_ONLY)))
                : NULL;
            for (size_t vert = 0; vert < vertexData->vertexCount; vert++)
            {
                float *vertex = 0;
                Real x, y, z;
                element->baseVertexPointerToElement(pVert, &vertex);
                x = *vertex++;
                y = *vertex++;
                z = *vertex++;
                Ogre::Vector3 vec(x, y, z);
                Ogre::Vector2 texvec(0,0);
                if (texelement)
                {
                    float *texvertex = 0;
                    float u, v, w;
                    texelement->baseVertexPointerToElement(pTexVert, &texvertex);
                    u = *texvertex++;
                    v = *texvertex++;
                    texvec = Ogre::Vector2(u, v);
                    pTexVert += texbuffer->getVertexSize();
                }

                lvertices.push_back(TriVertex(vec, texvec.x, texvec.y));

                pVert += buffer->getVertexSize();
            }

            buffer->unlock();
        }
        // find and lock buffer containg vertex indices
        Ogre::RenderOperation ro;
        subMesh->_getRenderOperation(ro);
        if (ro.useIndexes)
        {
            IndexData * indexData = subMesh->indexData;
            HardwareIndexBuffer *indexBuffer = indexData->indexBuffer.get();
            void *pIndex = static_cast<unsigned char *>(indexBuffer->lock(HardwareBuffer::HBL_READ_ONLY));
            if (indexBuffer->getType() == HardwareIndexBuffer::IT_16BIT)
            {
                for (size_t index = indexData->indexStart; index < indexData->indexCount; )
                {

                    uint16 *uint16Buffer = (uint16 *) pIndex;
                    uint16 v1 = uint16Buffer[index++];
                    uint16 v2 = uint16Buffer[index++];
                    uint16 v3 = uint16Buffer[index++];
                    if (v1 < lvertices.size() && v2 < lvertices.size() && v3 < lvertices.size())
                        mTriangles.push_back(Triangle(lvertices[v1], lvertices[v2], lvertices[v3]));
                }
            }
            else if (indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT)
            {
                for (size_t index = indexData->indexStart; index < indexData->indexCount; )
                {
                    uint32 *uint16Buffer = (uint32 *) pIndex;
                    uint32 v1 = uint16Buffer[index++];
                    uint32 v2 = uint16Buffer[index++];
                    uint32 v3 = uint16Buffer[index++];
                    if (v1 < lvertices.size() && v2 < lvertices.size() && v3 < lvertices.size())
                        mTriangles.push_back(Triangle(lvertices[v1], lvertices[v2], lvertices[v3]));
                }
            }
            else
            {
                assert(0);
            }
            indexBuffer->unlock();
        }
        else
        {
            for (unsigned int i=0; i<lvertices.size(); i+=3)
            {
                mTriangles.push_back(Triangle(lvertices[i], lvertices[i+1], lvertices[i+2]));
            }
        }
    }
}


Ogre::Ray OgreMesh::transformRay(Ogre::Node *node, const Ogre::Ray &ray) {
  return ray;
  const Ogre::Vector3 &position = node->_getDerivedPosition();
  const Ogre::Quaternion &orient = node->_getDerivedOrientation();
  const Ogre::Vector3 &scale = node->_getDerivedScale();
  Ogre::Vector3 newStart = (orient.Inverse() * (ray.getOrigin() - position)) / scale;
  Ogre::Vector3 newDirection = orient.Inverse() * ray.getDirection();
  return Ogre::Ray(newStart, newDirection);
}
// http://www.netsoc.tcd.ie/~nash/tangent_note/tangent_note.html

bool OgreMesh::intersectTri(const Ogre::Ray &ray, IntersectResult &rtn, Triangle*itr, bool isplane) {
    std::pair<bool, Real> hit = isplane
      ? Ogre::Math::intersects(ray, Ogre::Plane(itr->v1.coord, itr->v2.coord,itr->v3.coord))
      : Ogre::Math::intersects(ray, itr->v1.coord, itr->v2.coord,itr->v3.coord, true, false);
    rtn.u = 0;
    rtn.v = 0;
    if (hit.first && hit.second < rtn.distance) {
      rtn.intersected = hit.first;
      rtn.distance = hit.second;
      Ogre::Vector3 nml=(itr->v1.coord-itr->v2.coord).
              crossProduct(itr->v3.coord-itr->v2.coord);
      rtn.normal.x=nml.x;
      rtn.normal.y=nml.y;
      rtn.normal.z=nml.z;
      rtn.tri = *itr;
      Ogre::Vector3 intersect = ray.getPoint(hit.second) - rtn.tri.v2.coord;
      Ogre::Vector3 aVec = (rtn.tri.v1.coord - rtn.tri.v2.coord);
      Ogre::Vector3 bVec = (rtn.tri.v3.coord - rtn.tri.v2.coord);
      if (aVec.length() > 1.0e-10 && bVec.length() > 1.0e-10) {
        rtn.u = rtn.tri.v2.u + (rtn.tri.v1.u - rtn.tri.v2.u)*cos(aVec.angleBetween(intersect).valueRadians())*intersect.length()/aVec.length();
        rtn.v = rtn.tri.v2.v + (rtn.tri.v3.v - rtn.tri.v2.v)*cos(bVec.angleBetween(intersect).valueRadians())*intersect.length()/bVec.length();
      }
    }
    return rtn.intersected;
}

void OgreMesh::intersect(Ogre::Node *node, const Ogre::Ray &ray, IntersectResult &rtn)
{
  rtn = IntersectResult();
  const Ogre::Vector3 &position = node->_getDerivedPosition();
  const Ogre::Quaternion &orient = node->_getDerivedOrientation();
  const Ogre::Vector3 &scale = node->_getDerivedScale();

  std::vector<Triangle>::iterator itr,itere=mTriangles.end();
  int i = 0;
  for (itr = mTriangles.begin(); itr != itere; itr++) {
    Triangle newt = *itr;
    newt.v1.coord = (orient * (newt.v1.coord * scale)) + position;
    newt.v2.coord = (orient * (newt.v2.coord * scale)) + position;
    newt.v3.coord = (orient * (newt.v3.coord * scale)) + position;
    intersectTri(ray, rtn, &newt, true);
    if (rtn.intersected) { rtn.tri = *itr; }
    //intersectTri(ray, rtn, &*itr);
/*    if (i < 10) {
      std::cout << "c1:"<<newt.v1.coord << " c2:"<<newt.v2.coord << " c3:"<<newt.v3.coord << " u:"<<newt.v2.u<<" v:"<<newt.v2.v<<std::endl
    }*/
  }
}

} }
