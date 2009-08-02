#include "oh/Platform.hpp"
#include "OgreMeshRaytrace.hpp"
#include "OgreSubMesh.h"
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
OgreMesh::OgreMesh(Ogre::Entity *entity, Ogre::SubMesh *subMesh)
{
    syncFromOgreMesh(entity,subMesh);
}
int64 OgreMesh::size() const{
    return mTriangles.size()*sizeof(Triangle);
}
void OgreMesh::syncFromOgreMesh(Ogre::Entity *entity,Ogre::SubMesh*subMesh)
{
  const Ogre::Vector3 &position = entity->getParentNode()->_getDerivedPosition();
  const Ogre::Quaternion &orient = entity->getParentNode()->_getDerivedOrientation();
  const Ogre::Vector3 &scale = entity->getParentNode()->_getDerivedScale();

  VertexData *vertexData = subMesh->vertexData;
  if (vertexData) {
      VertexDeclaration *vertexDecl = vertexData->vertexDeclaration;
      
      // find the element for position
      const VertexElement *element = vertexDecl->findElementBySemantic(Ogre::VES_POSITION);
      
      // find and lock the buffer containing position information
      VertexBufferBinding *bufferBinding = vertexData->vertexBufferBinding;
      HardwareVertexBuffer *buffer = bufferBinding->getBuffer(element->getSource()).get();
      unsigned char *pVert = static_cast<unsigned char*>(buffer->lock(HardwareBuffer::HBL_READ_ONLY));
      std::vector<Ogre::Vector3> lvertices;
      for (size_t vert = 0; vert < vertexData->vertexCount; vert++) {
          Real *vertex = 0;
          Real x, y, z;
          element->baseVertexPointerToElement(pVert, &vertex);
          x = *vertex++;
          y = *vertex++;
          z = *vertex++;
          Ogre::Vector3 vec(x, y, z);
          vec = (orient * (vec * scale)) + position;
          
          lvertices.push_back(vec);
          
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

std::pair<bool, std::pair< double, Vector3f> > OgreMesh::intersect(const Ogre::Ray &ray)
{
  std::pair<bool, std::pair< double, Vector3f> > rtn(false,std::pair<double,Vector3f>(std::numeric_limits<Real>::max(),Vector3f(0,0,0)));
  std::vector<Triangle>::iterator itr,itere=mTriangles.end();

  for (itr = mTriangles.begin(); itr != itere; itr++) {
    std::pair<bool, Real> hit = Ogre::Math::intersects(ray,
      itr->mV1, itr->mV2,itr->mV3, true, false);
    if (hit.first && hit.second < rtn.second.first) {
      rtn.first = hit.first;
      rtn.second.first = hit.second;
      Ogre::Vector3 nml=(itr->mV1-itr->mV2).crossProduct(itr->mV3-itr->mV2);
      rtn.second.second.x=nml.x;
      rtn.second.second.y=nml.y;
      rtn.second.second.z=nml.z;
    }
  }

  return rtn;
}

} }
