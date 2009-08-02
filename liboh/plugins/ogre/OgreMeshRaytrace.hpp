#ifndef _OGRE_MESH_HPP_
#define _OGRE_MESH_HPP_

#include <vector>
#include "MeruDefs.hpp"
#include "OgreVector3.h"
namespace Ogre {
  class Entity;
  class SubMesh;
  class Ray;
}

namespace Sirikata { namespace Graphics {

class Triangle {
public:
  Triangle(const Ogre::Vector3 &v1, const Ogre::Vector3 &v2, const Ogre::Vector3 &v3) : mV1(v1), mV2(v2), mV3(v3)
  {
  }

  Ogre::Vector3 mV1;
  Ogre::Vector3 mV2;
  Ogre::Vector3 mV3;
};

/**
 * This class syncs Ogre::Meshes from the hardware and does ray intersection tests.
 */
class OgreMesh {
    class EntitySubmeshPair {
        void *mEntity;
        void *mSubmesh;
      public:
        EntitySubmeshPair() {
            mEntity=NULL;
            mSubmesh=NULL;
        }
        EntitySubmeshPair(Ogre::Entity*entity,Ogre::SubMesh*submesh){
            this->mEntity=entity;
            this->mSubmesh=submesh;
        }
        bool operator==(const EntitySubmeshPair&b)const {
            return mEntity==b.mEntity&&this->mSubmesh==b.mSubmesh;
        }
        bool operator<(const EntitySubmeshPair&b)const {
            if (mEntity==b.mEntity) return this->mSubmesh<b.mSubmesh;
            return mEntity<b.mEntity;
        }
    };
public:
  std::pair<bool, std::pair< double, Vector3f> > intersect(const Ogre::Ray &ray);
  OgreMesh(Ogre::Entity *entity, Ogre::SubMesh *submesh);
protected:
  void syncFromOgreMesh(Ogre::Entity *entity,  Ogre::SubMesh *mSubMesh);
  std::vector<Triangle> mTriangles;
public:
  int64 size()const;
};

} }

#endif
