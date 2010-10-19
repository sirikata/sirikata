#include "AggregateManager.hpp"

#include <sirikata/proxyobject/ModelsSystemFactory.hpp>

namespace Sirikata {

/* Code from an Intel matrix inversion optimization report
   (ftp://download.intel.com/design/pentiumiii/sml/24504301.pdf) */
double invert(Matrix4x4f& inv, Matrix4x4f& orig)
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


typedef struct QSlimStruct {
  float mCost;
  int mGeomIdx, mPrimitiveIdx, mPrimitiveIndicesIdx;
  enum VectorCombination {ONE_TWO, TWO_THREE, ONE_THREE} ;

  VectorCombination mCombination;
  
  Vector3f mReplacementVector;
  
  QSlimStruct(float cost, int i, int j, int k, VectorCombination c, Vector3f v) {
    mCost = cost;
    mGeomIdx = i;
    mPrimitiveIdx = j;
    mPrimitiveIndicesIdx = k;
    mCombination = c;
    mReplacementVector = v;
  }

  bool operator < (const QSlimStruct& qs) const {
    return mCost < qs.mCost;
  }

} QSlimStruct;

void simplify(std::tr1::shared_ptr<Meshdata> agg_mesh, uint numVerticesLeft) {
  //Go through all the triangles, getting the vertices they consist of. 
  //Calculate the Q for all the vertices.
  std::cout << agg_mesh->uri << " : agg_mesh->uri\n";

  int totalVertices = 0;  

  for (uint i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    totalVertices += curGeometry.positions.size();

    for (uint j = 0; j < curGeometry.positions.size(); j++) {
        curGeometry.positionQs.push_back( Matrix4x4f::nil());
    }

    
    for (uint j = 0; j < curGeometry.primitives.size(); j++) {      
      for (uint k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {

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
  for (uint i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    BoundingSphere3f boundingSphere = curGeometry.aabb.toBoundingSphere();
    boundingSphere.mergeIn( BoundingSphere3f(boundingSphere.center(), boundingSphere.radius()*11.0/10.0));

    for (uint j = 0; j < curGeometry.primitives.size(); j++) {
      for (uint k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {
        

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
    uint j = top.mPrimitiveIdx;
    uint k1 = top.mPrimitiveIndicesIdx;
    uint k2;

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

      /*if (idx2 < curGeometry.normals.size())
        curGeometry.normals.erase(curGeometry.normals.begin() + idx2);

      if (idx2 < curGeometry.texUVs.size())
        curGeometry.texUVs.erase(curGeometry.texUVs.begin() + idx2);
      */      
      
      vertexMapping[idx2] = idx;
    }
    

    if (remainingVertices % 1000 == 0)
      std::cout << remainingVertices << " : remainingVertices\n";
    
    vertexPairs.pop();    
  }

  //remove unused vertices; get new mapping from previous vertex indices to new vertex indices in vertexMapping2;
  std::tr1::unordered_map<int, std::tr1::unordered_map<int,int>  > vertexMapping2;    

  for (uint i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping = vertexMapping1[i];
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];
    
    std::vector<Sirikata::Vector3f> positions;
    std::vector<Sirikata::Vector3f> normals;
    std::vector<SubMeshGeometry::TextureSet>texUVs;
     
    for (uint j = 0 ; j < curGeometry.positions.size(); j++) {
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
  for (uint i = 0; i < agg_mesh->geometry.size(); i++) {
    SubMeshGeometry& curGeometry = agg_mesh->geometry[i];
    std::tr1::unordered_map<int, int>& vertexMapping =  vertexMapping1[i]; 
    std::tr1::unordered_map<int, int>& oldToNewMap = vertexMapping2[i];

    for (uint j = 0; j < curGeometry.primitives.size(); j++) {
      std::vector<unsigned short> newPrimitiveList;
      for (uint k = 0; k+2 < curGeometry.primitives[j].indices.size(); k+=3) {
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


AggregateManager::AggregateManager(SpaceContext* ctx, LocationService* loc) :
  mContext(ctx), mLoc(loc), mThreadRunning(true)
{
    mModelsSystem = NULL;
    if (ModelsSystemFactory::getSingleton().hasConstructor("colladamodels"))
        mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("colladamodels")(NULL, "");

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());

    static std::string x = "_1";
    mTransferPool = mTransferMediator->registerClient("SpaceAggregator"+x);
    x="_2";

    boost::thread thrd( boost::bind(&AggregateManager::uploadQueueServiceThread, this) );
}

AggregateManager::~AggregateManager() {
    delete mModelsSystem;

  mThreadRunning = false;

  mCondVar.notify_one();
}

void AggregateManager::addAggregate(const UUID& uuid) {
  std::cout << "addAggregate called: uuid=" << uuid.toString()  << "\n";  

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  mAggregateObjects[uuid] = std::tr1::shared_ptr<AggregateObject> (new AggregateObject(uuid, UUID::null()));
}

void AggregateManager::removeAggregate(const UUID& uuid) {
  std::cout << "removeAggregate: " << uuid.toString() << "\n";

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  mAggregateObjects.erase(uuid);
}

void AggregateManager::addChild(const UUID& uuid, const UUID& child_uuid) {
  std::vector<UUID>& children = getChildren(uuid);

  if ( std::find(children.begin(), children.end(), child_uuid) == children.end() ) {
    children.push_back(child_uuid);

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    if (mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      mAggregateObjects[child_uuid] = std::tr1::shared_ptr<AggregateObject> (new AggregateObject(child_uuid, uuid));
    }
    else {
      mAggregateObjects[child_uuid]->mParentUUID = uuid;
    }
    lock.unlock();

    String locationStr  = ( (mLoc->contains(child_uuid)) ? (mLoc->currentPosition(child_uuid).toString()) : " NOT IN LOC ");
    
    std::cout << "addChild: generateAggregateMesh called: "  << uuid.toString() 
              << " CHILD " << child_uuid.toString() << " "   << locationStr    << "\n";
    fflush(stdout);


    generateAggregateMesh(uuid, Duration::seconds(2400.0f+rand()%2400));
  }
}

void AggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
  std::vector<UUID>& children = getChildren(uuid);

  std::vector<UUID>::iterator it = std::find(children.begin(), children.end(), child_uuid);

  if (it != children.end()) {
    children.erase( it );

    String locationStr  = ( (mLoc->contains(child_uuid)) ? (mLoc->currentPosition(child_uuid).toString()) : " NOT IN LOC ");
    
    //std::cout << "removeChild: " <<  uuid.toString() << " CHILD " << child_uuid.toString() << " "   
    //          <<  locationStr
    //          << " generateAggregateMesh called\n";

    generateAggregateMesh(uuid, Duration::seconds(2400.0f+rand() % 2400));
  }
}

void AggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
  if (mModelsSystem == NULL) return;
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) return;  
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];  
  lock.unlock();
  aggObject->mLastGenerateTime = Timer::now();

  //std::cout << "Posted generateAggregateMesh for " << uuid.toString() << "\n";
  
  mContext->mainStrand->post( delayFor, std::tr1::bind(&AggregateManager::generateAggregateMeshAsync, this, uuid, aggObject->mLastGenerateTime)  );
}



Vector3f fixUp(int up, Vector3f v) {
    if (up==3) return Vector3f(v[0],v[2], -v[1]);
    else if (up==2) return v;
    std::cerr << "ERROR: X up? You gotta be frakkin' kiddin'\n";
    assert(false);
}

void AggregateManager::generateAggregateMeshAsync(const UUID uuid, Time postTime) {
  /* Get the aggregate object corresponding to UUID 'uuid'.  */
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
      std::cout << "0\n";
    return;
  }
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();
  /****/

  if (postTime < aggObject->mLastGenerateTime) {
    //std::cout << "1\n";fflush(stdout);
    return;
  }

  //  if (aggObject->mMeshdata) return;

  std::vector<UUID>& children = aggObject->mChildren;

  if (children.size() < 1) {
    std::cout << "2\n"; fflush(stdout);
    return;
  }


  if (!mLoc->contains(uuid)) { 
    std::cout << "3\n";
    generateAggregateMesh(uuid, Duration::milliseconds(10.0f));
    return;
  }

  for (uint i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];

    if (!mLoc->contains(child_uuid)) {
      generateAggregateMesh(uuid, Duration::milliseconds(10.0f)); 
      std::cout << "4\n";
      return;
    }

    std::string meshName = mLoc->mesh(child_uuid);

    if (meshName == "") {
      generateAggregateMesh(child_uuid, Duration::milliseconds(10.0f)); 
      std::cout << "5\n"; 
      return;
    }
  }

  std::tr1::shared_ptr<Meshdata> agg_mesh =  std::tr1::shared_ptr<Meshdata>( new Meshdata() );
  BoundingSphere3f bnds = mLoc->bounds(uuid);
  std::cout << uuid.toString() << "=uuid, "   << children.size() << " = children.size\n";
  
  uint   numAddedSubMeshGeometries = 0;
  double totalVertices = 0;
  for (uint i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];

    std::cout << child_uuid.toString() <<"=child_uuid\n";
    fflush(stdout);    

    Vector3f location = mLoc->currentPosition(child_uuid);
    Quaternion orientation = mLoc->currentOrientation(child_uuid);

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }    
    std::tr1::shared_ptr<Meshdata> m = mAggregateObjects[child_uuid]->mMeshdata;

    if (!m) {
      //request a download or generation of the mesh
      std::string meshName = mLoc->mesh(child_uuid);

      if (meshName != "") {

        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        if (mMeshStore.find(meshName) != mMeshStore.end()) {
          mAggregateObjects[child_uuid]->mMeshdata = m = mMeshStore[meshName];          
        }
        else {
          std::cout << meshName << " = meshName requesting download\n";
          Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, uuid, child_uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

          mTransferPool->addRequest(req);

          //once requested, wait until the mesh gets downloaded and loaded up;
          return;
        }
      }
    }

    lock.unlock();

    //    std::cout << "Aggregating mesh for " << child_uuid.toString() << "\n";
    agg_mesh->up_axis = m->up_axis;
    agg_mesh->lightInstances.insert(agg_mesh->lightInstances.end(),
                                    m->lightInstances.begin(),
                                    m->lightInstances.end() );
    agg_mesh->lights.insert(agg_mesh->lights.end(),
                            m->lights.begin(),
                            m->lights.end());

    for (Meshdata::URIMap::const_iterator tex_it = m->textureMap.begin();
         tex_it != m->textureMap.end(); tex_it++)
      {
        agg_mesh->textureMap[tex_it->first+"-"+child_uuid.toString()] = tex_it->second;
      }
        

    /** Find scaling factor **/
    BoundingBox3f3f originalMeshBoundingBox = BoundingBox3f3f::null();
    for (uint i = 0; i < m->instances.size(); i++) {
      const GeometryInstance& geomInstance = m->instances[i];
      SubMeshGeometry smg = m->geometry[geomInstance.geometryIndex];   

      for (uint j = 0; j < smg.positions.size(); j++) {
        Vector4f jth_vertex_4f =  geomInstance.transform*Vector4f(smg.positions[j].x,
                                                                  smg.positions[j].y,
                                                                  smg.positions[j].z,
                                                                  1.0f);
        Vector3f jth_vertex(jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z);
        jth_vertex = fixUp(m->up_axis, jth_vertex);                

        if (originalMeshBoundingBox == BoundingBox3f3f::null()) {
          originalMeshBoundingBox = BoundingBox3f3f(jth_vertex, 0);
        }
        else {
           originalMeshBoundingBox.mergeIn(jth_vertex );
        }
      }
    }
    BoundingSphere3f originalMeshBounds = originalMeshBoundingBox.toBoundingSphere();
    BoundingSphere3f scaledMeshBounds = mLoc->bounds(child_uuid);
    double scalingfactor = scaledMeshBounds.radius()/(4.0*originalMeshBounds.radius());

    //std::cout << mLoc->mesh(child_uuid) << " :mLoc->mesh(child_uuid)\n";
    //std::cout << originalMeshBounds << " , scaled = " << scaledMeshBounds << "\n";
    //std::cout << scalingfactor  << " : scalingfactor\n";
    /** End: find scaling factor **/

    uint geometrySize = agg_mesh->geometry.size(); 
    
    std::vector<GeometryInstance> instances;
    for (uint i = 0; i < m->instances.size(); i++) {
      GeometryInstance geomInstance = m->instances[i];      
      
      assert (geomInstance.geometryIndex < m->geometry.size());
      SubMeshGeometry smg = m->geometry[geomInstance.geometryIndex];

      geomInstance.geometryIndex =  numAddedSubMeshGeometries;
      
      smg.aabb = BoundingBox3f3f::null();

      for (uint j = 0; j < smg.positions.size(); j++) {
        Vector4f jth_vertex_4f =  geomInstance.transform*Vector4f(smg.positions[j].x,
                                                                  smg.positions[j].y,
                                                                  smg.positions[j].z,
                                                                  1.0f);
        smg.positions[j] = Vector3f( jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z );
        smg.positions[j] = fixUp(m->up_axis, smg.positions[j]);   

        smg.positions[j] = smg.positions[j] * scalingfactor;

        smg.positions[j] = orientation * smg.positions[j] ;

        smg.positions[j].x += (location.x - bnds.center().x);
        smg.positions[j].y += (location.y - bnds.center().y);
        smg.positions[j].z += (location.z - bnds.center().z);
       

        if (smg.aabb == BoundingBox3f3f::null()) {
          smg.aabb = BoundingBox3f3f(smg.positions[j], 0);
        }
        else {
          smg.aabb.mergeIn(smg.positions[j]);
        }
      }

      instances.push_back(geomInstance);
      agg_mesh->geometry.push_back(smg);

      numAddedSubMeshGeometries++;

      totalVertices = totalVertices + smg.positions.size();
    }

    for (uint i = 0; i < instances.size(); i++) {
      GeometryInstance geomInstance = instances[i];      

      for (GeometryInstance::MaterialBindingMap::iterator mat_it = geomInstance.materialBindingMap.begin();
           mat_it != geomInstance.materialBindingMap.end(); mat_it++)
        {
          (mat_it->second) += agg_mesh->materials.size();
        }

      agg_mesh->instances.push_back(geomInstance);
    }

    agg_mesh->materials.insert(agg_mesh->materials.end(),
                               m->materials.begin(),
                               m->materials.end());

    uint lightsSize = agg_mesh->lights.size();
    agg_mesh->lights.insert(agg_mesh->lights.end(),
                            m->lights.begin(),
                            m->lights.end());
    for (uint j = 0; j < m->lightInstances.size(); j++) {
      LightInstance& lightInstance = m->lightInstances[j];
      lightInstance.lightIndex += lightsSize;
      agg_mesh->lightInstances.push_back(lightInstance);
    }

    for (Meshdata::URIMap::const_iterator it = m->textureMap.begin();
         it != m->textureMap.end(); it++)
      {
        agg_mesh->textureMap[it->first] = it->second;
      }
  }

  for (uint i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      assert(false);
    }
    mAggregateObjects[child_uuid]->mMeshdata = std::tr1::shared_ptr<Meshdata>();
  }
  
  simplify(agg_mesh, 100000 );

  /* Set the mesh for the aggregated object and if it has a parent, schedule
   a task to update the parent's mesh */
  aggObject->mMeshdata = agg_mesh; 
  if (aggObject->mParentUUID != UUID::null()) {
    
    generateAggregateMesh(aggObject->mParentUUID, Duration::seconds(10.0));
  }

  uploadMesh(uuid, agg_mesh);
}

void AggregateManager::metadataFinished(const UUID uuid, const UUID child_uuid, std::string meshName,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (response != NULL) {
    //    std::cout << "metadataFinished: SUCCESS : " << response->getURI().toString()  <<  "\n";

    const Transfer::RemoteFileMetadata metadata = *response;

    Transfer::TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata,
                                               response->getChunkList().front(), 1.0,
                                               std::tr1::bind(&AggregateManager::chunkFinished, this, uuid, child_uuid,
                                                              std::tr1::placeholders::_1,
                                                              std::tr1::placeholders::_2) ) );

    mTransferPool->addRequest(req);
  }
  else {
    std::cout<<"Failed metadata download"   <<std::endl;
  }
}

void AggregateManager::chunkFinished(const UUID uuid, const UUID child_uuid,
                                     std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                     std::tr1::shared_ptr<const Transfer::DenseData> response)
{
  if (response != NULL) {
    if (mAggregateObjects[child_uuid]->mMeshdata == std::tr1::shared_ptr<Meshdata>() ) {
      
      //MeshdataPtr m = mModelsSystem->load( request->getURI(), request->getMetadata().getFingerprint() , (const char*)response->data(), response->length() );
      MeshdataPtr m = mModelsSystem->load(request->getURI(), request, response);
    
      mAggregateObjects[child_uuid]->mMeshdata = m;

      {
        std::cout << "STORING MESH: " << request->getURI().toString() << "\n";

        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        mMeshStore[request->getURI().toString()] = m;
      }
 
      //      std::cout << mAggregateObjects[child_uuid]->mMeshdata->materials.size() << " : mAggregateObjects[child_uuid]->mMeshdata->materials.size()\n";
      //std::cout << mAggregateObjects[child_uuid]->mMeshdata->geometry.size() << " : mAggregateObjects[child_uuid]->mMeshdata->geometry.size()\n";
      //std::cout << mAggregateObjects[child_uuid]->mMeshdata->instances.size() << " : mAggregateObjects[child_uuid]->mMeshdata->instances.size()\n";

      //      std::cout << "ChunkFinished: generateAggregateMesh called\n";

      generateAggregateMesh(uuid);
    }
  }
  else {
    std::cout << "ChunkFinished fail!\n";
  }
}

void AggregateManager::uploadMesh(const UUID& uuid, std::tr1::shared_ptr<Meshdata> meshptr) {
  boost::mutex::scoped_lock lock(mUploadQueueMutex);

  mUploadQueue[uuid] = meshptr;

  mCondVar.notify_one();
}

void AggregateManager::uploadQueueServiceThread() {

  while (mThreadRunning) {
    boost::mutex::scoped_lock lock(mUploadQueueMutex);

    while (mUploadQueue.empty()) {
      mCondVar.wait(lock);

      if (!mThreadRunning) {
        return;
      }
    }


      std::map<UUID, std::tr1::shared_ptr<Meshdata>  >::iterator it = mUploadQueue.begin();
      UUID uuid = it->first;
      std::tr1::shared_ptr<Meshdata> meshptr = it->second;

      mUploadQueue.erase(it);


      lock.unlock();

      const int MESHNAME_LEN = 1024;
      char localMeshName[MESHNAME_LEN];
      snprintf(localMeshName, MESHNAME_LEN, "aggregate_mesh_%s.dae", uuid.toString().c_str());
      //      std::cout << localMeshName << " = localMeshName\n";
      mModelsSystem->convertMeshdata(*meshptr, localMeshName);


      //      std::cout << "Uploading mesh for: " << uuid.toString() << "\n";
      //Upload to CDN
      std::string cmdline = std::string("./upload_to_cdn.sh ") + localMeshName;
      system( cmdline.c_str()  );

      //Update loc
      std::string cdnMeshName = "meerkat:///tahir/" + std::string(localMeshName);
      mLoc->updateLocalAggregateMesh(uuid, cdnMeshName);
  }
}

}
