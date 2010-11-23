/*  Sirikata
 *  OgreMeshRaytrace.hpp
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

#ifndef _OGRE_MESH_HPP_
#define _OGRE_MESH_HPP_

#include <vector>
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
  OgreMesh(Ogre::SubMesh *submesh, bool texcoord, std::vector<TriVertex>&sharedVertices);
protected:
  void syncFromOgreMesh(Ogre::SubMesh *mSubMesh, bool texcoord, std::vector<TriVertex>&sharedVertices);
  std::vector<Triangle> mTriangles;
public:
  int64 size()const;
};

} }

#endif
