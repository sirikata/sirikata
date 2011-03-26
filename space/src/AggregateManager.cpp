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

#include <sirikata/core/network/IOServiceFactory.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>


#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
#define snprintf _snprintf
#endif

//This is an estimate of the solid angle for one pixel on a 2560*1600 screen resolution;
//Human Eye FOV = 4.17 sr, dividing that by (2560*1600).
#define HUMAN_FOV  4.17
#define ONE_PIXEL_SOLID_ANGLE (HUMAN_FOV/(2560.0*1600.0))
#define TWO_PI (2.0*3.14159)

namespace Sirikata {

using namespace Mesh;

AggregateManager::AggregateManager( LocationService* loc) :
  mAggregationThread(NULL), mLoc(loc)
{
    mModelsSystem = NULL;
    if (ModelsSystemFactory::getSingleton().hasConstructor("any"))
        mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());

    mAggregationService = Network::IOServiceFactory::makeIOService();
    mAggregationStrand = mAggregationService->createStrand();
    mIOWork = new Network::IOWork(mAggregationService, "Aggregation Work");    

    static char x = '1';
    mTransferPool = mTransferMediator->registerClient("SpaceAggregator_"+x);
    x++;

    // Start the processing thread
    mAggregationThread = new Thread( std::tr1::bind(&AggregateManager::aggregationThreadMain, this) );
}

AggregateManager::~AggregateManager() {
    // Shut down the main processing thread
    if (mAggregationThread != NULL) {
        if (mAggregationService != NULL)
            mAggregationService->stop();
        mAggregationThread->join();
    }

    delete mAggregationStrand;
    Network::IOServiceFactory::destroyIOService(mAggregationService);
    mAggregationService = NULL;
    
    delete mAggregationThread;

    delete mIOWork;
    mIOWork = NULL;

    delete mModelsSystem;
}

// The main loop for the prox processing thread
void AggregateManager::aggregationThreadMain() {
  mAggregationService->run();
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

    updateChildrenTreeLevel(uuid, mAggregateObjects[uuid]->mTreeLevel);

    addDirtyAggregates(child_uuid);

    mAggregateGenerationStartTime = Timer::now();

    lock.unlock();

    std::cout << "addChild:  "  << uuid.toString()
              << " CHILD " << child_uuid.toString() << " "
              << "\n";   

    mAggregationStrand->post(Duration::seconds(20), std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, mAggregateGenerationStartTime));
  }
}

void AggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
  std::vector<UUID>& children = getChildren(uuid);

  std::vector<UUID>::iterator it = std::find(children.begin(), children.end(), child_uuid);

  if (it != children.end()) {
    children.erase( it );

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    addDirtyAggregates(child_uuid);
    
    mAggregateGenerationStartTime =  Timer::now();

    mAggregationStrand->post(Duration::seconds(20), std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, mAggregateGenerationStartTime));
  }
}

void AggregateManager::aggregateObserved(const UUID& objid, uint32 nobservers) {  
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  
  if (mAggregateObjects.find(objid) != mAggregateObjects.end())
    mAggregateObjects[objid]->mNumObservers = nobservers;
}


void AggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
  if (mModelsSystem == NULL) return;

  if (mDirtyAggregateObjects.find(uuid) != mDirtyAggregateObjects.end()) return;

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) return;
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();
  aggObject->mLastGenerateTime = Timer::now();

  mAggregationStrand->post( delayFor, std::tr1::bind(&AggregateManager::generateAggregateMeshAsyncIgnoreErrors, this, uuid, aggObject->mLastGenerateTime, true)  );
}
void AggregateManager::generateAggregateMeshAsyncIgnoreErrors(const UUID uuid, Time postTime, bool generateSiblings) {
	bool retval=generateAggregateMeshAsync(uuid, postTime, generateSiblings);
	if (!retval) {
		SILOG(aggregate,error,"generateAggregateMeshAsync returned false, but no error handling happening");
	}
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

  std::vector<UUID>& children = aggObject->mChildren; //mLeaves

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
    Mesh::MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;

    if (!m) {
      //request a download or generation of the mesh
      std::string meshName = mLoc->mesh(child_uuid);

      if (meshName != "") {
        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        if (mMeshStore.find(meshName) == mMeshStore.end()) {

          Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, curTime, uuid, child_uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

          mTransferPool->addRequest(req);

          allMeshesAvailable = false;

          mMeshStore[meshName] = MeshdataPtr();

          break;
        }
        else if (!mMeshStore[meshName]) {
          allMeshesAvailable = false;
          break;
        }
      }
    }
  }

  if (!allMeshesAvailable) return false;  

  MeshdataPtr agg_mesh =  MeshdataPtr( new Meshdata() );
  agg_mesh->globalTransform = Matrix4x4f::identity();
  BoundingSphere3f bnds = mLoc->bounds(uuid);
  float32 bndsX = bnds.center().x;
  float32 bndsY = bnds.center().y;
  float32 bndsZ = bnds.center().z;

  std::tr1::unordered_map<std::string, uint32> meshToStartIdxMapping;
  std::tr1::unordered_map<std::string, uint32> meshToStartMaterialsIdxMapping;
  std::tr1::unordered_map<std::string, uint32> meshToStartLightIdxMapping;
  std::tr1::unordered_map<std::string, uint32> meshToStartNodeIdxMapping;

  uint32   numAddedSubMeshGeometries = 0;
  // Make sure we've got all the Meshdatas
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;

    std::string meshName = mLoc->mesh(child_uuid);
    if (!m) {
      //request a download or generation of the mesh
      if (meshName != "") {
        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        if (mMeshStore.find(meshName) != mMeshStore.end()) {
          mAggregateObjects[child_uuid]->mMeshdata = mMeshStore[meshName];
        }
      }
      else {
        generateAggregateMesh(uuid, Duration::milliseconds(100.0f));
        return false;
      }
    }
  }

  // And finally, when we do, perform the merge
  std::tr1::unordered_set<String> textureSet; // Tracks textures so we can fill in
                                         // agg_mesh->textures when we're done
                                         // copying data in.

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;
    std::string meshName = mLoc->mesh(child_uuid);
    lock.unlock();

    assert(m);

    /** Find scaling factor **/
    BoundingBox3f3f originalMeshBoundingBox = BoundingBox3f3f::null();
    bool firstUpdate = true;

    Meshdata::GeometryInstanceIterator geoinst_it = m->getGeometryInstanceIterator();
    uint32 geoinst_idx;
    Matrix4x4f geoinst_pos_xform;

     while( geoinst_it.next(&geoinst_idx, &geoinst_pos_xform) ) {
      const GeometryInstance& geomInstance = m->instances[geoinst_idx];
      const SubMeshGeometry& smg = m->geometry[geomInstance.geometryIndex];

      uint32 smgPositionsSize = smg.positions.size();
      for (uint32 j = 0; j < smgPositionsSize; j++) {
        const Vector3f& v = smg.positions[j];

        Vector4f jth_vertex_4f = geoinst_pos_xform*Vector4f(v.x, v.y, v.z, 1.0f);
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

    // We me reuse more than one of the same mesh, e.g. the aggregate may have
    // two identical trees that are at different locations. In that case,
    // SubMeshGeometry, Textures, LightInfos, MaterialEffectInfos, and Nodes can
    // all be shared, and can therefore reuse offsets.
    // If necessary, add the offset in.
    if ( meshToStartIdxMapping.find(meshName) == meshToStartIdxMapping.end()) {
      meshToStartIdxMapping[ meshName ] = agg_mesh->geometry.size();
      meshToStartMaterialsIdxMapping[ meshName ] = agg_mesh->materials.size();
      meshToStartNodeIdxMapping[ meshName ] = agg_mesh->nodes.size();
      meshToStartLightIdxMapping[ meshName ] = agg_mesh->lights.size();

      // Copy SubMeshGeometries. We loop through so we can reset the numInstances
      for(uint32 smgi = 0; smgi < m->geometry.size(); smgi++) {
          SubMeshGeometry smg = m->geometry[smgi];

          agg_mesh->geometry.push_back(smg);
      }
      // Copy Materials
      agg_mesh->materials.insert(agg_mesh->materials.end(),
          m->materials.begin(),
          m->materials.end());
      // Copy names of textures from the materials into a set so we can fill in
      // the texture list when we finish adding all subobjects
      for(MaterialEffectInfoList::const_iterator mat_it = m->materials.begin(); mat_it != m->materials.end(); mat_it++) {
          for(MaterialEffectInfo::TextureList::const_iterator tex_it = mat_it->textures.begin(); tex_it != mat_it->textures.end(); tex_it++)
              if (!tex_it->uri.empty()) textureSet.insert(tex_it->uri);
      }
      // Copy Lights
      agg_mesh->lights.insert(agg_mesh->lights.end(),
          m->lights.begin(),
          m->lights.end());
      // Copy Nodes. Loop through to adjust node indices.
      /*uint32 noff = agg_mesh->nodes.size();
      for(uint32 ni = 0; ni < m->nodes.size(); ni++) {
          Node n = m->nodes[ni];
          n.parent += noff;
          for(uint32 ci = 0; ci < n.children.size(); ci++)
              n.children[ci] += noff;
          for(uint32 ci = 0; ci < n.instanceChildren.size(); ci++)
              n.instanceChildren[ci] += noff;
          agg_mesh->nodes.push_back(n);
        }*/
    }

    // And alwasy extract into convenience variables
    uint32 submeshGeomOffset = meshToStartIdxMapping[meshName];
    uint32 submeshMaterialsOffset = meshToStartMaterialsIdxMapping[meshName];
    uint32 submeshLightOffset = meshToStartLightIdxMapping[meshName];
    uint32 submeshNodeOffset = meshToStartNodeIdxMapping[meshName];

    // Extract the loc information we need for this object.
    Vector3f location = mLoc->currentPosition(child_uuid);
    float32 locationX = location.x;
    float32 locationY = location.y;
    float32 locationZ = location.z;
    Quaternion orientation = mLoc->currentOrientation(child_uuid);

    // Reuse geoinst_it and geoinst_idx from earlier, but with a new iterator.
    geoinst_it = m->getGeometryInstanceIterator();
    Matrix4x4f orig_geo_inst_xform;

    while( geoinst_it.next(&geoinst_idx, &orig_geo_inst_xform) ) {
      // Copy the instance data.
      GeometryInstance geomInstance = m->instances[geoinst_idx];

      // Sanity check
      assert (geomInstance.geometryIndex < m->geometry.size());

      // Shift indices for
      //  Materials
      for(GeometryInstance::MaterialBindingMap::iterator mbit = geomInstance.materialBindingMap.begin(); mbit != geomInstance.materialBindingMap.end(); mbit++)
          mbit->second += submeshMaterialsOffset;
      //  Geometry
      geomInstance.geometryIndex += submeshGeomOffset;
      //  Parent node
      //  FIXME see note below to understand why this ultimately has no
      //  effect. parentNode ends up getting overwritten with a new parent nodes
      //  index that flattens the node hierarchy.
      geomInstance.parentNode += submeshNodeOffset;

      //translation
      Matrix4x4f trs = Matrix4x4f( Vector4f(1,0,0,locationX - bndsX),
                                   Vector4f(0,1,0,locationY - bndsY),
                                   Vector4f(0,0,1,locationZ - bndsZ),
                                   Vector4f(0,0,0,1),                 Matrix4x4f::ROWS());

      //rotate
      float ox = orientation.normal().x;
      float oy = orientation.normal().y;
      float oz = orientation.normal().z;
      float ow = orientation.normal().w;

      trs *= Matrix4x4f( Vector4f(1-2*oy*oy - 2*oz*oz , 2*ox*oy - 2*ow*oz, 2*ox*oz + 2*ow*oy, 0),
                         Vector4f(2*ox*oy + 2*ow*oz, 1-2*ox*ox-2*oz*oz, 2*oy*oz-2*ow*ox, 0),
                         Vector4f(2*ox*oz-2*ow*oy, 2*oy*oz + 2*ow*ox, 1-2*ox*ox - 2*oy*oy,0),
                         Vector4f(0,0,0,1),                 Matrix4x4f::ROWS());

      //scaling
      trs *= Matrix4x4f::scale(scalingfactor);

      // Generate a node for this instance
      NodeIndex geom_node_idx = agg_mesh->nodes.size();
      // FIXME because we need to have trs * original_transform (i.e., left
      // instead of right multiply), the trs transform has to be at the
      // root. This means we can't trivially handle this with additional nodes
      // since the new node would have to be inserted at the root (and therefore
      // some may end up conflicting). For now, we just flatten these by
      // creating a new root node.

      agg_mesh->nodes.push_back( Node(trs * orig_geo_inst_xform) );

      agg_mesh->rootNodes.push_back(geom_node_idx);
      // Overwrite the parent node to make this new one with the correct
      // transform the one we use.
      geomInstance.parentNode = geom_node_idx;


      // Increase ref count on instanced geometry
      SubMeshGeometry& smgRef = agg_mesh->geometry[geomInstance.geometryIndex];

      //Push the instance into the Meshdata data structure
      agg_mesh->instances.push_back(geomInstance);
    }

    for (uint32 j = 0; j < m->lightInstances.size(); j++) {
      LightInstance& lightInstance = m->lightInstances[j];
      lightInstance.lightIndex += submeshLightOffset;
      agg_mesh->lightInstances.push_back(lightInstance);
    }
  }

  // We should have all the textures in our textureSet since we looped through
  // all the materials, just fill in the list now.
  for (std::tr1::unordered_set<String>::iterator it = textureSet.begin(); it != textureSet.end(); it++)
      agg_mesh->textures.push_back( *it );


  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i];
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    assert( mAggregateObjects.find(child_uuid) != mAggregateObjects.end()) ;

    mAggregateObjects[child_uuid]->mMeshdata = std::tr1::shared_ptr<Meshdata>();
  }

  const int MESHNAME_LEN = 1024;
  char localMeshName[MESHNAME_LEN];
  snprintf(localMeshName, MESHNAME_LEN, "%d_aggregate_mesh_%s.dae", aggObject->mTreeLevel, uuid.toString().c_str());
  std::string cdnMeshName = "meerkat:///tahir/" + std::string(localMeshName);
  agg_mesh->uri = cdnMeshName;
  
  //Simplify the mesh...
  mMeshSimplifier.simplify(agg_mesh, 600);

  //... and now create the collada file, upload to the CDN and update LOC. 
  mModelsSystem->convertMeshdata(*agg_mesh, "colladamodels", std::string("/home/tahir/merucdn/meru/dump/") + localMeshName);

  //Upload to CDN
  std::string cmdline = std::string("./upload_to_cdn.sh ") +  localMeshName;
  system( cmdline.c_str()  );    

  //Update loc
  mLoc->updateLocalAggregateMesh(uuid, cdnMeshName);

  // Code to generate scene files for each level of the tree.
  /*char scenefilename[MESHNAME_LEN];
  snprintf(scenefilename, MESHNAME_LEN, "%d_scene.db", aggObject->mTreeLevel);
  std::fstream scenefile(scenefilename, std::fstream::out | std::fstream::app);
  char sceneline[MESHNAME_LEN];
  snprintf(sceneline, MESHNAME_LEN,
           "\"mesh\",\"graphiconly\",\"tetrahedron\",,,,%f,%f,%f,%f,%f,%f,%f,0,0,0,0,1,0,0,1,1,1,1,1,1,1,0.3,0.1,0,0,1,\"%s\",,,,,,,,,,,,,,,,,,%f,,,,\n",
           bndsX, bndsY, bndsZ, 0.0, 0.0, 0.0, 1.0, cdnMeshName.c_str(), bnds.radius());
  scenefile.write(sceneline,strlen(sceneline));
  scenefile.close();*/
  
  //Keep the meshstore's memory usage under control.
  boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
  if (mMeshStore.size() > 20)
    mMeshStore.clear();

  aggObject->mLeaves.clear();

  return true;
}

void AggregateManager::metadataFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (response != NULL) {
    std::cout << ( Timer::now() - t )  << " : metadataFinished SUCCESS\n";

    const Transfer::RemoteFileMetadata metadata = *response;

    Transfer::TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata,
                                               response->getChunkList().front(), 1.0,
                                             std::tr1::bind(&AggregateManager::chunkFinished, this, t,uuid, child_uuid, meshName,
                                                              std::tr1::placeholders::_1,
                                                              std::tr1::placeholders::_2) ) );

    mTransferPool->addRequest(req);
  }
  else {
    std::cout<<"Failed metadata download: Retrying...: Response time: "   << ( Timer::now() - t )   << std::endl;
    Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, t, uuid, child_uuid, meshName,
                                       std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);

  }
}

void AggregateManager::chunkFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName,
                                       std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                       std::tr1::shared_ptr<const Transfer::DenseData> response)
{
    if (response != NULL) {
      std::cout << "Time spent downloading: " << (Timer::now() - t)  << "\n";

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
                                       &AggregateManager::metadataFinished, this, t, uuid, child_uuid, meshName,
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

void AggregateManager::getLeaves(const std::vector<UUID>& individualObjects) {
  for (uint32 i=0; i<individualObjects.size(); i++) {
    const  UUID& indl_uuid = individualObjects[i];
    UUID uuid = indl_uuid;

    std::tr1::shared_ptr<AggregateObject> obj = mAggregateObjects[uuid];
    float radius = mLoc->bounds(uuid).radius();

    while (uuid != UUID::null()) {
      if (mDirtyAggregateObjects.find(uuid) != mDirtyAggregateObjects.end()) {
        
        float solid_angle = TWO_PI * (1-sqrt(1- pow(radius/obj->mDistance,2)));
        
        if (solid_angle > ONE_PIXEL_SOLID_ANGLE) {
          obj->mLeaves.push_back(indl_uuid);
        }
      }
      
      uuid = obj->mParentUUID;
      
      if (mAggregateObjects.find(uuid) != mAggregateObjects.end())
        obj = mAggregateObjects[uuid];
    }
  }
}

void AggregateManager::generateMeshesFromQueue(Time postTime) {
    if (postTime < mAggregateGenerationStartTime) {      
      return;
    }

    //Get the leaves that belong to each node.
    std::vector<UUID> individualObjects;    

    if ( mDirtyAggregateObjects.size() > 0 ) {
      for (std::tr1::unordered_map<UUID, std::tr1::shared_ptr<AggregateObject>, UUID::Hasher >::iterator it = mAggregateObjects.begin();
           it != mAggregateObjects.end() ; it++)
      {
          std::tr1::shared_ptr<AggregateObject> aggObject = it->second;           
      
          if (aggObject->mChildren.size() == 0) {
            individualObjects.push_back(aggObject->mUUID);
          }

          if (mDirtyAggregateObjects.find(it->first) == mDirtyAggregateObjects.end()) {
            continue;
          }      
     
          float radius  = INT_MAX;
          for (uint32 i=0; i < aggObject->mChildren.size(); i++) {
            BoundingSphere3f bnds = mLoc->bounds(aggObject->mChildren[i]);
            if (bnds.radius() < radius) {
              radius = bnds.radius();
            }           
          }

          if (radius == INT_MAX) radius = 0;            

          aggObject->mDistance = 0.01 + radius/sqrt( 1.0 - pow( 1-HUMAN_FOV/TWO_PI, 2) );
      }

      getLeaves(individualObjects);
    }

    //Add objects to generation queue, ordered by priority.
    for (std::tr1::unordered_map<UUID, std::tr1::shared_ptr<AggregateObject>, UUID::Hasher>::iterator it = mDirtyAggregateObjects.begin();
         it != mDirtyAggregateObjects.end(); it++)
    {
      std::tr1::shared_ptr<AggregateObject> aggObject = it->second;
      if (aggObject->mTreeLevel >= 0)
        mObjectsByPriority[ aggObject->mNumObservers + (aggObject->mTreeLevel*0.001) ].push_back(aggObject);
    }

    //Generate the aggregates from the priority queue.
    Time curTime = (mObjectsByPriority.size() > 0) ? Timer::now() : Time::null();
    bool returner = false;
    for (std::map<float, std::deque<std::tr1::shared_ptr<AggregateObject> > >::reverse_iterator it =  mObjectsByPriority.rbegin();
         it != mObjectsByPriority.rend(); it++)
    {
      if (it->second.size() > 0) {
        std::tr1::shared_ptr<AggregateObject> aggObject = it->second.front();

        if (aggObject->generatedLastRound) continue;

        returner=generateAggregateMeshAsync(aggObject->mUUID, curTime, false);

        if (returner) it->second.pop_front();

        break;
      }
    }

    mDirtyAggregateObjects.clear();

    if (mObjectsByPriority.size() > 0) {
      Duration dur = (returner) ? Duration::milliseconds(1.0) : Duration::milliseconds(200.0);
      mAggregationStrand->post(dur, std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, curTime));
    }
}

void AggregateManager::updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel) {
    //mAggregateObjectsMutex MUST be locked BEFORE calling this function.

    mAggregateObjects[uuid]->mTreeLevel = treeLevel;    

    for (uint32 i = 0; i < mAggregateObjects[uuid]->mChildren.size(); i++) {
      updateChildrenTreeLevel(mAggregateObjects[uuid]->mChildren[i], treeLevel+1);
    }
}

//Recursively add uuid and all nodes upto the root to the dirty aggregates map.
void AggregateManager::addDirtyAggregates(UUID uuid) {
  //mAggregateObjectsMutex MUST be locked BEFORE calling this function.

  while (uuid != UUID::null()) {
    std::tr1::shared_ptr<AggregateObject> aggObj = mAggregateObjects[uuid];

    if (aggObj->mChildren.size() > 0) {
      mDirtyAggregateObjects[uuid] = aggObj;
      aggObj->generatedLastRound = false;
    }

    uuid = aggObj->mParentUUID;
  }
}

}
