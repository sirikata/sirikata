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



//#include <OpenMesh/Core/IO/MeshIO.hh>
//#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
//#include <OpenMesh/Core/Mesh/PolyConnectivity.hh>

#include <sirikata/core/util/Timer.hpp>

//typedef OpenMesh::TriMesh_ArrayKernelT<>  MyMesh;


namespace Sirikata {

namespace Mesh {

/* Code from an Intel matrix inversion optimization report
   (ftp://download.intel.com/design/pentiumiii/sml/24504301.pdf) */
double MeshSimplifier::invert(Matrix4x4f& inv, Matrix4x4f& orig)
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
  uint32 geomIdx;
  uint32 vertexIdx1;
  uint32 vertexIdx2;

  GeomPairContainer(uint32 g, uint32 v1, uint32 v2) {
    geomIdx = g;
    vertexIdx1 = v1;
    vertexIdx2 = v2;
  }

  GeomPairContainer() {
    geomIdx = vertexIdx1 = vertexIdx2 = 0;
  }

  size_t hash() const {
      size_t seed = 0;
      boost::hash_combine(seed, geomIdx);
      boost::hash_combine(seed, vertexIdx1);
      boost::hash_combine(seed, vertexIdx2);
      
      return seed;
  }

  bool operator==(const GeomPairContainer&other)const {
        return geomIdx==other.geomIdx && vertexIdx1==other.vertexIdx1 && vertexIdx2==other.vertexIdx2;
  }

  class Hasher{
  public:
        size_t operator() (const GeomPairContainer& g) const {
            return g.hash();
        }
  };

};

void MeshSimplifier::simplify(Mesh::MeshdataPtr agg_mesh, int32 numVerticesLeft) {
  using namespace Mesh;
  
  Sirikata::Time curTime = Sirikata::Timer::now();

  //MyMesh mesh;
  //std::tr1::unordered_map<Vector3f, MyMesh::VertexHandle, Vector3f::Hasher> positionHandles;
  
  //Maps a position to its Q value
  std::tr1::unordered_map<Vector3f, Matrix4x4f, Vector3f::Hasher> positionQs;
  std::tr1::unordered_map<GeomContainer, std::tr1::unordered_set<Vector3f, Vector3f::Hasher>, GeomContainer::Hasher> geometryIndexToPositionsMap;
  std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<GeomContainer, GeomContainer::Hasher>, Vector3f::Hasher> positionToGeomsMap;
  std::tr1::unordered_set<size_t> invalidFaceMap;
  std::tr1::unordered_map<uint32, std::tr1::unordered_map<Vector3f, std::tr1::unordered_set<uint32>, Vector3f::Hasher > > duplicateGeomVectors;
    
  Meshdata::GeometryInstanceIterator geoinst_it = agg_mesh->getGeometryInstanceIterator();
  uint32 geoinst_idx;
  Matrix4x4f geoinst_pos_xform;

  uint32 totalVertices = 0;
  
  std::tr1::unordered_set<size_t> seenFaces;
  
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

        size_t pos1Hash = pos1.hash();
        size_t pos2Hash = pos2.hash();
        size_t pos3Hash = pos3.hash();

        duplicateGeomVectors[i][ curGeometry.positions[idx]  ].insert(idx);
        duplicateGeomVectors[i][ curGeometry.positions[idx2] ].insert(idx2);
        duplicateGeomVectors[i][ curGeometry.positions[idx3] ].insert(idx3);


        /*** AVOID TAKING THE SAME FACE INTO CONSIDERATION REPEATEDLY   **/
        std::vector<size_t> facevecs;
        facevecs.push_back(pos1Hash);facevecs.push_back(pos2Hash);facevecs.push_back(pos3Hash);
        sort(facevecs.begin(), facevecs.end());
        if (seenFaces.find(facevecs[0] + facevecs[1] + facevecs[2]) != seenFaces.end()) 
        {
          continue;
        }
        seenFaces.insert(facevecs[0] + facevecs[1]+ facevecs[2]);
       
        /** END DUPLICATE FACE AVOIDANCE **/

        /** OpenMesh **
        if (positionHandles.find(pos1) == positionHandles.end()) {
          positionHandles[pos1] = mesh.add_vertex(MyMesh::Point(pos1.x, pos1.y, pos1.z));
        }
        if (positionHandles.find(pos2) == positionHandles.end()) {
          positionHandles[pos2] = mesh.add_vertex(MyMesh::Point(pos2.x, pos2.y, pos2.z));
        }
        if (positionHandles.find(pos3) == positionHandles.end()) {
          positionHandles[pos3] = mesh.add_vertex(MyMesh::Point(pos3.x, pos3.y, pos3.z));
        }
        
        std::vector<MyMesh::VertexHandle>  face_vhandles;        
        face_vhandles.clear();
        face_vhandles.push_back(positionHandles[pos1]);
        face_vhandles.push_back(positionHandles[pos2]);
        face_vhandles.push_back(positionHandles[pos3]);
        if ( mesh.add_face(face_vhandles) == OpenMesh::PolyConnectivity::InvalidFaceHandle) {
          invalidFaceMap.insert(facevecs[0] + facevecs[1]+ facevecs[2]);
        }
        **  OpenMesh **/
  
                
        GeomContainer gc1(i, idx);
        GeomContainer gc2(i, idx2);
        GeomContainer gc3(i, idx3);

        geometryIndexToPositionsMap[gc1].insert(  pos1  );
        geometryIndexToPositionsMap[gc2].insert(  pos2  );
        geometryIndexToPositionsMap[gc3].insert(  pos3  );

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


  totalVertices = positionQs.size();
  std::cout <<  agg_mesh->uri << " : agg_mesh->uri, " << totalVertices << " : totalVertices;\n";

  if (totalVertices <= numVerticesLeft) return;

  //Iterate through all vertex pairs. Calculate the cost, v'(Q1+Q2)v, for each vertex pair.
  std::priority_queue<QSlimStruct> vertexPairs;
  seenFaces.clear();

  std::tr1::unordered_map<GeomPairContainer, QSlimStruct, GeomPairContainer::Hasher> intermediateVertexPairs;

  geoinst_it = agg_mesh->getGeometryInstanceIterator();    
  
  while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    const GeometryInstance& geomInstance = agg_mesh->instances[geoinst_idx];
    SubMeshGeometry& curGeometry = agg_mesh->geometry[geomInstance.geometryIndex];
    

    uint32 i = geomInstance.geometryIndex;

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      if (curGeometry.primitives[j].primitiveType != SubMeshGeometry::Primitive::TRIANGLES) continue;

      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        if (idx3 < idx) {
          unsigned short temp = idx;
          idx = idx3;
          idx3 = temp;
        }

        if (idx2 < idx) {
          unsigned short temp = idx;
          idx = idx2;
          idx2 = temp;
        }

        if (idx3 < idx2) {
          unsigned short temp = idx3;
          idx3 = idx2;
          idx2 = temp;
        }

        Vector3f pos1 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx]);
        Vector3f pos2 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx2]);
        Vector3f pos3 = applyTransform(geoinst_pos_xform, curGeometry.positions[idx3]);

        /*if (mesh.is_boundary( positionHandles[pos1] ) || 
            mesh.is_boundary( positionHandles[pos2] ) ||
            mesh.is_boundary( positionHandles[pos3] ) )
          continue;*/

        size_t pos1Hash = pos1.hash();
        size_t pos2Hash = pos2.hash();
        size_t pos3Hash = pos3.hash();

        /*** AVOID TAKING THE SAME FACE INTO CONSIDERATION REPEATEDLY   **/
        std::vector<size_t> facevecs;
        facevecs.push_back(pos1Hash);facevecs.push_back(pos2Hash);facevecs.push_back(pos3Hash);
        sort(facevecs.begin(), facevecs.end());
        if (seenFaces.find(facevecs[0] + facevecs[1] + facevecs[2])!=
            seenFaces.end()) 
        {
          continue;
        }
        seenFaces.insert(facevecs[0] + facevecs[1]+ facevecs[2]);

        if (invalidFaceMap.find(facevecs[0] + facevecs[1]+ facevecs[2]) != invalidFaceMap.end())
        {
          continue;
        }
        /** END DUPLICATE FACE AVOIDANCE **/
       

        //vectors 1 and 2
        Vector4f vbar4f;
        Matrix4x4f Q = positionQs[pos1] + positionQs[pos2];
       
        vbar4f = Vector4f(pos2.x, pos2.y,pos2.z,1);
        float cost = abs(vbar4f.dot(  Q * vbar4f )) ; //problem here: every instance would have a 
                                                      //different target vertex
                                                      //after decimation.
        QSlimStruct qs( ((cost == 0) ? 1e-15 : cost), i, j, k, QSlimStruct::ONE_TWO, Vector3f(vbar4f.x, vbar4f.y, vbar4f.z) );
        GeomPairContainer gpc(i,idx,idx2);
        

        if (intermediateVertexPairs.find(gpc) == intermediateVertexPairs.end()) {            
            intermediateVertexPairs[gpc] = qs;
        }
        else {
          intermediateVertexPairs[gpc].mCost += qs.mCost;
        }
        

        //vectors 2 and 3
        Q = positionQs[pos3] + positionQs[pos2];
        
        vbar4f = Vector4f(pos3.x, pos3.y,pos3.z,1);
        cost = abs(vbar4f.dot(  Q * vbar4f )) ;
        qs = QSlimStruct( ((cost == 0) ? 1e-15:cost), i, j, k, QSlimStruct::TWO_THREE, Vector3f(vbar4f.x, vbar4f.y, vbar4f.z) );
        gpc = GeomPairContainer(i,idx2,idx3);


        if (intermediateVertexPairs.find(gpc) == intermediateVertexPairs.end()) {            
            intermediateVertexPairs[gpc] = qs;
        }
        else {
          intermediateVertexPairs[gpc].mCost += qs.mCost;
        }
        
        //vectors 1 and 3
        Q = positionQs[pos1] + positionQs[pos3];
       
        
        vbar4f = Vector4f(pos3.x, pos3.y,pos3.z,1);
        cost = abs(vbar4f.dot(  Q * vbar4f )) ;
        qs = QSlimStruct( ((cost == 0) ? 1e-15:cost), i, j, k, QSlimStruct::ONE_THREE, Vector3f(vbar4f.x, vbar4f.y, vbar4f.z) );
        gpc = GeomPairContainer(i,idx,idx3);

        if (intermediateVertexPairs.find(gpc) == intermediateVertexPairs.end()) {            
          intermediateVertexPairs[gpc] = qs;
        }
        else {
          intermediateVertexPairs[gpc].mCost += qs.mCost;
        } 
      }
    }
  }

  for (std::tr1::unordered_map<GeomPairContainer, QSlimStruct>::iterator it = intermediateVertexPairs.begin();
       it != intermediateVertexPairs.end(); it++)
    {
      vertexPairs.push(it->second);
    }

  std::cout << vertexPairs.size () << " : vertexPairs.size\n";

  //Remove the least cost pair from the list of vertex pairs. Replace it with a new vertex.
  //Modify all triangles that had either of the two vertices to point to the new vertex.
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping1;

  int remainingVertices = totalVertices;  

  while (remainingVertices > numVerticesLeft && vertexPairs.size() > 0) {
    const QSlimStruct& top = vertexPairs.top();

    SubMeshGeometry& curGeometry = agg_mesh->geometry[top.mGeomIdx];
    int i = top.mGeomIdx;
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];

    uint32 j = top.mPrimitiveIdx;
    uint32 k1 = top.mPrimitiveIndicesIdx;
    uint32 k2;

    switch(top.mCombination) {
    case QSlimStruct::ONE_TWO:
      k2 = k1+1;
      break;

    case QSlimStruct::TWO_THREE:
      k2 = k1 + 2;
      k1 = k1 + 1;

      break;

    case QSlimStruct::ONE_THREE:
      k2 = k1 + 2;
      break;
    }

    unsigned short idx = curGeometry.primitives[j].indices[k1];
    unsigned short idx2 = curGeometry.primitives[j].indices[k2];

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

      std::tr1::unordered_set<Vector3f, Vector3f::Hasher> removedPositions;

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

          std::tr1::unordered_set<Vector3f, Vector3f::Hasher>::iterator it = posList.begin();

          for ( ; it != posList.end(); it++) {
            
            std::tr1::unordered_set<GeomContainer, GeomContainer::Hasher>& idxesSet = positionToGeomsMap[*it];
            idxesSet.erase(geomContainer);

            if (idxesSet.size() == 0) {
              //std::cout << "Removed position vertex " << it->second << "\n";
              removedPositions.insert(*it);
            }
          }
        }
      }

      remainingVertices -= removedPositions.size();
      /** END REMOVE ... **/
    }

    //int theoretic = remainingVertices;
    //    std::cout << remainingVertices << " : theoretic remainingVertices\n";

    vertexPairs.pop();
  }

  //std::cout << remainingVertices << " : guessed remainingVertices\n";

  remainingVertices = 0;

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
  std::tr1::unordered_set<Vector3f, Vector3f::Hasher> pq;
  geoinst_it = agg_mesh->getGeometryInstanceIterator();

  std::tr1::unordered_map<uint32, std::tr1::unordered_map<uint32, std::vector<unsigned short> > > newIndices;

  while ( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
    const GeometryInstance& geomInstance = agg_mesh->instances[geoinst_idx];
    SubMeshGeometry& curGeometry = agg_mesh->geometry[geomInstance.geometryIndex];
    int i = geomInstance.geometryIndex;
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
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

          Vector3f pos1 = applyTransform(geoinst_pos_xform, curGeometry.positions[oldToNewMap[idx]]);
          Vector3f pos2 = applyTransform(geoinst_pos_xform, curGeometry.positions[oldToNewMap[idx2]]);
          Vector3f pos3 = applyTransform(geoinst_pos_xform, curGeometry.positions[oldToNewMap[idx3]]);
          
          pq.insert(pos1);
          pq.insert(pos3);
          pq.insert(pos2);
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

  

  /*for (std::tr1::unordered_map<Vector3f, Matrix4x4f>::iterator it = positionQs.begin();
    it != positionQs.end(); it++) 
  {
    if (pq.find(it->first) == pq.end()) {
    std::cout << "Eliminated position " << it->first << "\n";
    }
  }*/
  
    
  remainingVertices = pq.size();

  
  std::cout << (Timer::now() - curTime).toMilliseconds() <<" : time taken\n";

  std::cout << remainingVertices << " : remainingVertices\n";

}

} //namespace Mesh
} //namespace Sirikata



/*Qbar =Matrix4x4f(Vector4f(Q(0,0), Q(0,1), Q(0,2), Q(0,3)),
  Vector4f(Q(0,1), Q(1,1), Q(1,2), Q(1,3)),
  Vector4f(Q(0,2), Q(1,2), Q(2,2), Q(2,3)),
  Vector4f(0,0,0,1), Matrix4x4f::ROWS());

  det = invert(Qbarinv, Qbar);
        
  if (det <= 1e-2 ) {
  Vector3f vbar = (pos1+pos2)/2;
  vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
  }
  else {
  vbar4f = Qbarinv * Vector4f(0,0,0,1);
  Vector3f vbar = Vector3f(vbar4f.x, vbar4f.y, vbar4f.z);          
  }*/
