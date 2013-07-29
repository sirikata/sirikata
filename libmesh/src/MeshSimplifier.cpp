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
#include <iomanip>
#define SIMPLIFY_LOG(lvl, msg) SILOG(simplify, lvl, msg)

namespace Sirikata {

namespace Mesh {

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

  size_t hash() const {
      size_t seed = 0;

      std::vector<uint32> sortedIndices;
      sortedIndices.push_back(idx1);
      sortedIndices.push_back(idx2);
      sortedIndices.push_back(idx3);
      std::sort(sortedIndices.begin(), sortedIndices.end());

      seed = 0;
      boost::hash_combine(seed, sortedIndices[0]);
      boost::hash_combine(seed, sortedIndices[1]);
      boost::hash_combine(seed, sortedIndices[2]);

      return seed;
  }

  bool operator==(const IndexedFaceContainer&other) const {
    std::vector<uint32> sortedIndices;
    sortedIndices.push_back(idx1);
    sortedIndices.push_back(idx2);
    sortedIndices.push_back(idx3);
    std::sort(sortedIndices.begin(), sortedIndices.end());

    std::vector<uint32> sortedIndices2;
    sortedIndices2.push_back(other.idx1);
    sortedIndices2.push_back(other.idx2);
    sortedIndices2.push_back(other.idx3);
    std::sort(sortedIndices2.begin(), sortedIndices2.end());

    return (sortedIndices[0] == sortedIndices2[0]
            && sortedIndices[1] == sortedIndices2[1]
            && sortedIndices[2] == sortedIndices2[2] );
  }

  class Hasher{
  public:
        size_t operator() (const IndexedFaceContainer& g) const {
            return g.hash();
        }
  };


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
      facevecs.push_back(pos1Hash);facevecs.push_back(pos2Hash);facevecs.push_back(pos3Hash);
      sort(facevecs.begin(), facevecs.end());      

      seed = 0;
      boost::hash_combine(seed, facevecs[0]);
      boost::hash_combine(seed, facevecs[1]);
      boost::hash_combine(seed, facevecs[2]);

      return seed;
  }

  bool operator==(const FaceContainer&other)const {
    if (v1+v2+v3 != other.v1+other.v2+other.v3) return false;

    //There is a more elegant way to do this, but this is
    //easier to code and faster.
    if (v1 == other.v1) {
       if ((v2==other.v2 && v3==other.v3) ||
           (v2==other.v3 && v3==other.v2))
          return true;
    }

    if (v1 == other.v2) {
      if ((v2==other.v1 && v3==other.v3) ||
          (v2==other.v3 && v3==other.v1))
        return true;
    }

    if (v1 == other.v3) {
     if ((v2==other.v1 && v3==other.v2) ||
         (v2==other.v2 && v3==other.v1))
       return true;
    }

    return false;
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
  float64 mCost;
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

  GeomPairContainer(uint32 g, uint32 v1, uint32 v2, float64 cost)  {
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
          assert(mVertexIdx2 != other.mVertexIdx2);
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

bool optimize(Matrix4x4d& Q, const Vector3f& v11, const Vector3f& v21, Vector3f& best) {
    Vector3d v1(v11.x,v11.y,v11.z);
    Vector3d v2(v21.x,v21.y,v21.z);

    ///First compute cost of contracting to endpoint
    Vector4d vbar4f (v1.x, v1.y, v1.z, 1);
    float64 cost1 = vbar4f[0]*vbar4f[0]*Q(0,0) + 2*vbar4f[0]*vbar4f[1]*Q(0,1) + 2*vbar4f[0]*vbar4f[2]*Q(0,2) + 2*vbar4f[0]*Q(0,3)
      + vbar4f[1]*vbar4f[1]*Q(1,1) + 2*vbar4f[1]*vbar4f[2]*Q(1,2) + 2*vbar4f[1]*Q(1,3)
      + vbar4f[2]*vbar4f[2]*Q(2,2) + 2*vbar4f[2]*Q(2,3)
      + Q(3,3);
    cost1 = (cost1 < 0.0) ? -cost1 : cost1;

    vbar4f = Vector4d (v2.x, v2.y, v2.z, 1);
    float64 cost2 = vbar4f[0]*vbar4f[0]*Q(0,0) + 2*vbar4f[0]*vbar4f[1]*Q(0,1) + 2*vbar4f[0]*vbar4f[2]*Q(0,2) + 2*vbar4f[0]*Q(0,3)
      + vbar4f[1]*vbar4f[1]*Q(1,1) + 2*vbar4f[1]*vbar4f[2]*Q(1,2) + 2*vbar4f[1]*Q(1,3)
      + vbar4f[2]*vbar4f[2]*Q(2,2) + 2*vbar4f[2]*Q(2,3)
      + Q(3,3);
    cost2 = (cost2 < 0.0) ? -cost2 : cost2;

    //Now find cost of contracting to an "optimal" vertex
    Vector3d d = v1 - v2;
    Matrix3x3d A(Vector3d(Q(0,0), Q(0,1), Q(0,2)),
                 Vector3d(Q(0,1), Q(1,1), Q(1,2)),
                 Vector3d(Q(0,2), Q(1,2), Q(2,2)),
                 ROWS());       //tensor of Q;

    Vector3d Av2 = A*v2;
    Vector3d Ad  = A*d;

    float64 denom = 2.0*(d.dot(Ad));
    if (   denom <= 1e-12 ) {
      if (cost2 < cost1) best = v21;
      else best = v11;

      return false;
    }

    Vector3d vec(Q(3,0),Q(3,1),Q(3,2))  ;
    double a =  ( -2.0*(vec.dot(d)) - (d.dot(Av2)) - (v2.dot(Ad)) ) / ( 2.0*(d.dot(Ad)) );

    if( a<0.0 ) a=0.0; else if( a>1.0 ) a=1.0;

    Vector3d best64 = a*d + v2;
    best.x = best64.x; best.y = best64.y; best.z = best64.z;

    //Optimal found: now find cost of contracting to it
    vbar4f = Vector4d (best.x, best.y, best.z, 1);
    float64 cost3 = vbar4f[0]*vbar4f[0]*Q(0,0) + 2*vbar4f[0]*vbar4f[1]*Q(0,1) + 2*vbar4f[0]*vbar4f[2]*Q(0,2) + 2*vbar4f[0]*Q(0,3)
      + vbar4f[1]*vbar4f[1]*Q(1,1) + 2*vbar4f[1]*vbar4f[2]*Q(1,2) + 2*vbar4f[1]*Q(1,3)
      + vbar4f[2]*vbar4f[2]*Q(2,2) + 2*vbar4f[2]*Q(2,3)
      + Q(3,3);
    
    if (cost1<cost2 && cost1<cost3){
      best = v11;
    }
    else if (cost2<cost1 && cost2<cost3){
      best = v21;
    }

    return true;
}

void computeCosts(Mesh::MeshdataPtr agg_mesh,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, Matrix4x4d> >& submeshPositionQs,
                  std::set<GeomPairContainer>& vertexPairs,
                  std::tr1::unordered_map<GeomPairContainer, float64, GeomPairContainer::Hasher>& pairPriorities,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > >& submeshNeighborVertices,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<uint32> > >& vertexToFacesMap,
                  std::tr1::unordered_map<GeomPairContainer, uint32, GeomPairContainer::Hasher>& pairFrequency
                 )
{

  for (uint32 i = 0;  i < agg_mesh->geometry.size(); i++) {

    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<uint32, Matrix4x4d>& positionQs = submeshPositionQs[i];
    std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> >& neighborVertices = submeshNeighborVertices[i];
    std::tr1::unordered_map<uint32, std::vector<uint32> >& submeshVertexToFacesMap = vertexToFacesMap[i];

    for (uint32 j = 0; j < curGeometry.positions.size(); j++) {
      
      if (neighborVertices.find(j) == neighborVertices.end()) continue;

      const Vector3f& position = curGeometry.positions[j];

      Matrix4x4d Q = positionQs[j];

      std::tr1::unordered_set<uint32>& neighbors = neighborVertices[j];

      for (std::tr1::unordered_set<uint32>::iterator neighbor_it = neighbors.begin(); neighbor_it != neighbors.end(); neighbor_it++) {
        uint32 neighbor = *neighbor_it;

        bool neighbor_edgeVertex = (submeshVertexToFacesMap[neighbor].size() == 1);

        Vector3f best;
        
        const Vector3f& neighborPosition = curGeometry.positions[neighbor];
        Matrix4x4d Q = positionQs[j] + positionQs[neighbor];

        optimize(Q, position, neighborPosition, best);
        
        Vector4d vbar4f (best.x, best.y, best.z, 1);

        float64 cost = vbar4f[0]*vbar4f[0]*Q(0,0) + 2*vbar4f[0]*vbar4f[1]*Q(0,1) + 2*vbar4f[0]*vbar4f[2]*Q(0,2) + 2*vbar4f[0]*Q(0,3)
          + vbar4f[1]*vbar4f[1]*Q(1,1) + 2*vbar4f[1]*vbar4f[2]*Q(1,2) + 2*vbar4f[1]*Q(1,3)
          + vbar4f[2]*vbar4f[2]*Q(2,2) + 2*vbar4f[2]*Q(2,3)
          + Q(3,3);
        cost = (cost < 0.0) ? -cost : cost;
        
        uint32 idx1 = j, idx2 = neighbor;
        if (best == position) {
          uint32 temp = idx1;
          idx1 = idx2;
          idx2 = temp;
        }

        GeomPairContainer gpc(i, idx1, idx2);

        //Insert this cost as the cost of the edge if the edge hasnt been inserted 
        //before or if this cost is less than the previously computed cost.
        if (pairPriorities[gpc] == -1 || (pairPriorities[gpc] != -1 && cost < pairPriorities[gpc]) ) { 
          gpc.mCost = pairPriorities[gpc];
          vertexPairs.erase(gpc);

          gpc.mReplacementVector = best;

          gpc.mCost = cost;
          pairPriorities[gpc] = gpc.mCost;
          vertexPairs.insert(gpc);
        }
      }
    }
  }
}

inline uint32 findMappedVertex(std::tr1::unordered_map<int, int>& vertexMapping, uint32 idx) {
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
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, Matrix4x4d> >& submeshPositionQs,
                  std::set<GeomPairContainer>& vertexPairs,
                  std::tr1::unordered_map<GeomPairContainer, float64, GeomPairContainer::Hasher>& pairPriorities,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > >& submeshNeighborVertices,
                  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<uint32> > >& vertexToFacesMap,
                  std::tr1::unordered_map<GeomPairContainer, uint32, GeomPairContainer::Hasher>& pairFrequency
                  )
{
  
  SubMeshGeometry& curGeometry = agg_mesh->geometry[geomIdx];
  std::tr1::unordered_map<uint32, Matrix4x4d>& positionQs = submeshPositionQs[geomIdx];
  std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> >& neighborVertices = submeshNeighborVertices[geomIdx];
  std::tr1::unordered_map<uint32, std::vector<uint32> >& submeshVertexToFacesMap = vertexToFacesMap[geomIdx];

  targetPositionIdx = findMappedVertex(vertexMapping, targetPositionIdx);

  const Vector3f& position = curGeometry.positions[targetPositionIdx];

  std::tr1::unordered_set<uint32>& neighbors = neighborVertices[targetPositionIdx];


  for (std::tr1::unordered_set<uint32>::iterator neighbor_it = neighbors.begin(); neighbor_it != neighbors.end(); neighbor_it++) {   
    uint32 neighborIdx = *neighbor_it;

    neighborIdx = findMappedVertex(vertexMapping, neighborIdx);

    if (neighborIdx == targetPositionIdx) continue;

    uint32 idx1 = targetPositionIdx;
    uint32 idx2 = neighborIdx;

    float64 cost = 0;        
    const Vector3f& neighborPosition = curGeometry.positions[neighborIdx];
    Matrix4x4d Q = positionQs[targetPositionIdx] + positionQs[neighborIdx];

    Vector3f best;
    optimize(Q, position, neighborPosition, best);

    Vector4d vbar4f (best.x, best.y, best.z, 1);
    cost = vbar4f[0]*vbar4f[0]*Q(0,0) + 2*vbar4f[0]*vbar4f[1]*Q(0,1) + 2*vbar4f[0]*vbar4f[2]*Q(0,2) + 2*vbar4f[0]*Q(0,3)
            + vbar4f[1]*vbar4f[1]*Q(1,1) + 2*vbar4f[1]*vbar4f[2]*Q(1,2) + 2*vbar4f[1]*Q(1,3)
            + vbar4f[2]*vbar4f[2]*Q(2,2) + 2*vbar4f[2]*Q(2,3)
            + Q(3,3);
    cost = (cost < 0.0) ? -cost : cost;

    if (best == position) {
      uint32 temp = idx1;
      idx1 = idx2;
      idx2 = temp;
    }

    //Remove the previous vertex pair that existed before one vertex collapsed.
    GeomPairContainer gpc(geomIdx, sourcePositionIdx, neighborIdx);
    gpc.mCost = pairPriorities[gpc];
    vertexPairs.erase(gpc);
    gpc = GeomPairContainer(geomIdx, neighborIdx, sourcePositionIdx);
    gpc.mCost = pairPriorities[gpc];
    vertexPairs.erase(gpc);
    
    //Insert this vertex pair with the new cost, but first remove this pair if it already exists, then re-insert it.
    gpc = GeomPairContainer(geomIdx, idx1, idx2);
    gpc.mCost = pairPriorities[gpc];
    vertexPairs.erase(gpc);
    gpc = GeomPairContainer(geomIdx, idx2, idx1);
    gpc.mCost = pairPriorities[gpc];
    vertexPairs.erase(gpc);        

    gpc = GeomPairContainer(geomIdx, idx1, idx2);
    gpc.mCost = pairPriorities[gpc];

    gpc.mCost = cost;
    gpc.mReplacementVector = best;
    pairPriorities[gpc] = gpc.mCost;

    vertexPairs.insert(gpc); 
  }
}

void MeshSimplifier::simplify(Mesh::MeshdataPtr agg_mesh, int32 targetFaces) {
  std::tr1::unordered_map<uint32, uint32> submeshInstanceCount;

  int countFaces = 0;
  uint32 geoinst_idx;
  Matrix4x4f geoinst_pos_xform;
  Meshdata::GeometryInstanceIterator geoinst_it = agg_mesh->getGeometryInstanceIterator();

  std::set<GeomPairContainer> vertexPairs;
  std::tr1::unordered_map<GeomPairContainer, float64, GeomPairContainer::Hasher> pairPriorities;
  std::tr1::unordered_map<GeomPairContainer, uint32, GeomPairContainer::Hasher> pairFrequency; 

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, Matrix4x4d> > submeshPositionQs;

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > > submeshNeighborVertices;

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::tr1::unordered_set<uint32> > > submeshDuplicateIndices; 
  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, uint32> > submeshMapToFirstIndex;

  bool meshChangedDuringPreprocess = false;
  

  /* Make every index in prims specification point to the earliest occurrence of the corresponding position vector */
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];

    std::tr1::unordered_map<Vector3f, uint32, Vector3f::Hasher> firstPositionMap;

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
          submeshMapToFirstIndex[i][idx] = idx;
        }
        else {
          uint32 firstIdx = firstPositionMap[pos1];
          if (idx != firstIdx) {
            submeshDuplicateIndices[i][firstIdx].insert(idx);
            submeshMapToFirstIndex[i][idx] = firstIdx;
          }
        }

        if (firstPositionMap.find(pos2) == firstPositionMap.end()) {
          firstPositionMap[pos2] = idx2;
          submeshMapToFirstIndex[i][idx2] = idx2;
        }
        else {
           uint32 firstIdx = firstPositionMap[pos2];
           if (idx2 != firstIdx) {
             submeshDuplicateIndices[i][firstIdx].insert(idx2);
             submeshMapToFirstIndex[i][idx2] = firstIdx;
           }
        }

        if (firstPositionMap.find(pos3) == firstPositionMap.end()) {
          firstPositionMap[pos3] = idx3;
          submeshMapToFirstIndex[i][idx3] = idx3;
        }
        else {
          uint32 firstIdx = firstPositionMap[pos3];
          if (idx3 != firstIdx) {
            submeshDuplicateIndices[i][firstIdx].insert(idx3);
            submeshMapToFirstIndex[i][idx3] = firstIdx;
          }
        }
      }
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

    std::tr1::unordered_map<uint32, Matrix4x4d>& positionQs = submeshPositionQs[i];

    std::tr1::unordered_set<IndexedFaceContainer, IndexedFaceContainer::Hasher> duplicateFaces;
    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
        SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

        for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
          unsigned short idx = primitive.indices[k];
          unsigned short idx2 = primitive.indices[k+1];
          unsigned short idx3 = primitive.indices[k+2];

          idx = submeshMapToFirstIndex[i][idx];
          idx2 = submeshMapToFirstIndex[i][idx2];
          idx3 = submeshMapToFirstIndex[i][idx3];

          if (idx == idx2 || idx == idx3 || idx2 == idx3)
            continue;

          Vector3f& pos1 = curGeometry.positions[idx];
          Vector3f& pos2 = curGeometry.positions[idx2];
          Vector3f& pos3 = curGeometry.positions[idx3];

          IndexedFaceContainer origface(idx, idx2, idx3);
          uint32 faceIndex = facesList.size();
          facesList.push_back(origface);

          geomsVertexToFacesMap[idx].push_back(faceIndex);
          geomsVertexToFacesMap[idx2].push_back(faceIndex);
          geomsVertexToFacesMap[idx3].push_back(faceIndex);

          if (duplicateFaces.find(origface) != duplicateFaces.end()) continue;

          GeomPairContainer gpc1(i, idx, idx2, -1);
          GeomPairContainer gpc2(i, idx2, idx3, -1);
          GeomPairContainer gpc3(i, idx, idx3, -1);

          neighborVertices[idx].insert(idx2);
          neighborVertices[idx].insert(idx3);
          neighborVertices[idx2].insert(idx);
          neighborVertices[idx2].insert(idx3);
          neighborVertices[idx3].insert(idx);
          neighborVertices[idx3].insert(idx2);

          vertexPairs.insert(gpc1);
          pairPriorities[gpc1] = -1;
          pairFrequency[gpc1]++;

          vertexPairs.insert(gpc2);
          pairPriorities[gpc2] = -1;
          pairFrequency[gpc2]++;

          vertexPairs.insert(gpc3);
          pairPriorities[gpc3] = -1;
          pairFrequency[gpc3]++;

          duplicateFaces.insert(origface);
        }
    }
  }



  //Find the list of instances associated with each submesh
  countFaces = 0;
  
  while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    const GeometryInstance& geomInstance = agg_mesh->instances[geoinst_idx];
    Matrix4x4f geoinst_pos_xform_transpose = geoinst_pos_xform.transpose();
    Matrix4x4d transform;
    for (int row=0; row<4; row++) {
      for (int col=0; col<4; col++) {
        transform(row,col) = geoinst_pos_xform(row,col);        
      }
    } 
    
    int geomIdx = geomInstance.geometryIndex;
    submeshInstanceCount[geomIdx]++;


    SubMeshGeometry& curGeometry = agg_mesh->geometry[geomIdx];
    std::tr1::unordered_map<uint32, Matrix4x4d>& positionQs = submeshPositionQs[geomIdx];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      SubMeshGeometry::Primitive& primitive = curGeometry.primitives[j];

      for (uint32 k = 0; k+2 < primitive.indices.size(); k+=3) {
        unsigned short idx = primitive.indices[k];
        unsigned short idx2 = primitive.indices[k+1];
        unsigned short idx3 = primitive.indices[k+2];

        idx = submeshMapToFirstIndex[geomIdx][idx];
        idx2 = submeshMapToFirstIndex[geomIdx][idx2];
        idx3 = submeshMapToFirstIndex[geomIdx][idx3];

        if (idx == idx2 || idx == idx3 || idx2 == idx3) continue;
        countFaces++;

        Vector3d orig_pos1 (curGeometry.positions[idx].x, curGeometry.positions[idx].y,
                           curGeometry.positions[idx].z);
        Vector3d orig_pos2 (curGeometry.positions[idx2].x, curGeometry.positions[idx2].y,
                            curGeometry.positions[idx2].z);
        Vector3d orig_pos3 (curGeometry.positions[idx3].x, curGeometry.positions[idx3].y,
                            curGeometry.positions[idx3].z);                

        Vector3d pos1 = transform * orig_pos1;
        Vector3d pos2 = transform * orig_pos2;
        Vector3d pos3 = transform * orig_pos3;
        
        Vector3d normal = (pos2 - pos1).cross(pos3-pos1);
        normal = normal.normal();
        float64 A = normal[0];
        float64 B = normal[1];
        float64 C = normal[2];
        float64 D = -(normal.dot(pos1));

        Matrix4x4d Qmat ( Vector4d(A*A, A*B, A*C, A*D),
                          Vector4d(A*B, B*B, B*C, B*D),
                          Vector4d(A*C, B*C, C*C, C*D),
                          Vector4d(A*D, B*D, C*D, D*D), Matrix4x4d::ROWS() );
        
        Qmat = transform.transpose() * Qmat * transform;

        float64 face_area = (pos1-pos2).cross(pos1-pos3).length() * ((double)0.5);
        Qmat *= face_area;

        positionQs[idx] += Qmat;
        positionQs[idx2] += Qmat;
        positionQs[idx3] += Qmat;

        GeomPairContainer gpc1(geomIdx, idx, idx2, -1);
        GeomPairContainer gpc2(geomIdx, idx3, idx2, -1);
        GeomPairContainer gpc3(geomIdx, idx, idx3, -1);

        //Handle boundary edges adding the perpendicular constraint plane.
        //Implementation could be much much better though.
        if (pairFrequency[gpc1] == 1) {
          Vector3d org = pos1, dest = pos2;
          Vector3d e = dest - org;
          Vector3d constraint = e.cross(normal);
          constraint = constraint.normal();

          float64 A = constraint[0];
          float64 B = constraint[1];
          float64 C = constraint[2];
          float64 D = -(constraint.dot(org));

          Matrix4x4d Qmat ( Vector4d(A*A, A*B, A*C, A*D),
                          Vector4d(A*B, B*B, B*C, B*D),
                          Vector4d(A*C, B*C, C*C, C*D),
                          Vector4d(A*D, B*D, C*D, D*D), Matrix4x4d::ROWS() );
          //Qmat*=100.0;
          Qmat*=e.lengthSquared();

          Qmat=transform.transpose()*Qmat*transform; 
          positionQs[idx]+=Qmat;
          positionQs[idx2]+=Qmat;
        }
        if (pairFrequency[gpc2] == 1) {
          Vector3d org = pos3, dest = pos2;
          Vector3d e = dest - org;
          Vector3d constraint = e.cross(normal);
          constraint = constraint.normal();

          float64 A = constraint[0];
          float64 B = constraint[1];
          float64 C = constraint[2];
          float64 D = -(constraint.dot(org));

          Matrix4x4d Qmat ( Vector4d(A*A, A*B, A*C, A*D),
                          Vector4d(A*B, B*B, B*C, B*D),
                          Vector4d(A*C, B*C, C*C, C*D),
                          Vector4d(A*D, B*D, C*D, D*D), Matrix4x4d::ROWS() );
          //Qmat*=100.0;
          Qmat*=e.lengthSquared();

          Qmat=transform.transpose()*Qmat*transform;
          positionQs[idx3]+=Qmat;
          positionQs[idx2]+=Qmat;

        }
        if (pairFrequency[gpc3] == 1) {
          Vector3d org = pos1, dest = pos3;
          Vector3d e = dest - org;
          Vector3d constraint = e.cross(normal);
          constraint = constraint.normal();

          float64 A = constraint[0];
          float64 B = constraint[1];
          float64 C = constraint[2];
          float64 D = -(constraint.dot(org));

          Matrix4x4d Qmat ( Vector4d(A*A, A*B, A*C, A*D),
                          Vector4d(A*B, B*B, B*C, B*D),
                          Vector4d(A*C, B*C, C*C, C*D),
                          Vector4d(A*D, B*D, C*D, D*D), Matrix4x4d::ROWS() );
          //Qmat*=100.0;
          Qmat*=e.lengthSquared();

          Qmat=transform.transpose()*Qmat*transform;
          positionQs[idx]+=Qmat;
          positionQs[idx3]+=Qmat;
        } 
      }
    }
  }

  //targetFaces = countFaces/2.0;
  SIMPLIFY_LOG(warn, "countFaces = " << countFaces);
  SIMPLIFY_LOG(warn, "targetFaces = " << targetFaces);
  if (targetFaces < countFaces) {
      SIMPLIFY_LOG(warn, "targetFaces < countFaces: Simplification needed");
      computeCosts(agg_mesh, submeshPositionQs, vertexPairs, pairPriorities, submeshNeighborVertices, vertexToFacesMap, pairFrequency);
  }
  else if (!meshChangedDuringPreprocess) {
    return;
  }

  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping1;

  //Do the actual edge collapses.
  while (countFaces > targetFaces && vertexPairs.size() > 0) {
    GeomPairContainer top = *(vertexPairs.begin());   

    int i = top.mGeomIdx;
    uint32 targetIdx = top.mVertexIdx1;
    uint32 sourceIdx = top.mVertexIdx2;

    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];

    targetIdx = findMappedVertex(vertexMapping, targetIdx);
    sourceIdx = findMappedVertex(vertexMapping, sourceIdx);

    Vector3f& targetPos = curGeometry.positions[targetIdx];
    Vector3f& sourcePos = curGeometry.positions[sourceIdx];

    vertexMapping[sourceIdx] = targetIdx;
    assert( submeshMapToFirstIndex[i][sourceIdx] == sourceIdx);

    for (std::tr1::unordered_set<uint32>::iterator dup_it = submeshDuplicateIndices[i][sourceIdx].begin();
           dup_it != submeshDuplicateIndices[i][sourceIdx].end(); dup_it++)
    {
        vertexMapping[*dup_it] = targetIdx;
    }

    //Collapse vertex at sourceIdx into targetIdx.
    if (targetIdx != sourceIdx && targetPos != sourcePos) {
      
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
      std::tr1::unordered_set<uint32>& sourceNeighborVertices =  submeshNeighborVertices[i][sourceIdx];
      std::tr1::unordered_set<uint32>& targetNeighborVertices =  submeshNeighborVertices[i][targetIdx];
      std::tr1::unordered_map<uint32, Matrix4x4d>& positionQs = submeshPositionQs[i];
      for (std::tr1::unordered_set<uint32>::iterator neighbor_it = sourceNeighborVertices.begin();
           neighbor_it != sourceNeighborVertices.end(); neighbor_it++)
      {
        uint32 neighborIdx = *neighbor_it;

        if (neighborIdx != targetIdx) {
          targetNeighborVertices.insert(neighborIdx);
        }
      }

      positionQs[ targetIdx ] += positionQs[sourceIdx];

      curGeometry.positions[targetIdx] = top.mReplacementVector;
      for (std::tr1::unordered_set<uint32>::iterator dup_it = submeshDuplicateIndices[i][targetIdx].begin();
           dup_it != submeshDuplicateIndices[i][targetIdx].end(); dup_it++)
      {
        curGeometry.positions[*dup_it] = top.mReplacementVector;
      }

      //Finally recompute the costs of the neighbors.
      computeCosts(agg_mesh, i, sourceIdx, targetIdx, vertexMapping,
                   submeshPositionQs, vertexPairs, pairPriorities, submeshNeighborVertices,
                   vertexToFacesMap, pairFrequency);  
    }

    vertexPairs.erase(top);
    pairPriorities.erase(top);

  }


  //Now adjust the primitives to point to the new indexes of the submesh geometry vertices.
  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<unsigned short> > > newIndices;

  //First create new indices from all the non-degenerate faces.
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];

    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];

    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<SubMeshGeometry::TextureSet> texUVs;

    for (uint32 j = 0; j < curGeometry.texUVs.size(); j++) {
      SubMeshGeometry::TextureSet ts;
      ts.stride = curGeometry.texUVs[j].stride;
      texUVs.push_back(ts);
    }

    uint32 counter = 0;
    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      std::vector<unsigned short>& indices = newIndices[i][j];

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {
        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        idx = findMappedVertex(vertexMapping, idx);
        idx2 = findMappedVertex(vertexMapping, idx2);
        idx3 = findMappedVertex(vertexMapping, idx3);

        Vector3f pos1 = curGeometry.positions[idx];
        Vector3f pos2 = curGeometry.positions[idx2];
        Vector3f pos3 = curGeometry.positions[idx3];

        if (pos1 != pos2 && pos2 != pos3 && pos1 != pos3) { 
          indices.push_back(counter);
          indices.push_back(counter+1);
          indices.push_back(counter+2);
          counter += 3;

          positions.push_back(pos1);
          positions.push_back(pos2);
          positions.push_back(pos3);

          if (idx < curGeometry.normals.size())
            normals.push_back(curGeometry.normals[idx]);
          if (idx2 < curGeometry.normals.size())
            normals.push_back(curGeometry.normals[idx2]);
          if (idx3 < curGeometry.normals.size())
            normals.push_back(curGeometry.normals[idx3]);

          ///////////////////////////////////////////////////////
          for (uint32 k = 0; k < curGeometry.texUVs.size(); k++) {
            unsigned int stride = curGeometry.texUVs[k].stride;
            if (stride*idx < curGeometry.texUVs[k].uvs.size()) {
              uint32 idxuv = stride * idx;
              while ( idxuv < stride*idx+stride){
                texUVs[k].uvs.push_back(curGeometry.texUVs[k].uvs[idxuv]);
                idxuv++;
              }
            }
          }

          //////////////////////////////////////////////////////
          for (uint32 k = 0; k < curGeometry.texUVs.size(); k++) {
            unsigned int stride = curGeometry.texUVs[k].stride;
            if (stride*idx2 < curGeometry.texUVs[k].uvs.size()) {
              uint32 idxuv = stride * idx2;
              while ( idxuv < stride*idx2+stride){
                texUVs[k].uvs.push_back(curGeometry.texUVs[k].uvs[idxuv]);
                idxuv++;
              }
            }
          }

          //////////////////////////////////////////////////////
          for (uint32 k = 0; k < curGeometry.texUVs.size(); k++) {
            unsigned int stride = curGeometry.texUVs[k].stride;
            if (stride*idx3 < curGeometry.texUVs[k].uvs.size()) {
              uint32 idxuv = stride * idx3;
              while ( idxuv < stride*idx3+stride){
                texUVs[k].uvs.push_back(curGeometry.texUVs[k].uvs[idxuv]);
                idxuv++;
              }
            }
          }
 
          /////////////////////////////////////////////////////
        }
      }
    }


    curGeometry.positions = positions;
    curGeometry.normals = normals;
    curGeometry.texUVs = texUVs;
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
