#ifndef _OGRE_MESH_HPP_
#define _OGRE_MESH_HPP_

#include <vector>
#include "meruCompat/MeruDefs.hpp"
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
  TriVertex() {}
  TriVertex(const Ogre::Vector3& coord, float u, float v)
    : coord(coord), u(u), v(v) {
  }
};

struct Triangle {
  Triangle() {}
  Triangle(const TriVertex &v1, const TriVertex &v2, const TriVertex &v3) : v1(v1), v2(v2), v3(v3)
  {
  }

  TriVertex v1, v2, v3;
};

struct IntersectResult {
	bool intersected;
	double distance;
	Vector3f normal;
	Triangle tri;
	float u;
	float v;
	IntersectResult()
	: intersected(false),
	  distance(std::numeric_limits<Ogre::Real>::max()),
	  normal(Vector3f(0,0,0)),
	  u(0),
	  v(0) {}
};

/**
 * This class syncs Ogre::Meshes from the hardware and does ray intersection tests.
 */
class OgreMesh {
public:
  static Ogre::Ray transformRay(Ogre::Node *entity, const Ogre::Ray &original);
  static bool intersectTri(const Ogre::Ray &ray, IntersectResult &rtn, Triangle*itr, bool plane);
  void  intersect(Ogre::Node *node, const Ogre::Ray &ray, IntersectResult &res);
  OgreMesh(Ogre::SubMesh *submesh, bool texcoord);
protected:
  void syncFromOgreMesh(Ogre::SubMesh *mSubMesh, bool texcoord);
  std::vector<Triangle> mTriangles;
public:
  int64 size()const;
};

} }

#endif
