/*  Sirikata
 *  MeshSimplifier.cpp
 *
 *  Copyright (c) 2010, Tahir Azim.
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

#include <sirikata/mesh/MeshSimplifier.hpp>

#include <boost/functional/hash.hpp>

#include <sirikata/core/util/Timer.hpp>
#ifdef _WIN32
#include <float.h>
#else
#include <cmath>
#endif
#include <math.h>

#define SIMPLIFY_LOG(lvl, msg) SILOG(simplify, lvl, msg)

namespace Sirikata {

namespace Mesh {

#define SIMPLIFIER_INVALID_VECTOR Vector3f(-1000000,-1000000,-1000000)

Vector3f applyTransform(const Matrix4x4f& transform, const Vector3f& v) {
  Vector4f jth_vertex_4f = transform*Vector4f(v.x, v.y, v.z, 1.0f);
  return Vector3f(jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z);
}

class IndexedFaceContainer {
public:


  bool valid;
  uint32 idx1;
  uint32 idx2;
  uint32 idx3;

  IndexedFaceContainer(uint32 i, uint32 j, uint32 k) :
    valid(true), idx1(i), idx2(j), idx3(k)
  {
  }

};

class FaceContainer {
public:
  Vector3f v1;
  Vector3f v2;
  Vector3f v3;

  FaceContainer(Vector3f& v1, Vector3f& v2, Vector3f& v3) {
    this->v1 = v1;
    this->v2 = v2;
    this->v3 = v3;
  }

  FaceContainer() {
  }

  size_t hash() const {
      size_t seed = 0;

      size_t pos1Hash = v1.hash();
      size_t pos2Hash = v2.hash();
      size_t pos3Hash = v3.hash();

      std::vector<size_t> facevecs;
      seed = 0;
      facevecs.push_back(pos1Hash);facevecs.push_back(pos2Hash);facevecs.push_back(pos3Hash);
      sort(facevecs.begin(), facevecs.end());
      boost::hash_combine(seed, facevecs[0]);
      boost::hash_combine(seed, facevecs[1]);
      boost::hash_combine(seed, facevecs[2]);

      return seed;
  }

  bool operator==(const FaceContainer&other)const {
        return v1 == other.v1 && v2==other.v2 && v3==other.v3;
  }

  class Hasher{
  public:
        size_t operator() (const FaceContainer& g) const {
            return g.hash();
        }
  };

};


class GeomContainer {
public:
  uint32 geomIdx;
  uint32 vertexIdx;

  GeomContainer(int g, int v) {
    geomIdx = g;
    vertexIdx = v;
  }

  GeomContainer() {
    geomIdx = vertexIdx = 0;
  }

  size_t hash() const {
      size_t seed = 0;
      boost::hash_combine(seed, geomIdx);
      boost::hash_combine(seed, vertexIdx);

      return seed;
  }

  bool operator==(const GeomContainer&other)const {
        return geomIdx==other.geomIdx && vertexIdx==other.vertexIdx;
  }

  class Hasher{
  public:
        size_t operator() (const GeomContainer& g) const {
            return g.hash();
        }
  };

};

class GeomPairContainer {
public:
  uint32 mGeomIdx;
  uint32 mVertexIdx1;
  uint32 mVertexIdx2;

  float mCost;
  Vector3f mReplacementVector;

  void init(uint32 g, uint32 v1, uint32 v2) {
    mGeomIdx = g;
    mVertexIdx1 = v1;
    mVertexIdx2 = v2;

    if (mVertexIdx1 > mVertexIdx2) {
      uint32 temp = mVertexIdx1;
      mVertexIdx1 = mVertexIdx2;
      mVertexIdx2 = temp;
    }

    mCost = 0;
  }

  GeomPairContainer(uint32 g, uint32 v1, uint32 v2) {
    init(g,v1,v2);
  }

  GeomPairContainer(uint32 g, uint32 v1, uint32 v2, float cost)  {
    init(g,v1,v2);
    mCost = cost;
  }

  GeomPairContainer() {
    mGeomIdx = mVertexIdx1 = mVertexIdx2 = 0;
    mCost = 0;
  }

  size_t hash() const {
      size_t seed = 0;
      boost::hash_combine(seed, mGeomIdx);
      boost::hash_combine(seed, mVertexIdx1);
      boost::hash_combine(seed, mVertexIdx2);

      return seed;
  }

  bool operator==(const GeomPairContainer&other)const {
    return mGeomIdx==other.mGeomIdx && mVertexIdx1==other.mVertexIdx1 && mVertexIdx2==other.mVertexIdx2;
  }

  bool operator<(const GeomPairContainer&other) const {

    if (*this == other) return false;

    if (mCost == other.mCost) {
      if (mGeomIdx == other.mGeomIdx) {
        if (mVertexIdx1 == other.mVertexIdx1) {
          return mVertexIdx2 < other.mVertexIdx2;
        }

        return mVertexIdx1 < other.mVertexIdx1;
      }

      return mGeomIdx < other.mGeomIdx;
    }

    return mCost < other.mCost;
  }

  class Hasher{
  public:
        size_t operator() (const GeomPairContainer& g) const {
            return g.hash();
        }
  };

};
bool custom_isnan (double data) {
#ifdef _WIN32
    return _isnan(data);
#else
    return std::isnan(data);
#endif
}

void computeCosts(Mesh::MeshdataPtr agg_mesh,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher> >& submeshPositionQs,
                  std::set<GeomPairContainer>& vertexPairs,
                  std::tr1::unordered_map<GeomPairContainer, float, GeomPairContainer::Hasher>& pairPriorities,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > >& submeshNeighborVertices,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<uint32> > >& vertexToFacesMap
                 )
{
  
  for (uint32 i = 0;  i < agg_mesh->geometry.size(); i++) {

    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher>& positionQs = submeshPositionQs[i];
    std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> >& neighborVertices = submeshNeighborVertices[i];
    std::tr1::unordered_map<uint32, std::vector<uint32> >& submeshVertexToFacesMap = vertexToFacesMap[i];

    

    for (uint32 j = 0; j < curGeometry.positions.size(); j++) {
      
      if (neighborVertices.find(j) == neighborVertices.end()) continue;

      bool j_edgeVertex = (submeshVertexToFacesMap[j].size() == 1);

      const Vector3f& position = curGeometry.positions[j];

      Matrix4x4f Q = positionQs[position];      

      std::tr1::unordered_set<uint32>& neighbors = neighborVertices[j];

      for (std::tr1::unordered_set<uint32>::iterator neighbor_it = neighbors.begin(); neighbor_it != neighbors.end(); neighbor_it++) {
        uint32 neighbor = *neighbor_it;

        bool neighbor_edgeVertex = (submeshVertexToFacesMap[neighbor].size() == 1);

        double cost = 0;
        if (false/*j_edgeVertex || neighbor_edgeVertex*/) {
          cost = 1e15;
        }
        else {
          const Vector3f& neighborPosition = curGeometry.positions[neighbor];

          Matrix4x4f Q = positionQs[position] + positionQs[neighborPosition];

          Vector4f vbar4f (neighborPosition.x, neighborPosition.y, neighborPosition.z, 1);

          cost = (vbar4f.dot(  Q * vbar4f ));          

          if (cost == 0) {
            /*Deal properly with the case where all faces adjacent to a vertex are coplanar.
             While this face may have coplanar vertices in this instance, these vertices
             might connect with other faces in other instances. */
            /*Q = Q + Matrix4x4f(Vector4f(1e-10, 1e-10, 1e-10, 1e-10),
                               Vector4f(1e-10, 1e-10, 1e-10, 1e-10),
                               Vector4f(1e-10, 1e-10, 1e-10, 1e-10),
                               Vector4f(0.00000, 0.00000, 0.00000, 1), Matrix4x4f::ROWS() );  

             cost = (vbar4f.dot(  Q * vbar4f ));                        */
          }

          cost = (cost < 0.0) ? -cost : cost;          
        }

        /* From here to the end of this innermost loop, it takes almost 5 seconds */
        GeomPairContainer gpc(i, j, neighbor);
        gpc.mCost = pairPriorities[gpc];
        vertexPairs.erase(gpc);

        float32 origCost = gpc.mCost;
        gpc.mCost += cost;
        pairPriorities[gpc] = gpc.mCost;
        vertexPairs.insert(gpc);
      }
    }
  }
}

inline uint32  findMappedVertex(std::tr1::unordered_map<int, int>& vertexMapping, uint32 idx) {
  uint32 startIdx = idx;

  int countIter = 0;
  while (vertexMapping.find(idx) != vertexMapping.end()) {
    countIter++;
    if ((int)idx == vertexMapping[idx]) {
      break;
    }
    idx = vertexMapping[idx];
  }

  if (countIter > 1) {
    vertexMapping[startIdx] = idx;
  }

  return idx;
}

void computeCosts(Mesh::MeshdataPtr agg_mesh, uint32 geomIdx, uint32 sourcePositionIdx, uint32 targetPositionIdx,
                  std::tr1::unordered_map<int, int>& vertexMapping,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher> >& submeshPositionQs,
                  std::set<GeomPairContainer>& vertexPairs,
                  std::tr1::unordered_map<GeomPairContainer, float, GeomPairContainer::Hasher>& pairPriorities,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > >& submeshNeighborVertices,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<uint32> > >& vertexToFacesMap)
{
  std::tr1::unordered_set<GeomPairContainer, GeomPairContainer::Hasher> pairPrioritiesSeen;

  SubMeshGeometry& curGeometry = agg_mesh->geometry[geomIdx];
  std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher>& positionQs = submeshPositionQs[geomIdx];
  std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> >& neighborVertices = submeshNeighborVertices[geomIdx];
  std::tr1::unordered_map<uint32, std::vector<uint32> >& submeshVertexToFacesMap = vertexToFacesMap[geomIdx];

  targetPositionIdx = findMappedVertex(vertexMapping, targetPositionIdx);

  const Vector3f& position = curGeometry.positions[targetPositionIdx];

  std::tr1::unordered_set<uint32>& neighbors = neighborVertices[targetPositionIdx];

  bool is_edgeVertex = (submeshVertexToFacesMap[geomIdx].size() == 1);

  for (std::tr1::unordered_set<uint32>::iterator neighbor_it = neighbors.begin(); neighbor_it != neighbors.end(); neighbor_it++) {
    uint32 neighborIdx = *neighbor_it;

    neighborIdx = findMappedVertex(vertexMapping, neighborIdx);

    if (neighborIdx == targetPositionIdx) continue;

    bool neighbor_edgeVertex = (submeshVertexToFacesMap[neighborIdx].size() == 1);

    float cost = 0;
    if (false /*is_edgeVertex || neighbor_edgeVertex */) {
      cost = 1e15;
    }
    else {
      const Vector3f& neighborPosition = curGeometry.positions[neighborIdx];

      Matrix4x4f Q = positionQs[position] + positionQs[neighborPosition];

      Vector4f vbar4f (neighborPosition.x, neighborPosition.y, neighborPosition.z, 1);
      cost = (vbar4f.dot(  Q * vbar4f )) ;
      
      if (cost == 0) {
         /*Deal properly with the case where all faces adjacent to a vertex are coplanar.
           While this face may have coplanar vertices in this instance, these vertices
           might connect with other faces in other instances. */
         /*Q = Q + Matrix4x4f(Vector4f(1e-10, 1e-10, 1e-10, 1e-10),
                              Vector4f(1e-10, 1e-10, 1e-10, 1e-10),
                              Vector4f(1e-10, 1e-10, 1e-10, 1e-10),
                              Vector4f(0.0000000, 0.0000000, 0.0000000, 1), Matrix4x4f::ROWS() );

         cost = (vbar4f.dot(  Q * vbar4f )) ;*/
      }

      cost = (cost < 0.0) ? -cost : cost;
    }

    GeomPairContainer gpc(geomIdx, sourcePositionIdx, neighborIdx);
    gpc.mCost = pairPriorities[gpc];
    vertexPairs.erase(gpc);

    gpc = GeomPairContainer(geomIdx, targetPositionIdx, neighborIdx);

    gpc.mCost = pairPriorities[gpc];
    vertexPairs.erase(gpc);

    float prevCost = gpc.mCost;

    if (pairPrioritiesSeen.count(gpc) == 0) {
      pairPriorities[gpc] = 0;
      pairPrioritiesSeen.insert(gpc);
    }

    gpc.mCost = pairPriorities[gpc];

    gpc.mCost += cost;

    pairPriorities[gpc] = gpc.mCost;
    vertexPairs.insert(gpc);
  }
}

void MeshSimplifier::simplify(Mesh::MeshdataPtr agg_mesh, int32 numFacesLeft) {
  std::tr1::unordered_map<uint32, uint32> submeshInstanceCount;

  int countFaces = 0;
  uint32 geoinst_idx;
  Matrix4x4f geoinst_pos_xform;
  Meshdata::GeometryInstanceIterator geoinst_it = agg_mesh->getGeometryInstanceIterator();

  std::set<GeomPairContainer> vertexPairs;
  std::tr1::unordered_map<GeomPairContainer, float, GeomPairContainer::Hasher> pairPriorities;

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher> > submeshPositionQs;

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > > submeshNeighborVertices;

  /* Make every index in prims specification point to the earliest occurrence of the corresponding position vector */
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];

    std::tr1::unordered_map<Vector3f, uint32, Vector3f::Hasher> firstPositionMap;

    std::tr1::unordered_set<uint32> deletedIndices;
    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

      for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
        unsigned short idx = primitive.indices[k];
        unsigned short idx2 = primitive.indices[k+1];
        unsigned short idx3 = primitive.indices[k+2];

        Vector3f& pos1 = curGeometry.positions[idx];
        Vector3f& pos2 = curGeometry.positions[idx2];
        Vector3f& pos3 = curGeometry.positions[idx3];

        if (firstPositionMap.find(pos1) == firstPositionMap.end()) {
          firstPositionMap[pos1] = idx;
        }
        else {
          if (idx != firstPositionMap[pos1]) {
            primitive.indices[k] = firstPositionMap[pos1];
            deletedIndices.insert(idx);
          }
        }

        if (firstPositionMap.find(pos2) == firstPositionMap.end()) {
          firstPositionMap[pos2] = idx2;
        }
        else {
           if (idx2 != firstPositionMap[pos2]) {
             primitive.indices[k+1] = firstPositionMap[pos2];
             deletedIndices.insert(idx2);
           }
        }

        if (firstPositionMap.find(pos3) == firstPositionMap.end()) {
          firstPositionMap[pos3] = idx3;
        }
        else {
          if (idx3 != firstPositionMap[pos3]) {
            primitive.indices[k+2] = firstPositionMap[pos3];
            deletedIndices.insert(idx3);
          }
        }
      }
    }

    for (std::tr1::unordered_set<uint32>::iterator it = deletedIndices.begin();
          it != deletedIndices.end(); it++)
    {
      curGeometry.positions[*it] = SIMPLIFIER_INVALID_VECTOR;
    }
  }

  std::tr1::unordered_map<uint32, std::vector<IndexedFaceContainer> > geomFacesList;
  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<uint32> > > vertexToFacesMap;

  /* Identify the non-unique faces in the geometry. Insert the vertex pairs */
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];

    std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> >& neighborVertices = submeshNeighborVertices[i];
    std::tr1::unordered_map<uint32, std::vector<uint32> >& geomsVertexToFacesMap = vertexToFacesMap[i];
    std::vector<IndexedFaceContainer>& facesList = geomFacesList[i];

    std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher>& positionQs = submeshPositionQs[i];

    std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher> duplicateFaces;
    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
        SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

        for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
          unsigned short idx = primitive.indices[k];
          unsigned short idx2 = primitive.indices[k+1];
          unsigned short idx3 = primitive.indices[k+2];

          if (idx == idx2 || idx == idx3 || idx2 == idx3)
            continue;

          Vector3f& pos1 = curGeometry.positions[idx];
          Vector3f& pos2 = curGeometry.positions[idx2];
          Vector3f& pos3 = curGeometry.positions[idx3];

          FaceContainer origface(pos1, pos2, pos3);

          if (duplicateFaces.find(origface) != duplicateFaces.end()) {
            primitive.indices[k] = USHRT_MAX;
            primitive.indices[k+1] = USHRT_MAX;
            primitive.indices[k+2] = USHRT_MAX;

            continue;
          }

          GeomPairContainer gpc1(i, idx, idx2);
          GeomPairContainer gpc2(i, idx2, idx3);
          GeomPairContainer gpc3(i, idx, idx3);

          neighborVertices[idx].insert(idx2);
          neighborVertices[idx].insert(idx3);
          neighborVertices[idx2].insert(idx);
          neighborVertices[idx2].insert(idx3);
          neighborVertices[idx3].insert(idx);
          neighborVertices[idx3].insert(idx2);

          vertexPairs.insert(gpc1);
          pairPriorities[gpc1] = 0;

          vertexPairs.insert(gpc2);
          pairPriorities[gpc2] = 0;

          vertexPairs.insert(gpc3);
          pairPriorities[gpc3] = 0;

          duplicateFaces.insert(origface);

          IndexedFaceContainer ifc(idx, idx2, idx3);
          uint32 faceIndex = facesList.size();
          facesList.push_back(ifc);

          geomsVertexToFacesMap[idx].push_back(faceIndex);
          geomsVertexToFacesMap[idx2].push_back(faceIndex);
          geomsVertexToFacesMap[idx3].push_back(faceIndex);
        }
    }
  }


  //Find the list of instances associated with each submesh
  countFaces = 0;
  
  while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    const GeometryInstance& geomInstance = agg_mesh->instances[geoinst_idx];
    Matrix4x4f geoinst_pos_xform_transpose = geoinst_pos_xform.transpose();

    int geomIdx = geomInstance.geometryIndex;
    submeshInstanceCount[geomIdx]++;

    SubMeshGeometry& curGeometry = agg_mesh->geometry[geomIdx];
    std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher>& positionQs = submeshPositionQs[geomIdx];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

      for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
        unsigned short idx = primitive.indices[k];
        unsigned short idx2 = primitive.indices[k+1];
        unsigned short idx3 = primitive.indices[k+2];

        if (idx == USHRT_MAX && idx2 == USHRT_MAX && idx3 == USHRT_MAX) {
          continue;
        }

        if (idx == idx2 || idx == idx3 || idx2 == idx3) continue;

        Vector3f& orig_pos1 =  curGeometry.positions[idx];
        Vector3f& orig_pos2 =  curGeometry.positions[idx2];
        Vector3f& orig_pos3 = curGeometry.positions[idx3];

        Vector3f pos1 = geoinst_pos_xform * orig_pos1;
        Vector3f pos2 = geoinst_pos_xform * orig_pos2;
        Vector3f pos3 = geoinst_pos_xform * orig_pos3;

        double A = pos1.y*(pos2.z - pos3.z) + pos2.y*(pos3.z - pos1.z) + pos3.y*(pos1.z - pos2.z); A*=1000000.0;
        double B = pos1.z*(pos2.x - pos3.x) + pos2.z*(pos3.x - pos1.x) + pos3.z*(pos1.x - pos2.x);B*=1000000.0;
        double C = pos1.x*(pos2.y - pos3.y) + pos2.x*(pos3.y - pos1.y) + pos3.x*(pos1.y - pos2.y);C*=1000000.0;
        double D = ( pos1.x*(pos3.y*pos2.z - pos2.y*pos3.z ) +
                    pos2.x*(pos1.y*pos3.z - pos3.y*pos1.z ) +
                    pos3.x*(pos2.y*pos1.z - pos1.y*pos2.z) );        D*=1000000.0;

        double normalizer = sqrt(A*A+B*B+C*C);

        if (normalizer > -1e-10 && normalizer < 1e-10) {
          A = B = C = D = 0;          
        }
        else {
          //normalize...
          A  = A / normalizer;
          B  = B / normalizer;
          C  = C / normalizer;
          D  = D / normalizer;
        }                
        
        Matrix4x4f Qmat ( Vector4f(A*A, A*B, A*C, A*D),
                          Vector4f(A*B, B*B, B*C, B*D),
                          Vector4f(A*C, B*C, C*C, C*D),
                          Vector4f(A*D, B*D, C*D, D*D), Matrix4x4f::ROWS() );        
 
        Qmat = geoinst_pos_xform_transpose * Qmat * geoinst_pos_xform; //2 seconds on the large scene for this mult        

        double face_area = fabs(pos1.x*(pos2.y - pos3.y) + pos2.x*(pos3.y - pos1.y) + pos3.x*(pos1.y - pos2.y));
        face_area /= 2.0;
        Qmat *= face_area;

        positionQs[orig_pos1] += Qmat;
        positionQs[orig_pos2] += Qmat;
        positionQs[orig_pos3] += Qmat;        

        countFaces++;
      }
    }
  }    

  SIMPLIFY_LOG(warn, "countFaces = " << countFaces);
  SIMPLIFY_LOG(warn, "numFacesLeft = " << numFacesLeft);
  if (numFacesLeft < countFaces) {
      SIMPLIFY_LOG(warn, "numFacesLeft < countFaces: Simplification needed");
  }  
  else {
    return;
  }

  computeCosts(agg_mesh, submeshPositionQs, vertexPairs, pairPriorities, submeshNeighborVertices, vertexToFacesMap);

  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping1;

  //Do the actual edge collapses.
  while (countFaces > numFacesLeft && vertexPairs.size() > 0) {
    GeomPairContainer top = *(vertexPairs.begin());

    int i = top.mGeomIdx;

    uint32 targetIdx = top.mVertexIdx1;
    uint32 sourceIdx = top.mVertexIdx2;

    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];

    targetIdx = findMappedVertex(vertexMapping, targetIdx);
    sourceIdx = findMappedVertex(vertexMapping, sourceIdx);

    Vector3f& pos1 = curGeometry.positions[targetIdx];
    Vector3f& pos2 = curGeometry.positions[sourceIdx];

    //Collapse vertex at sourceIdx into targetIdx.
    if (targetIdx != sourceIdx && pos1 != pos2) {
      vertexMapping[sourceIdx] = targetIdx;      
      
      // FIXME: Can add a boundary edge check here by checking if countfaces reduced by 1 or more than 1.
      // If 1, its a boundary edge.

      std::tr1::unordered_map<uint32, std::vector<uint32> >& geomsVertexToFacesMap = vertexToFacesMap[i];
      std::vector<uint32>& ifcVector = geomsVertexToFacesMap[sourceIdx];
      std::vector<IndexedFaceContainer>& facesList = geomFacesList[i];

      //count how many faces get invalidated and degenerate because of this edge collapse.
      for (uint32 ifcVectorIdx = 0; ifcVectorIdx < ifcVector.size(); ifcVectorIdx++) {
        uint32 faceIndex = ifcVector[ifcVectorIdx];
        IndexedFaceContainer& ifc = facesList[faceIndex];

        if (!ifc.valid) continue;

        unsigned short vidx = ifc.idx1;
        unsigned short vidx2 = ifc.idx2;
        unsigned short vidx3 = ifc.idx3;

        vidx = findMappedVertex(vertexMapping, vidx);
        vidx2 = findMappedVertex(vertexMapping, vidx2);
        vidx3 = findMappedVertex(vertexMapping, vidx3);

        if (vidx == vidx2 || vidx2 == vidx3 || vidx == vidx3) {
          //degenerate face; invalidate it.
          countFaces -= submeshInstanceCount[i];
          ifc.valid = false;
        }
        else {
          //add this face to the neighbors of the target vertex.
          geomsVertexToFacesMap[targetIdx].push_back(faceIndex);
        }
      }
      //done invalidating degenerate faces.

      //Now update the neighbors of the target vertex and its quadric matrix.
      std::tr1::unordered_set<uint32>& neighborVertices2 =  submeshNeighborVertices[i][sourceIdx];
      std::tr1::unordered_set<uint32>& neighborVertices1 =  submeshNeighborVertices[i][targetIdx];
      std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher>& positionQs = submeshPositionQs[i];
      for (std::tr1::unordered_set<uint32>::iterator neighbor_it = neighborVertices2.begin();
           neighbor_it != neighborVertices2.end(); neighbor_it++)
      {
        uint32 neighborIdx = *neighbor_it;

        if (neighborIdx != targetIdx) {
          neighborVertices1.insert(neighborIdx);
        }
      }

      positionQs[ pos1 ] += positionQs[pos2];

      //Finally recompute the costs of the neighbors.
      computeCosts(agg_mesh, i, sourceIdx, targetIdx, vertexMapping, 
                   submeshPositionQs, vertexPairs, pairPriorities, submeshNeighborVertices,
                   vertexToFacesMap);
    }

    vertexPairs.erase(top);
    pairPriorities.erase(top);
  }


  //remove unused vertices; get new mapping from previous vertex indices to new vertex indices in vertexMapping2;
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int> > vertexMapping2;

  //Remove vertices no longer used in the simplified mesh.
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<SubMeshGeometry::TextureSet>texUVs;

    for (uint32 j = 0; j < curGeometry.texUVs.size(); j++) {
      SubMeshGeometry::TextureSet ts;
      ts.stride = curGeometry.texUVs[j].stride;
      texUVs.push_back(ts);
    }

    std::tr1::unordered_map<Vector3f, uint32, Vector3f::Hasher> vector3fSet;

    for (uint32 j = 0 ; j < curGeometry.positions.size() ; j++) {
      if (vertexMapping.find(j) == vertexMapping.end()) {

        if (curGeometry.positions[j] == SIMPLIFIER_INVALID_VECTOR) {
          continue;
        }

        if (vector3fSet.find(curGeometry.positions[j]) != vector3fSet.end()) {
          oldToNewMap[j] = vector3fSet[ curGeometry.positions[j] ];
          continue;
        }

        oldToNewMap[j] = positions.size();

        vector3fSet[ curGeometry.positions[j] ] = positions.size();

        positions.push_back(curGeometry.positions[j]);

        if (j < curGeometry.normals.size())
          normals.push_back(curGeometry.normals[j]);

        for (uint32 k = 0; k < curGeometry.texUVs.size(); k++) {
          unsigned int stride = curGeometry.texUVs[k].stride;
          if (stride*j < curGeometry.texUVs[k].uvs.size()) {
            uint32 idx = stride * j;
            while ( idx < stride*j+stride){
              texUVs[k].uvs.push_back(curGeometry.texUVs[k].uvs[idx]);
              idx++;
            }
          }
        }
      }
    }

    curGeometry.positions = positions;
    curGeometry.normals = normals;
    curGeometry.texUVs = texUVs;
  }

  //Now adjust the primitives to point to the new indexes of the submesh geometry vertices.
  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<unsigned short> > > newIndices;

  //First create new indices from all the non-degenerate faces.
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];

    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      std::vector<unsigned short>& indices = newIndices[i][j];

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        if (idx == USHRT_MAX && idx2 == USHRT_MAX && idx3 == USHRT_MAX) {
          continue;
        }

        idx = findMappedVertex(vertexMapping, idx);
        idx2 = findMappedVertex(vertexMapping, idx2);
        idx3 = findMappedVertex(vertexMapping, idx3);

        if (idx != idx2 && idx2 != idx3 && idx != idx3) {
          indices.push_back(oldToNewMap[idx]);
          indices.push_back(oldToNewMap[idx2]);
          indices.push_back(oldToNewMap[idx3]);
        }
      }
    }
  }

  //Now set the new indices.
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];

    for (uint32 j = 0 ; j < curGeometry.primitives.size() ; j++) {
      curGeometry.primitives[j].indices = newIndices[i][j];
    }
  }
}

}

}
