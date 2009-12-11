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

struct TriVertex {
  Ogre::Vector3 coord;
  float u;
  float v;
  TriVertex(const Ogre::Vector3& coord, float u, float v)
    : coord(coord), u(u), v(v) {
  }
};
class Triangle;
struct IntersectResult {
	bool intersected;
	double distance;
	Vector3f normal;
	Triangle *tri;
	float u;
	float v;
	IntersectResult()
	: intersected(false),
	  distance(std::numeric_limits<Ogre::Real>::max()),
	  normal(Vector3f(0,0,0)),
	  tri(NULL),
	  u(0),
	  v(0) {}
};

class Triangle {
public:
  Triangle(const TriVertex &v1, const TriVertex &v2, const TriVertex &v3) : v1(v1), v2(v2), v3(v3)
  {
  }

  TriVertex v1, v2, v3;
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
  void  intersect(const Ogre::Ray &ray, IntersectResult &res);
  OgreMesh(Ogre::Entity *entity, Ogre::SubMesh *submesh, bool texcoord);
protected:
  void syncFromOgreMesh(Ogre::Entity *entity,  Ogre::SubMesh *mSubMesh, bool texcoord);
  std::vector<Triangle> mTriangles;
public:
  int64 size()const;
};

} }

#endif
