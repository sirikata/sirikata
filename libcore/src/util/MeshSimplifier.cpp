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

#include <sirikata/core/util/MeshSimplifier.hpp>

namespace Sirikata {

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


void MeshSimplifier::simplify(std::tr1::shared_ptr<Sirikata::Meshdata> agg_mesh, uint32 numVerticesLeft) {
  //Go through all the triangles, getting the vertices they consist of.
  //Calculate the Q for all the vertices.  

  int totalVertices = 0;

  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    totalVertices += curGeometry.positions.size();

    for (uint32 j = 0; j < curGeometry.positions.size(); j++) {
        curGeometry.positionQs.push_back( Matrix4x4f::nil());
    }


    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {

        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        Vector3f& pos1 = curGeometry.positions[idx];
        Vector3f& pos2 = curGeometry.positions[idx2];
        Vector3f& pos3 = curGeometry.positions[idx3];

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

        curGeometry.positionQs[idx] += mat;
        curGeometry.positionQs[idx2] += mat;
        curGeometry.positionQs[idx3] += mat;
      }
    }
  }

  //Iterate through all vertex pairs. Calculate the cost, v'(Q1+Q2)v, for each vertex pair.
  std::priority_queue<QSlimStruct> vertexPairs;
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    BoundingSphere3f boundingSphere = curGeometry.aabb.toBoundingSphere();
    boundingSphere.mergeIn( BoundingSphere3f(boundingSphere.center(), boundingSphere.radius()*11.0/10.0));

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {


        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        Vector3f& pos1 = curGeometry.positions[idx];
        Vector3f& pos2 = curGeometry.positions[idx2];
        Vector3f& pos3 = curGeometry.positions[idx3];

        //vectors 1 and 2
        Matrix4x4f Q = curGeometry.positionQs[idx] + curGeometry.positionQs[idx2];
        Matrix4x4f Qbar(Vector4f(Q(0,0), Q(0,1), Q(0,2), Q(0,3)),
                        Vector4f(Q(0,1), Q(1,1), Q(1,2), Q(1,3)),
                        Vector4f(Q(0,2), Q(1,2), Q(2,2), Q(2,3)),
                        Vector4f(0,0,0,1),  Matrix4x4f::ROWS());


        Matrix4x4f Qbarinv;
        double det = invert(Qbarinv, Qbar);

        Vector4f vbar4f ;
        //std::cout << Qbar << " " << Qbarinv << " : inverted?\n";

        if (det == 0 ) {
          Vector3f vbar = (pos1+pos2)/2;
          vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
        }
        else {
          vbar4f = Qbarinv * Vector4f(0,0,0,1);
          Vector3f vbar = Vector3f(vbar4f.x, vbar4f.y, vbar4f.z);
          if ( !boundingSphere.contains(vbar) ) {
            //std::cout << "det != 0 " << vbar4f << " from " << pos1 << " and " << pos2 << " and Q=" <<  Q << "\n";
            Vector3f vbar = (pos1+pos2)/2;
            vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
          }
        }
        float cost = 1.0/vbar4f.dot(  Q * vbar4f );  // cost is inverted because priority-queue pops the maximum first (instead of the minimum).
        QSlimStruct qs(cost, i, j, k, QSlimStruct::ONE_TWO, Vector3f(vbar4f.x, vbar4f.y, vbar4f.z) );

        vertexPairs.push(qs);

        //vectors 2 and 3
        Q = curGeometry.positionQs[idx3] + curGeometry.positionQs[idx2];
        Qbar =Matrix4x4f(Vector4f(Q(0,0), Q(0,1), Q(0,2), Q(0,3)),
                        Vector4f(Q(0,1), Q(1,1), Q(1,2), Q(1,3)),
                        Vector4f(Q(0,2), Q(1,2), Q(2,2), Q(2,3)),
                        Vector4f(0,0,0,1),  Matrix4x4f::ROWS());

        det = invert(Qbarinv, Qbar);

        //std::cout << Qbar << " " << Qbarinv << " : inverted?\n";
        if (det == 0 ) {
          Vector3f vbar = (pos1+pos2)/2;
          vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
        }
        else {
          vbar4f = Qbarinv * Vector4f(0,0,0,1);
          Vector3f vbar = Vector3f(vbar4f.x, vbar4f.y, vbar4f.z);
          if ( !boundingSphere.contains(vbar) ) {
            //std::cout << "det != 0 " << vbar4f << " from " << pos1 << " and " << pos2 << " and Q=" <<  Q << "\n";
            vbar = (pos1+pos2)/2;
            vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
          }
        }
        cost = 1.0/vbar4f.dot(  Q * vbar4f );
        qs = QSlimStruct(cost, i, j, k, QSlimStruct::TWO_THREE, Vector3f(vbar4f.x, vbar4f.y, vbar4f.z));

        vertexPairs.push(qs);

        //vectors 1 and 3
        Q = curGeometry.positionQs[idx] + curGeometry.positionQs[idx3];
        Qbar =Matrix4x4f(Vector4f(Q(0,0), Q(0,1), Q(0,2), Q(0,3)),
                        Vector4f(Q(0,1), Q(1,1), Q(1,2), Q(1,3)),
                        Vector4f(Q(0,2), Q(1,2), Q(2,2), Q(2,3)),
                        Vector4f(0,0,0,1),  Matrix4x4f::ROWS());

        det = invert(Qbarinv, Qbar);
        //std::cout << Qbar << " " << Qbarinv << " : inverted?\n";

        if (det == 0 ) {
          Vector3f vbar = (pos1+pos2)/2;
          vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
        }
        else {
          vbar4f = Qbarinv * Vector4f(0,0,0,1);
          Vector3f vbar = Vector3f(vbar4f.x, vbar4f.y, vbar4f.z);
          if ( !boundingSphere.contains(vbar) ) {
            //std::cout << "det != 0 " << vbar4f << " from " << pos1 << " and " << pos2 << " and Q=" <<  Q << "\n";
            Vector3f vbar = (pos1+pos2)/2;
            vbar4f = Vector4f(vbar.x, vbar.y, vbar.z, 1);
          }
        }
        cost = 1.0/vbar4f.dot(  Q * vbar4f );
        qs = QSlimStruct(cost, i, j, k, QSlimStruct::ONE_THREE, Vector3f(vbar4f.x, vbar4f.y, vbar4f.z));

        vertexPairs.push(qs);
      }
    }
  }



  //Remove the least cost pair from the list of vertex pairs. Replace it with a new vertex.
  //Modify all triangles that had either of the two vertices to point to the new vertex.
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping1;

  int remainingVertices = totalVertices;
  while (remainingVertices > numVerticesLeft && vertexPairs.size() > 0) {
    const QSlimStruct& top = vertexPairs.top();
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

    SubMeshGeometry& curGeometry = agg_mesh->geometry[top.mGeomIdx];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[top.mGeomIdx];
    unsigned short idx = curGeometry.primitives[j].indices[k1];
    unsigned short idx2 = curGeometry.primitives[j].indices[k2];

    while (vertexMapping.find(idx) != vertexMapping.end()) {
      idx = vertexMapping[idx];
    }
    while (vertexMapping.find(idx2) != vertexMapping.end()) {
      idx2 = vertexMapping[idx2];
    }

    if (idx != idx2) {
      Vector3f& pos1 = curGeometry.positions[idx];
      Vector3f& pos2 = curGeometry.positions[idx2];
      pos1.x = top.mReplacementVector.x;
      pos1.y = top.mReplacementVector.y;
      pos1.z = top.mReplacementVector.z;

      remainingVertices--;
     
      vertexMapping[idx2] = idx;
    }


    if (remainingVertices % 10000 == 0)
      std::cout << remainingVertices << " : remainingVertices\n";

    vertexPairs.pop();
  }

  //remove unused vertices; get new mapping from previous vertex indices to new vertex indices in vertexMapping2;
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping2;

  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<SubMeshGeometry::TextureSet>texUVs;

    for (uint32 j = 0 ; j < curGeometry.positions.size(); j++) {
      if (vertexMapping.find(j) == vertexMapping.end()) {
        oldToNewMap[j] = positions.size();
        positions.push_back(curGeometry.positions[j]);
      }
      if (j < curGeometry.normals.size()) {
        normals.push_back(curGeometry.normals[j]);
      }
      if (j < curGeometry.texUVs.size()) {
        texUVs.push_back(curGeometry.texUVs[j]);
      }
    }

    curGeometry.positions = positions;
    curGeometry.normals = normals;
    curGeometry.texUVs = texUVs;
  }

  //remove degenerate triangles.
  for (uint32 i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping =  vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    for (uint32 j = 0; j < curGeometry.primitives.size(); j++) {
      std::vector<unsigned short> newPrimitiveList;
      for (uint32 k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {
        unsigned short idx = curGeometry.primitives[j].indices[k];
        unsigned short idx2 = curGeometry.primitives[j].indices[k+1];
        unsigned short idx3 = curGeometry.primitives[j].indices[k+2];

        while  (vertexMapping.find(idx) != vertexMapping.end()) {
          idx = vertexMapping[idx];
        }
        while (vertexMapping.find(idx2) != vertexMapping.end()) {
          idx2 = vertexMapping[idx2];
        }
        while (vertexMapping.find(idx3) != vertexMapping.end()) {
          idx3 = vertexMapping[idx3];
        }


        if (idx!=idx2 && idx2 != idx3 && idx3!=idx){
          newPrimitiveList.push_back( oldToNewMap[idx]);
          newPrimitiveList.push_back( oldToNewMap[idx2]);
          newPrimitiveList.push_back( oldToNewMap[idx3]);
        }

      }
      curGeometry.primitives[j].indices = newPrimitiveList;
    }
  }

}



}
