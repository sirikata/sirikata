#include <oh/Platform.hpp>
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
OgreMesh::OgreMesh(Ogre::Entity *entity, Ogre::SubMesh *subMesh, bool texcoord)
{
    syncFromOgreMesh(entity,subMesh, texcoord);
}
int64 OgreMesh::size() const{
    return mTriangles.size()*sizeof(Triangle);
}
void OgreMesh::syncFromOgreMesh(Ogre::Entity *entity,Ogre::SubMesh*subMesh, bool texcoord)
{
  const Ogre::Vector3 &position = entity->getParentNode()->_getDerivedPosition();
  const Ogre::Quaternion &orient = entity->getParentNode()->_getDerivedOrientation();
  const Ogre::Vector3 &scale = entity->getParentNode()->_getDerivedScale();

  VertexData *vertexData = subMesh->vertexData;
  if (vertexData) {
      VertexDeclaration *vertexDecl = vertexData->vertexDeclaration;
      
      // find the element for position
      const VertexElement *element = vertexDecl->findElementBySemantic(Ogre::VES_POSITION);
      const VertexElement *texelement = texcoord ? vertexDecl->findElementBySemantic(Ogre::VES_TEXTURE_COORDINATES) : NULL;

      // find and lock the buffer containing position information
      VertexBufferBinding *bufferBinding = vertexData->vertexBufferBinding;
      HardwareVertexBuffer *buffer = bufferBinding->getBuffer(element->getSource()).get();
      unsigned char *pVert = static_cast<unsigned char*>(buffer->lock(HardwareBuffer::HBL_READ_ONLY));
      HardwareVertexBuffer *texbuffer = texelement ? bufferBinding->getBuffer(texelement->getSource()).get() : NULL;
      unsigned char *pTexVert = texbuffer
          ? (texbuffer == buffer ? pVert : static_cast<unsigned char*>(texbuffer->lock(HardwareBuffer::HBL_READ_ONLY)))
          : NULL;
      std::vector<TriVertex> lvertices;
      for (size_t vert = 0; vert < vertexData->vertexCount; vert++) {
          float *vertex = 0;
          Real x, y, z;
          element->baseVertexPointerToElement(pVert, &vertex);
          x = *vertex++;
          y = *vertex++;
          z = *vertex++;
          Ogre::Vector3 vec(x, y, z);
          vec = (orient * (vec * scale)) + position;
          Ogre::Vector2 texvec(0,0);
          if (texelement) {
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
      
      // find and lock buffer containg vertex indices
      IndexData * indexData = subMesh->indexData;
      HardwareIndexBuffer *indexBuffer = indexData->indexBuffer.get();
      void *pIndex = static_cast<unsigned char *>(indexBuffer->lock(HardwareBuffer::HBL_READ_ONLY));
      if (indexBuffer->getType() == HardwareIndexBuffer::IT_16BIT) {
          for (size_t index = indexData->indexStart; index < indexData->indexCount; ) {
              uint16 *uint16Buffer = (uint16 *) pIndex;
              uint16 v1 = uint16Buffer[index++];
              uint16 v2 = uint16Buffer[index++];
              uint16 v3 = uint16Buffer[index++];
              
              mTriangles.push_back(Triangle(lvertices[v1], lvertices[v2], lvertices[v3]));
          }
      } else if (indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT) {
          for (size_t index = indexData->indexStart; index < indexData->indexCount; ) {
              uint32 *uint16Buffer = (uint32 *) pIndex;
              uint32 v1 = uint16Buffer[index++];
              uint32 v2 = uint16Buffer[index++];
              uint32 v3 = uint16Buffer[index++];
              
              mTriangles.push_back(Triangle(lvertices[v1], lvertices[v2], lvertices[v3]));
          }
      } else {
          assert(0);
      }
      indexBuffer->unlock();
  }
}

// http://www.netsoc.tcd.ie/~nash/tangent_note/tangent_note.html

static bool intersectTri(const Ogre::Ray &ray, IntersectResult &rtn, Triangle*itr) {
    std::pair<bool, Real> hit = Ogre::Math::intersects(ray,
      itr->v1.coord, itr->v2.coord,itr->v3.coord, true, false);
    if (hit.first && hit.second <= rtn.distance) {
      rtn.intersected = hit.first;
      rtn.distance = hit.second;
      Ogre::Vector3 nml=(itr->v1.coord-itr->v2.coord).
              crossProduct(itr->v3.coord-itr->v2.coord);
      rtn.normal.x=nml.x;
      rtn.normal.y=nml.y;
      rtn.normal.z=nml.z;
      rtn.tri = itr;
      Ogre::Vector3 intersect = ray.getPoint(rtn.distance) - rtn.tri->v2.coord;
      Ogre::Vector3 aVec = (rtn.tri->v1.coord - rtn.tri->v2.coord);
      Ogre::Vector3 bVec = (rtn.tri->v3.coord - rtn.tri->v2.coord);
      rtn.u = rtn.tri->v2.u + (rtn.tri->v1.u - rtn.tri->v2.u)*cos(aVec.angleBetween(intersect).valueRadians())*intersect.length()/aVec.length();
      rtn.v = rtn.tri->v2.v + (rtn.tri->v3.v - rtn.tri->v2.v)*cos(bVec.angleBetween(intersect).valueRadians())*intersect.length()/bVec.length();
      return true;
    }
    return false;
}

void OgreMesh::intersect(const Ogre::Ray &ray, IntersectResult &rtn)
{
  if (rtn.intersected) {
    if (intersectTri(ray, rtn, rtn.tri)) {
      return;
    }
  }
  rtn = IntersectResult();

  std::vector<Triangle>::iterator itr,itere=mTriangles.end();

  for (itr = mTriangles.begin(); itr != itere; itr++) {
    intersectTri(ray, rtn, &*itr);
  }
}

} }
