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

namespace Sirikata {

namespace Mesh {

/* Code from an Intel matrix inversion optimization report
   (ftp://download.intel.com/design/pentiumiii/sml/24504301.pdf) */
double MeshSimplifier::invert(Matrix4x4f& inv, const Matrix4x4f& orig)
{
  float mat[16];
  float dst[16];

  int counter = 0;
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      mat[counter] = orig(i,j);
      counter++;
    }
  }

  float tmp[12]; /* temp array for pairs */
  float src[16]; /* array of transpose source matrix */
  float det; /* determinant */
  /* transpose matrix */
  for (int i = 0; i < 4; i++) {
    src[i] = mat[i*4];
    src[i + 4] = mat[i*4 + 1];
    src[i + 8] = mat[i*4 + 2];
    src[i + 12] = mat[i*4 + 3];
  }
  /* calculate pairs for first 8 elements (cofactors) */
  tmp[0] = src[10] * src[15];
  tmp[1] = src[11] * src[14];
  tmp[2] = src[9] * src[15];
  tmp[3] = src[11] * src[13];
  tmp[4] = src[9] * src[14];
  tmp[5] = src[10] * src[13];
  tmp[6] = src[8] * src[15];
  tmp[7] = src[11] * src[12];
  tmp[8] = src[8] * src[14];
  tmp[9] = src[10] * src[12];
  tmp[10] = src[8] * src[13];
  tmp[11] = src[9] * src[12];
  /* calculate first 8 elements (cofactors) */
  dst[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
  dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
  dst[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
  dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
  dst[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
  dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
  dst[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
  dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
  dst[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
  dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
  dst[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
  dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
  dst[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
  dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
  dst[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
  dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
  /* calculate pairs for second 8 elements (cofactors) */
  tmp[0] = src[2]*src[7];
  tmp[1] = src[3]*src[6];
  tmp[2] = src[1]*src[7];
  tmp[3] = src[3]*src[5];
  tmp[4] = src[1]*src[6];
  tmp[5] = src[2]*src[5];
  tmp[6] = src[0]*src[7];
  tmp[7] = src[3]*src[4];
  tmp[8] = src[0]*src[6];
  tmp[9] = src[2]*src[4];
  tmp[10] = src[0]*src[5];
  tmp[11] = src[1]*src[4];
  /* calculate second 8 elements (cofactors) */
  dst[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
  dst[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
  dst[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
  dst[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
  dst[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
  dst[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
  dst[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
  dst[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
  dst[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
  dst[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
  dst[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
  dst[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
  dst[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
  dst[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
  dst[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
  dst[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
  /* calculate determinant */
  det=src[0]*dst[0]+src[1]*dst[1]+src[2]*dst[2]+src[3]*dst[3];

  if (det == 0.0) return 0.0;

  /* calculate matrix inverse */
  det = 1/det;
  for (int j = 0; j < 16; j++)
    dst[j] *= det;

  counter = 0;
  for (int i=0; i<4; i++) {
    for (int j=0; j<4; j++) {
      inv(i,j) = dst[counter];
      counter++;
    }
  }

  return det;
}

Vector3f applyTransform(const Matrix4x4f& transform, const Vector3f& v) {
  Vector4f jth_vertex_4f = transform*Vector4f(v.x, v.y, v.z, 1.0f);
  return Vector3f(jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z);
}

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

void computeCosts(std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& positionVectors,                  
                  std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<Vector3f, Vector3f::Hasher>, Vector3f::Hasher >& neighborVertices,
                  std::tr1::unordered_map<GeomPairContainer, float, GeomPairContainer::Hasher>& pairPriorities,
                  std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher>& positionQs,
                  std::tr1::unordered_map<Vector3f, std::tr1::unordered_map<Vector3f, std::vector<GeomPairContainer>, Vector3f::Hasher >, Vector3f::Hasher>& overallPositionPairs,
                  std::tr1::unordered_map<Vector3f, Vector3f, Vector3f::Hasher>& overallPositionMapping,
                  std::set<GeomPairContainer>& vertexPairs)
{
  std::tr1::unordered_set<GeomPairContainer, GeomPairContainer::Hasher> pairPrioritiesSeen;

  for (std::tr1::unordered_set<Vector3f, Vector3f::Hasher>::iterator it = positionVectors.begin();
       it != positionVectors.end(); it++) 
  {
    Vector3f origPosition = *it;
    Vector3f position = origPosition;
    
    while (overallPositionMapping.find(position) != overallPositionMapping.end()) { 
      if (overallPositionMapping[position] == position) break;
      
      position = overallPositionMapping[position];
    }

    std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& neighbors = neighborVertices[position];

    for ( std::tr1::unordered_set<Vector3f>::iterator n_it = neighbors.begin(); n_it != neighbors.end(); n_it++) {
      Vector3f origNeighborPosition = *n_it;
      Vector3f neighborPosition = origNeighborPosition;

      while (overallPositionMapping.find(neighborPosition) != overallPositionMapping.end()) {
        if (overallPositionMapping[neighborPosition] == neighborPosition) break;
        
        neighborPosition = overallPositionMapping[neighborPosition];
      }

      if (position == neighborPosition) continue;

      Matrix4x4f Q = positionQs[position] + positionQs[neighborPosition];
      Vector4f vbar4f (neighborPosition.x, neighborPosition.y, neighborPosition.z, 1);
      float cost = abs(vbar4f.dot(  Q * vbar4f )) ;
      cost = ((cost <= 1e-12 || isnan(cost)) ? 1e15:cost);

      std::vector<GeomPairContainer>& opp =  overallPositionPairs[origPosition][origNeighborPosition];
      
      for (std::vector<GeomPairContainer>::iterator s_it = opp.begin(); s_it != opp.end();s_it++) {
            GeomPairContainer gpc(s_it->mGeomIdx, s_it->mVertexIdx1, s_it->mVertexIdx2 );
            gpc.mCost = pairPriorities[gpc];     

            if ( vertexPairs.find(gpc) == vertexPairs.end()) continue;

            vertexPairs.erase(gpc);

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
  }
}

void MeshSimplifier::simplify(Mesh::MeshdataPtr agg_mesh, int32 numVerticesLeft) {
  Sirikata::Time curTime = Sirikata::Timer::now();
 
  std::tr1::unordered_map<GeomPairContainer, float, GeomPairContainer::Hasher> pairPriorities;
  //Maps a position to its Q value
  std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher> positionQs;
  std::tr1::unordered_map<GeomContainer, std::tr1::unordered_set<Vector3f, Vector3f::Hasher>, GeomContainer::Hasher> geometryIndexToPositionsMap;
  std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<GeomContainer, GeomContainer::Hasher>, Vector3f::Hasher> positionToGeomsMap;
  
  std::tr1::unordered_set<Vector3f, Vector3f::Hasher> positionVectors; 

  std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher> faceSet;

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<uint32>, Vector3f::Hasher > > duplicateGeomVectors;

  std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<Vector3f, Vector3f::Hasher>, Vector3f::Hasher> neighborVertices;
    
  Meshdata::GeometryInstanceIterator geoinst_it = agg_mesh->getGeometryInstanceIterator();
  uint32 geoinst_idx;
  Matrix4x4f geoinst_pos_xform;

  uint32 totalVertices = 0;    

  std::set<GeomPairContainer> vertexPairs;

  std::tr1::unordered_set<Vector3f, Vector3f::Hasher> removedPositions;

  //stores mapping of vertices collapsed to other vertices
  std::tr1::unordered_map<Vector3f, Vector3f, Vector3f::Hasher> overallPositionMapping;

  //maps pairs of points in the overall mesh to a list of point pairs in the underlying submesh geometries.
  std::tr1::unordered_map<Vector3f, std::tr1::unordered_map<Vector3f, std::vector<GeomPairContainer>, Vector3f::Hasher >, Vector3f::Hasher> overallPositionPairs;
  
  //Calculate the Qs for each vertex
  while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    
    const GeometryInstance& geomInstance = agg_mesh->instances[geoinst_idx];
    SubMeshGeometry& curGeometry = agg_mesh->geometry[geomInstance.geometryIndex];
    int i = geomInstance.geometryIndex;

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {    

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];            

        Vector3f pos1 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx]);
        Vector3f pos2 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx2]);
        Vector3f pos3 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx3]);

        duplicateGeomVectors[i][ curGeometry.positions[idx]  ].insert(idx);
        duplicateGeomVectors[i][ curGeometry.positions[idx2] ].insert(idx2);
        duplicateGeomVectors[i][ curGeometry.positions[idx3] ].insert(idx3);

        /*** AVOID TAKING THE SAME FACE INTO CONSIDERATION REPEATEDLY   **/
        FaceContainer face(pos1,pos2,pos3);
        if (faceSet.find(face) != faceSet.end()) {
          continue;
        }
        faceSet.insert(FaceContainer(pos1,pos2,pos3));       
        /** END DUPLICATE FACE AVOIDANCE **/

        GeomPairContainer gpc1(i, idx, idx2);
        GeomPairContainer gpc2(i, idx2, idx3);
        GeomPairContainer gpc3(i, idx, idx3);

        vertexPairs.insert(gpc1);
        pairPriorities[gpc1] = 0;
        overallPositionPairs[pos1][pos2].push_back(gpc1);

        vertexPairs.insert(gpc2);
        pairPriorities[gpc2] = 0;        
        overallPositionPairs[pos2][pos3].push_back(gpc2);

        vertexPairs.insert(gpc3);
        pairPriorities[gpc3] = 0;
        overallPositionPairs[pos1][pos3].push_back(gpc3);        

        neighborVertices[pos1].insert(pos2);
        neighborVertices[pos1].insert(pos3);
        neighborVertices[pos2].insert(pos1);
        neighborVertices[pos2].insert(pos3);
        neighborVertices[pos3].insert(pos1);
        neighborVertices[pos3].insert(pos2);
                
        GeomContainer gc1(i, idx);
        GeomContainer gc2(i, idx2);
        GeomContainer gc3(i, idx3);

        geometryIndexToPositionsMap[gc1].insert(  pos1  );
        geometryIndexToPositionsMap[gc2].insert(  pos2  );
        geometryIndexToPositionsMap[gc3].insert(  pos3  );

        positionVectors.insert(pos1);
        positionVectors.insert(pos2);
        positionVectors.insert(pos3);

        positionToGeomsMap[pos1].insert( gc1 );
        positionToGeomsMap[pos2].insert( gc2 );
        positionToGeomsMap[pos3].insert( gc3 );

        double A = pos1.y*(pos2.z - pos3.z) + pos2.y*(pos3.z - pos1.z) + pos3.y*(pos1.z - pos2.z);
        double B = pos1.z*(pos2.x - pos3.x) + pos2.z*(pos3.x - pos1.x) + pos3.z*(pos1.x - pos2.x);
        double C = pos1.x*(pos2.y - pos3.y) + pos2.x*(pos3.y - pos1.y) + pos3.x*(pos1.y - pos2.y);
        double D = -1 *( pos1.x*(pos2.y*pos3.z - pos3.y*pos2.z) + pos2.x*(pos3.y*pos1.z - pos1.y*pos3.z) + pos3.x*(pos1.y*pos2.z - pos2.y*pos1.z) );

        double normalizer = sqrt(A*A+B*B+C*C);
        
        //normalize...
        A /= normalizer;
        B /= normalizer;
        C /= normalizer;
        D /= normalizer;

        Matrix4x4f mat ( Vector4f(A*A, A*B, A*C, A*D),
                         Vector4f(A*B, B*B, B*C, B*D),
                         Vector4f(A*C, B*C, C*C, C*D),
                         Vector4f(A*D, B*D, C*D, D*D), Matrix4x4f::ROWS() ); 

        positionQs[pos1] += mat;
        positionQs[pos2] += mat;
        positionQs[pos3] += mat;
      }
    }
  }

  std::cout << vertexPairs.size () << " : vertexPairs.size\n";

  totalVertices = positionQs.size();
  std::cout <<  agg_mesh->uri << " : agg_mesh->uri, " << totalVertices << " : totalVertices;\n";
  if (totalVertices <= numVerticesLeft) return;
  
  computeCosts(positionVectors, neighborVertices, pairPriorities, positionQs,  overallPositionPairs, overallPositionMapping, vertexPairs);

  //Remove the least cost pair from the list of vertex pairs. Replace it with a new vertex.
  //Modify all triangles that had either of the two vertices to point to the new vertex.
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping1;  

  while (totalVertices - removedPositions.size() > numVerticesLeft && vertexPairs.size() > 0) {
   // std::cout << vertexPairs.size() <<  " : vertexPairs.size\n";
    //std::cout << (totalVertices - removedPositions.size()) << " : totalVertices - removedPositions.size()\n";

    GeomPairContainer top = *(vertexPairs.begin());

    SubMeshGeometry& curGeometry = agg_mesh->geometry[top.mGeomIdx];
    int i = top.mGeomIdx;
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];

    uint32 idx = top.mVertexIdx1;
    uint32 idx2 = top.mVertexIdx2;

    while (vertexMapping.find(idx) != vertexMapping.end()) {
      if (idx == vertexMapping[idx]) {
        break;
      }
      idx = vertexMapping[idx];
    }
    while (vertexMapping.find(idx2) != vertexMapping.end()) {
      if (idx2 == vertexMapping[idx2]) {
        break;
      }
      idx2 = vertexMapping[idx2];
    }

    Vector3f& pos1 = curGeometry.positions[idx];
    Vector3f& pos2 = curGeometry.positions[idx2];

    if (idx != idx2 && pos1 != pos2) {
      Vector3f posToReplace = pos2;

      std::tr1::unordered_set<uint32> duplicateGeomSet = duplicateGeomVectors[i][pos2];
      /** REMOVE ALL POSITION VERTICES THAT ARE DUPLICATES OF THE VERTEX AT THIS SUB_MESH_GEOMETRY IDX  **/
      for (std::tr1::unordered_set<uint32>::iterator it = duplicateGeomSet.begin();
           it != duplicateGeomSet.end(); it++)
      {        
        uint32 curct = *it;
        while (vertexMapping.find(curct) != vertexMapping.end()) {          
          if (curct == vertexMapping[curct]) {
            break;
          }
          curct = vertexMapping[curct];
        }
        
        Vector3f& posct = curGeometry.positions[curct] ;
        
        if (posct == posToReplace) {
          posct.x = (pos1.x);
          posct.y = (pos1.y);
          posct.z = (pos1.z);

          vertexMapping[curct] = idx;

          GeomContainer geomContainer(i, curct);

          std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& posList = geometryIndexToPositionsMap[geomContainer];

          GeomContainer geomContainer2(i, idx);
          std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& targetPosList = geometryIndexToPositionsMap[geomContainer2];

          std::tr1::unordered_set<Vector3f, Vector3f::Hasher>::iterator it = posList.begin();

          for ( ; it != posList.end(); it++) {
            std::tr1::unordered_set<GeomContainer, GeomContainer::Hasher>& idxesSet = positionToGeomsMap[*it];
            idxesSet.erase(geomContainer);

            if (idxesSet.size() == 0) {
              // Removed position vertex *it
              removedPositions.insert(*it);

              std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& neighbors = neighborVertices[*it];

              Vector3f newNeighbor;
              bool newNeighborValid = false;
              for ( std::tr1::unordered_set<Vector3f>::iterator n_it = neighbors.begin();
                    n_it != neighbors.end(); n_it++)
              {
                if (targetPosList.find(*n_it) != targetPosList.end()) {
                  newNeighbor = *n_it;
                  positionQs[newNeighbor] += positionQs[*it];
                  overallPositionMapping[*it] = newNeighbor;
                  newNeighborValid = true;
      
                  break;
                }
              }

              if (newNeighborValid) {
                computeCosts(neighborVertices[*it], neighborVertices, pairPriorities, positionQs,  overallPositionPairs, overallPositionMapping, vertexPairs);
                computeCosts(neighborVertices[newNeighbor], neighborVertices, pairPriorities, positionQs, overallPositionPairs,overallPositionMapping, vertexPairs);
              }
            }
          }
        }
      }
      /** END REMOVE ... **/
    }

    vertexPairs.erase(top);
    pairPriorities.erase(top);
  }
  
  int remainingVertices = 0;

  //remove unused vertices; get new mapping from previous vertex indices to new vertex indices in vertexMapping2;
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int> > vertexMapping2;

  //Remove vertices no longer used in the simplified mesh
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<SubMeshGeometry::TextureSet>texUVs;

    for (uint32 j = 0 ; j < curGeometry.positions.size() ; j++) {
      if (vertexMapping.find(j) == vertexMapping.end()) {
        oldToNewMap[j] = positions.size();

        positions.push_back(curGeometry.positions[j]);      
      
        if (j < curGeometry.normals.size())
          normals.push_back(curGeometry.normals[j]);
      
        if (j < curGeometry.texUVs.size())
          texUVs.push_back(curGeometry.texUVs[j]);
      }      
    }
    
    curGeometry.positions = positions;
    curGeometry.normals = normals;
    curGeometry.texUVs = texUVs;
  }

  //Now adjust the primitives to point to the new indexes of the submesh geometry vertices.
  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<unsigned short> > > newIndices;
  
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
        
        while  (vertexMapping.find(idx) != vertexMapping.end()) {
          if (idx == vertexMapping[idx]) {
            break;
          }        
          
          idx = vertexMapping[idx];
        }
        while (vertexMapping.find(idx2) != vertexMapping.end()) {
          if (idx2 == vertexMapping[idx2]) {
            break;
          }
          
          idx2 = vertexMapping[idx2];
        }
        while (vertexMapping.find(idx3) != vertexMapping.end()) {
          if (idx3 == vertexMapping[idx3]) {
            break;
          }
          
          idx3 = vertexMapping[idx3];
        }

        if (idx != idx2 && idx2 != idx3 && idx != idx3) {
          indices.push_back(oldToNewMap[idx]);
          indices.push_back(oldToNewMap[idx2]);
          indices.push_back(oldToNewMap[idx3]);
        }
      }
    }
  }

  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];   

    for (uint32 j = 0 ; j < curGeometry.primitives.size() ; j++) {
      curGeometry.primitives[j].indices = newIndices[i][j];
    }
  }

  /// recalculate normals.
  /*
  std::tr1::unordered_map<GeomContainer, float, GeomContainer::Hasher> vertexFaceCount;
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {    
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];   

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {    

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        Vector3f& pos1 = curGeometry.positions[idx];
        Vector3f& pos2 =  curGeometry.positions[idx2];
        Vector3f& pos3 = curGeometry.positions[idx3];

        vertexFaceCount[GeomContainer(i,idx)]++;
        vertexFaceCount[GeomContainer(i,idx2)]++;
        vertexFaceCount[GeomContainer(i,idx3)]++;
        
        float normx = (pos1.z-pos2.z)*(pos3.y-pos2.y)-(pos1.y-pos2.y)*(pos3.z-pos2.z);
        float normy = (pos1.x-pos2.x)*(pos3.z-pos2.z)-(pos1.z-pos2.z)*(pos3.x-pos2.x);
        float normz = (pos1.y-pos2.y)*(pos3.x-pos2.x)-(pos1.x-pos2.x)*(pos3.y-pos2.y);
        float normlength = sqrt((normx*normx)+(normy*normy)+(normz*normz));
        normx /= normlength;
        normy /= normlength;
        normz /= normlength; 

        Vector3f normal(normx, normy, normz);                
        curGeometry.normals[idx] += normal;
        curGeometry.normals[idx2] += normal;
        curGeometry.normals[idx3] += normal;

        
      }
    }

    for (uint32 j = 0; j < curGeometry.normals.size(); j++) {
      curGeometry.normals[j] /= vertexFaceCount[GeomContainer(i,j)];
    }
  }
  */

  /** Count the remaining vertices and spit out the answer... just to be sure */
  std::tr1::unordered_set<Vector3f, Vector3f::Hasher> pq;
  geoinst_it = agg_mesh->getGeometryInstanceIterator();
  while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    const GeometryInstance& geomInstance = agg_mesh->instances[geoinst_idx];
    SubMeshGeometry& curGeometry = agg_mesh->geometry[geomInstance.geometryIndex];    

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      
      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        Vector3f pos1 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx]);
        Vector3f pos2 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx2]);
        Vector3f pos3 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx3]);        

        pq.insert(pos1);
        pq.insert(pos2);
        pq.insert(pos3);        
      }
    }
  }
  remainingVertices = pq.size();
  
  std::cout << (Timer::now() - curTime).toMilliseconds() <<" : time taken\n";

  std::cout << remainingVertices << " : remainingVertices\n";  
}

}

}

