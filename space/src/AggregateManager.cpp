/*  Sirikata
 *  AggregateManager.cpp
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

#include "AggregateManager.hpp"

#include <sirikata/mesh/ModelsSystemFactory.hpp>

#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#define snprintf _snprintf
#endif

namespace Sirikata {

AggregateManager::AggregateManager(SpaceContext* ctx, LocationService* loc) :
  mContext(ctx), mLoc(loc)
{
    mModelsSystem = NULL;
    if (ModelsSystemFactory::getSingleton().hasConstructor("any"))
        mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());

    static char x = '1';
    mTransferPool = mTransferMediator->registerClient("SpaceAggregator_"+x);
    x++;    
}

AggregateManager::~AggregateManager() {
    delete mModelsSystem;
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
  uint32 numChildrenBefore = children.size();

  if ( std::find(children.begin(), children.end(), child_uuid) == children.end() ) {
    children.push_back(child_uuid);

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    if (mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      mAggregateObjects[child_uuid] = std::tr1::shared_ptr<AggregateObject> (new AggregateObject(child_uuid, uuid));
    }
    else {
      mAggregateObjects[child_uuid]->mParentUUID = uuid;
    }

    mAggregateGenerationStartTime = Timer::now();

    updateChildrenTreeLevel(uuid, mAggregateObjects[uuid]->mTreeLevel);

    lock.unlock();

    std::cout << "addChild:  "  << uuid.toString()
              << " CHILD " << child_uuid.toString() << " "
              << "\n";
    fflush(stdout);   

    mContext->mainStrand->post(Duration::seconds(60), std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, mAggregateGenerationStartTime));    
  }
}

void AggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
  std::vector<UUID>& children = getChildren(uuid);

  std::vector<UUID>::iterator it = std::find(children.begin(), children.end(), child_uuid);

  if (it != children.end()) {
    children.erase( it );

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    
    mDirtyAggregateObjects[uuid] = mAggregateObjects[uuid];
    mAggregateObjects[uuid]->generatedLastRound = false;
    mAggregateGenerationStartTime =  Timer::now();

    mContext->mainStrand->post(Duration::seconds(60), std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, mAggregateGenerationStartTime));
  }
}

void AggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
  if (mModelsSystem == NULL) return;

  if (mDirtyAggregateObjects.find(uuid) != mDirtyAggregateObjects.end()) return;

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) return;
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();
  aggObject->mLastGenerateTime = Timer::now();

  mContext->mainStrand->post( delayFor, std::tr1::bind(&AggregateManager::generateAggregateMeshAsync, this, uuid, aggObject->mLastGenerateTime, true)  );
}


bool AggregateManager::generateAggregateMeshAsync(const UUID uuid, Time postTime, bool generateSiblings) {
  Time curTime = Timer::now();

  /* Get the aggregate object corresponding to UUID 'uuid'.  */
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
    std::cout << uuid.toString() <<" : not found in aggregate objects map\n";
    return false;
  }
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();
  /****/

  if (postTime < aggObject->mLastGenerateTime) {
    return false;
  }

  if (postTime < mAggregateGenerationStartTime) {
    return false;
  }

  std::vector<UUID>& children = aggObject->mChildren;

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];

    if (!mLoc->contains(child_uuid)) {
      generateAggregateMesh(uuid, Duration::milliseconds(10.0f));
      
      return false;
    }
  }

  if (!mLoc->contains(uuid)) {
    generateAggregateMesh(uuid, Duration::milliseconds(10.0f));

    return false;
  }

  bool allMeshesAvailable = true;
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];

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
        if (mMeshStore.find(meshName) == mMeshStore.end()) {
          std::cout << meshName << " = meshName requesting download\n";
          Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, uuid, child_uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

          mTransferPool->addRequest(req);

          std::cout << uuid.toString() <<" : child mesh: "<< meshName  <<" not found locally\n";
          
          allMeshesAvailable = false;

          mMeshStore[meshName] = std::tr1::shared_ptr<Meshdata>();
        }
        else if (!mMeshStore[meshName]) {
          allMeshesAvailable = false;
        }
      }
    }
  }

  if (!allMeshesAvailable) return false;


  std::tr1::shared_ptr<Meshdata> agg_mesh =  std::tr1::shared_ptr<Meshdata>( new Meshdata() );
  BoundingSphere3f bnds = mLoc->bounds(uuid);
  float32 bndsX = bnds.center().x;
  float32 bndsY = bnds.center().y;
  float32 bndsZ = bnds.center().z;

  uint32   numAddedSubMeshGeometries = 0;
  double totalVertices = 0;
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];
    Time insideLoopCurTime = Timer::now();

    Vector3f location = mLoc->currentPosition(child_uuid);
    float32 locationX = location.x;
    float32 locationY = location.y;
    float32 locationZ = location.z;

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
          m = mMeshStore[meshName];
        }
      }
      else {
        generateAggregateMesh(uuid, Duration::milliseconds(100.0f));
        return false;
      }
    }

    lock.unlock();

    std::cout << aggObject->mTreeLevel << " aggObject->mTreeLevel\n";
    

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
    bool firstUpdate = true;
    for (uint32 i = 0; i < m->instances.size(); i++) {
      const GeometryInstance& geomInstance = m->instances[i];
      const SubMeshGeometry& smg = m->geometry[geomInstance.geometryIndex];      

      uint32 smgPositionsSize = smg.positions.size();
      for (uint32 j = 0; j < smgPositionsSize; j++) {
        const Vector3f& v = smg.positions[j];

        Vector4f jth_vertex_4f = geomInstance.transform*Vector4f(v.x,
                                                                  v.y,
                                                                  v.z,
                                                                  1.0f);
        Vector3f jth_vertex(jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z);        

        if (firstUpdate) {
          originalMeshBoundingBox = BoundingBox3f3f(jth_vertex, 0);
          firstUpdate = false;
        }
        else {
           originalMeshBoundingBox.mergeIn(jth_vertex );
        }
      }
    }
    
    BoundingSphere3f originalMeshBounds = originalMeshBoundingBox.toBoundingSphere();
    BoundingSphere3f scaledMeshBounds = mLoc->bounds(child_uuid);
    double scalingfactor = scaledMeshBounds.radius()/(originalMeshBounds.radius());

    /** End: find scaling factor **/

    uint32 geometrySize = agg_mesh->geometry.size();
 
    
    std::vector<GeometryInstance> instances;
    for (uint32 i = 0; i < m->instances.size(); i++) {
      GeometryInstance geomInstance = m->instances[i];

      assert (geomInstance.geometryIndex < m->geometry.size());
      SubMeshGeometry smg = m->geometry[geomInstance.geometryIndex];

      geomInstance.geometryIndex =  numAddedSubMeshGeometries;

      smg.aabb = BoundingBox3f3f::null();

      uint32 smgPositionsSize = smg.positions.size();

      firstUpdate = true;

      for (uint32 j = 0; j < smgPositionsSize; j++) {

        Vector3f& jth_vertex = smg.positions[j];

        Vector4f jth_vertex_4f = geomInstance.transform*Vector4f(jth_vertex.x,
                                                                  jth_vertex.y,
                                                                  jth_vertex.z,
                                                                  1.0f);

        jth_vertex = Vector3f(jth_vertex_4f.x, jth_vertex_4f.y, jth_vertex_4f.z);              

        jth_vertex *= scalingfactor;

        jth_vertex = orientation * jth_vertex ;

        jth_vertex.x += (locationX - bndsX);
        jth_vertex.y += (locationY - bndsY);
        jth_vertex.z += (locationZ - bndsZ);

        if (firstUpdate) {
          smg.aabb = BoundingBox3f3f(jth_vertex, 0);
          firstUpdate = false;
        }
        else {
          smg.aabb.mergeIn(jth_vertex);
        }
      }

      if (smgPositionsSize > 0) {
        instances.push_back(geomInstance);
        agg_mesh->geometry.push_back(smg);
      
        numAddedSubMeshGeometries++;
      }

      totalVertices = totalVertices + smgPositionsSize;
    }

    for (uint32 i = 0; i < instances.size(); i++) {
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

    uint32 lightsSize = agg_mesh->lights.size();
    agg_mesh->lights.insert(agg_mesh->lights.end(),
                            m->lights.begin(),
                            m->lights.end());
    for (uint32 j = 0; j < m->lightInstances.size(); j++) {
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

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    assert( mAggregateObjects.find(child_uuid) != mAggregateObjects.end()) ;
    
    mAggregateObjects[child_uuid]->mMeshdata = std::tr1::shared_ptr<Meshdata>();
  }

  //Simplify the mesh...

  mMeshSimplifier.simplify(agg_mesh, 40000);
  
  //... and now upload the mesh to the CDN
  const int MESHNAME_LEN = 1024;
  char localMeshName[MESHNAME_LEN];
  snprintf(localMeshName, MESHNAME_LEN, "aggregate_mesh_%s.dae", uuid.toString().c_str());

  mModelsSystem->convertMeshdata(*agg_mesh, "colladamodels", std::string("/home/tahir/merucdn/meru/dump/") + localMeshName);
        
  //Upload to CDN
  std::string cmdline = std::string("./upload_to_cdn.sh ") +  localMeshName;
  system( cmdline.c_str()  );      

  //Update loc
  std::string cdnMeshName = "meerkat:///tahir/" + std::string(localMeshName);
  mLoc->updateLocalAggregateMesh(uuid, cdnMeshName);  
  

  //Keep the meshstore's memory usage under control.
  boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
  if (mMeshStore.size() > 100) {
    mMeshStore.erase(mMeshStore.begin());
  }

  return true;
}

void AggregateManager::metadataFinished(const UUID uuid, const UUID child_uuid, std::string meshName,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (response != NULL) {
    const Transfer::RemoteFileMetadata metadata = *response;

    Transfer::TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata,
                                               response->getChunkList().front(), 1.0,
                                               std::tr1::bind(&AggregateManager::chunkFinished, this, uuid, child_uuid, meshName,
                                                              std::tr1::placeholders::_1,
                                                              std::tr1::placeholders::_2) ) );

    mTransferPool->addRequest(req);
  }
  else {
    std::cout<<"Failed metadata download: Retrying..."   <<std::endl;
    Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, uuid, child_uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);

  }
}

void AggregateManager::chunkFinished(const UUID uuid, const UUID child_uuid, std::string meshName,
                                       std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                       std::tr1::shared_ptr<const Transfer::DenseData> response)
{
    if (response != NULL) {
      boost::mutex::scoped_lock aggregateObjectsLock(mAggregateObjectsMutex);
      if (mAggregateObjects[child_uuid]->mMeshdata == std::tr1::shared_ptr<Meshdata>() ) {

        MeshdataPtr m = mModelsSystem->load(request->getURI(), request->getMetadata().getFingerprint(), response);

        mAggregateObjects[child_uuid]->mMeshdata = m;

        aggregateObjectsLock.unlock();

        {
          boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);

          mMeshStore[request->getURI().toString()] = m;
          
          std::cout << "Stored mesh in mesh store for: " <<  request->getURI().toString()  << "\n";
        }        
      }
    }
    else {
      std::cout << "ChunkFinished fail... retrying\n";
      Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, uuid, child_uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

      mTransferPool->addRequest(req);
    }
}

std::vector<UUID>& AggregateManager::getChildren(const UUID& uuid) {
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    static std::vector<UUID> emptyVector;

    if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
      return emptyVector;
    }

    std::vector<UUID>& children = mAggregateObjects[uuid]->mChildren;

    return children;
}


void AggregateManager::generateMeshesFromQueue(Time postTime) {
    if (postTime < mAggregateGenerationStartTime) {
      return;
    }

    std::cout << "generateMeshesFromQueue called\n";

    for (std::tr1::unordered_map<UUID, std::tr1::shared_ptr<AggregateObject>, UUID::Hasher>::iterator it = mDirtyAggregateObjects.begin();
         it != mDirtyAggregateObjects.end(); it++)
    {
      mObjectsByPriority[it->second->mTreeLevel].push_back(it->second);
    }

    bool returner = false;
    for (std::map<uint16, std::deque<std::tr1::shared_ptr<AggregateObject> > >::reverse_iterator it =  mObjectsByPriority.rbegin();
         it != mObjectsByPriority.rend(); it++)
    {
      if (it->second.size() > 0) {
        std::tr1::shared_ptr<AggregateObject> aggObject = it->second.front();       

        std::cout << "generateMeshesFromQueue loop called\n";
        if (aggObject->generatedLastRound) continue;        

        returner=generateAggregateMeshAsync(aggObject->mUUID, Timer::now(), false);

        if (returner) it->second.pop_front();
        
        break;
      }
    }

    mDirtyAggregateObjects.clear();

    if (mObjectsByPriority.size() > 0) {
      Duration dur = (returner) ? Duration::milliseconds(1.0) : Duration::milliseconds(200.0);
      mContext->mainStrand->post(dur, std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, Timer::now()));
    }
}

void AggregateManager::updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel) {
    //mAggregateObjectsMutex MUST be locked BEFORE calling this function.

    mAggregateObjects[uuid]->mTreeLevel = treeLevel;

    if ( mAggregateObjects[uuid]->mChildren.size() > 0 ) {
      mDirtyAggregateObjects[uuid] = mAggregateObjects[uuid];
      mAggregateObjects[uuid]->generatedLastRound = false;
    }

    for (uint32 i = 0; i < mAggregateObjects[uuid]->mChildren.size(); i++) {
      updateChildrenTreeLevel(mAggregateObjects[uuid]->mChildren[i], treeLevel+1);
    }
}

}
