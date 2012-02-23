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

#include <sirikata/space/AggregateManager.hpp>

#include <sirikata/mesh/ModelsSystemFactory.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/IOWork.hpp>

#include <sirikata/core/transfer/AggregatedTransferPool.hpp>

#include <sirikata/core/transfer/URL.hpp>
#include <json_spirit/json_spirit.h>

#include <sirikata/core/transfer/OAuthHttpManager.hpp>

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#define snprintf _snprintf
#endif

//This is an estimate of the solid angle for one pixel on a 2560*1600 screen resolution;
//Human Eye FOV = 4.17 sr, dividing that by (2560*1600).
#define HUMAN_FOV  4.17
#define ONE_PIXEL_SOLID_ANGLE (HUMAN_FOV/(2560.0*1600.0))
#define TWO_PI (2.0*3.14159)

#define AGG_LOG(lvl, msg) SILOG(aggregate-manager, lvl, msg)

namespace Sirikata {

using namespace Mesh;

AggregateManager::AggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username)
  : mAggregationThread(NULL),
    mAggregationService(new Network::IOService("AggregateManager")),
    mAggregationStrand(mAggregationService->createStrand("AggregateManager")),
    mIOWork(new Network::IOWork(mAggregationService, "Aggregation Work")),
    mLoc(loc),
    mOAuth(oauth),
    mCDNUsername(username),
    mModelTTL(Duration::minutes(10)),
    mCDNKeepAlivePoller(
        new Poller(
            mAggregationStrand,
            std::tr1::bind(&AggregateManager::sendKeepAlives, this),
            "AggregateManager CDN Keep-Alive Poller",
            Duration::seconds(30)
        )
    )
{
    mModelsSystem = NULL;
    if (ModelsSystemFactory::getSingleton().hasConstructor("any"))
        mModelsSystem = ModelsSystemFactory::getSingleton().getConstructor("any")("");

    mTransferMediator = &(Transfer::TransferMediator::getSingleton());

    static char x = '1';
    mTransferPool = mTransferMediator->registerClient<Transfer::AggregatedTransferPool>("SpaceAggregator_"+x);
    x++;

    // Start the processing thread
    mAggregationThread = new Thread( std::tr1::bind(&AggregateManager::aggregationThreadMain, this) );

    removeStaleLeaves();

    mCDNKeepAlivePoller->start();
}

AggregateManager::~AggregateManager() {
    // We need to make sure we clean this up before the IOService and IOStrand
    // it's running on.
    mCDNKeepAlivePoller->stop();
    delete mCDNKeepAlivePoller;

    // Shut down the main processing thread
    delete mIOWork;
    mIOWork = NULL;

    if (mAggregationThread != NULL) {
        if (mAggregationService != NULL)
            mAggregationService->stop();
        mAggregationThread->join();
    }

    delete mAggregationStrand;
    delete mAggregationService;
    mAggregationService = NULL;

    delete mAggregationThread;

    delete mModelsSystem;
}

// The main loop for the prox processing thread
void AggregateManager::aggregationThreadMain() {
  mAggregationService->run();
}

void AggregateManager::addAggregate(const UUID& uuid) {
  AGG_LOG(detailed, "addAggregate called: uuid=" << uuid.toString() << "\n");

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  mAggregateObjects[uuid] = AggregateObjectPtr (new AggregateObject(uuid, UUID::null(), false));
}

bool AggregateManager::cleanUpChild(const UUID& parent_id, const UUID& child_id) {
    // *Must* have already locked mAggregateObjectsMutex

    // This cleans up the tree so we don't have any stale pointers.
    //
    // The approach isn't really ideal, but since we add individual leaf objects
    // as aggregates and don't get feedback on their removal, we have to take
    // this approach.
    //
    // You would think that we could just remove all children since we should
    // get removals bottom up. While we do get removals in that order, it is
    // also possible for a tree to end up collapsing -- if a node ends up with
    // only one child, the parent can be removed, violating the expected
    // condition that the children will be leaves (and therefore have no
    // children of their own). So we detect these two possibilities and either
    // remove the leaf object or cleanup the parent pointer for the aggregate
    // (since we sometimes walk up parent pointers and it will clearly not be
    // valid anymore).

    if (mAggregateObjects.find(child_id) == mAggregateObjects.end()) {
      return true;
    }

    if (!mAggregateObjects[child_id]->leaf) {
        // Aggregate that's getting removed because it only has one child
        // left, just remove the parent pointer
        mAggregateObjects[child_id]->mParentUUID = UUID::null();
        return false;
    }

    return true;
}

void AggregateManager::removeAggregate(const UUID& uuid) {
  AGG_LOG(detailed, "removeAggregate: " << uuid.toString() << "\n");

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  // Cleans up children if necessary, or makes sure they at least don't refer to
  // this object anymore. See cleanUpChild for details.
  AggregateObjectPtr agg = mAggregateObjects[uuid];
  for (std::vector<AggregateObjectPtr>::iterator child_it = agg->mChildren.begin(); child_it != agg->mChildren.end(); child_it++)
    cleanUpChild(uuid, (*child_it)->mUUID);

  mAggregateObjects.erase(uuid);
}

void AggregateManager::addChild(const UUID& uuid, const UUID& child_uuid) {
  std::vector<AggregateObjectPtr>& children = getChildren(uuid);

  if ( ! findChild(children, child_uuid) ) {
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    if (mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
        // TODO(ewencp,tazim) This alone clearly isn't right -- we'll
        // never get a signal that these "aggregates" (which are
        // really just individual objects that don't need aggregation)
        // have been removed, so they'll never be cleaned up... Currently we
        // just remove them when they are left as children of an aggregate (see
        // removeAggregate).
        mAggregateObjects[child_uuid] = std::tr1::shared_ptr<AggregateObject> (new AggregateObject(child_uuid, uuid, true));
    }
    else {
      mAggregateObjects[child_uuid]->mParentUUID = uuid;
    }

    children.push_back(mAggregateObjects[child_uuid]);

    updateChildrenTreeLevel(uuid, mAggregateObjects[uuid]->mTreeLevel);

    addDirtyAggregates(child_uuid);

    mAggregateGenerationStartTime = Timer::now();

    lock.unlock();

    AGG_LOG(detailed, "addChild:  "  << uuid.toString() << " CHILD " << child_uuid.toString() << "\n");

    mAggregationStrand->post(
        Duration::seconds(20),
        std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, mAggregateGenerationStartTime),
        "AggregateManager::generateMeshesFromQueue"
    );
  }
}

void AggregateManager::removeChild(const UUID& uuid, const UUID& child_uuid) {
  AGG_LOG(detailed, "removeChild:  "  << uuid.toString() << " CHILD " << child_uuid.toString() << "\n");

  std::vector<AggregateObjectPtr>& children = getChildren(uuid);



  if ( findChild(children, child_uuid)  ) {
    removeChild( children, child_uuid );

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    // Cleans up the child if necessary, or makes sure it doesn't still refer to
    // this object anymore. See cleanUpChild for details.
    bool child_removed = cleanUpChild(uuid, child_uuid);

    addDirtyAggregates(uuid);

    mAggregateGenerationStartTime =  Timer::now();

    mAggregationStrand->post(
        Duration::seconds(20),
        std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, mAggregateGenerationStartTime),
        "AggregateManager::generateMeshesFromQueue"
    );
  }
}

void AggregateManager::aggregateObserved(const UUID& objid, uint32 nobservers) {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  if (mAggregateObjects.find(objid) != mAggregateObjects.end())
    mAggregateObjects[objid]->mNumObservers = nobservers;
}


void AggregateManager::generateAggregateMesh(const UUID& uuid, const Duration& delayFor) {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) return;
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();
  generateAggregateMesh(uuid, aggObject, delayFor);
}

void AggregateManager::generateAggregateMesh(const UUID& uuid, AggregateObjectPtr aggObject, const Duration& delayFor) {
  if (mModelsSystem == NULL) return;
  if (mDirtyAggregateObjects.find(uuid) != mDirtyAggregateObjects.end()) return;

  AGG_LOG(detailed,"Setting up aggregate " << uuid << " to generate aggregate mesh with " << aggObject->mChildren.size() << " in " << delayFor);
  mAggregationStrand->post(
      delayFor,
      std::tr1::bind(&AggregateManager::generateAggregateMeshAsyncIgnoreErrors, this, uuid, aggObject->mLastGenerateTime, true),
      "AggregateManager::generateAggregateMeshAsyncIgnoreErrors"
  );
}

void AggregateManager::generateAggregateMeshAsyncIgnoreErrors(const UUID uuid, Time postTime, bool generateSiblings) {
	bool retval=generateAggregateMeshAsync(uuid, postTime, generateSiblings);
	if (!retval) {
          SILOG(aggregate,error,"generateAggregateMeshAsync returned false, but no error handling happening" << "\n");
	}
}

bool AggregateManager::generateAggregateMeshAsync(const UUID uuid, Time postTime, bool generateSiblings) {
  Time curTime = Timer::now();

  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
    AGG_LOG(info, uuid.toString() <<" : not found in aggregate objects map" << "\n");

    /*Returning true here because this aggregate is no longer valid, so it should be
      removed from the list of aggregates whose meshes are pending. */
    return true;
  }
  std::tr1::shared_ptr<AggregateObject> aggObject = mAggregateObjects[uuid];
  lock.unlock();

  /*Check if it makes sense to generate the aggregates now. Has the a
    aggregate been updated since the mesh generation command was posted?
    Does LOC contain info about this aggregate and its children?
    Are all the children's meshes available to generate the aggregate? */
  if (postTime < aggObject->mLastGenerateTime) {
    return false;
  }

  if (postTime < mAggregateGenerationStartTime) {
    return false;
  }


  /* Does LOC contain info about this aggregate and its children? */
  if (!mLoc->contains(uuid)) {
    generateAggregateMesh(uuid, aggObject, Duration::milliseconds(10.0f));
    return false;
  }

  std::vector<AggregateObjectPtr>& children = aggObject->mChildren;
                                                   //Set this to mLeaves if you want
                                                   //to generate directly from the leaves of the tree

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;

    if (!mLoc->contains(child_uuid)) {
      generateAggregateMesh(uuid, aggObject, Duration::milliseconds(10.0f));

      return false;
    }
  }

  /*Are the meshes of all the children available to generate the aggregate mesh? */
  bool allMeshesAvailable = true;
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;

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
                                       1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

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
  if (!allMeshesAvailable) {
    return false;
  }

  /* OK to generate the mesh! Go! */
  aggObject->mLastGenerateTime = curTime;
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

  uint32 numAddedSubMeshGeometries = 0;
  // Make sure we've got all the Meshdatas
  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;

    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;

    std::string meshName = mLoc->mesh(child_uuid);
    if (!m && meshName != "") {
        boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
        if (mMeshStore.find(meshName) != mMeshStore.end()) {
          mAggregateObjects[child_uuid]->mMeshdata = mMeshStore[meshName];
        }
    }
  }

  // And finally, when we do, perform the merge

  // Tracks textures so we can fill in agg_mesh->textures when we're
  // done copying data in. Also tracks mapping of texture filename ->
  // original texture URL so we can tell the CDN to reuse that data.
  std::tr1::unordered_map<String, String> textureSet;

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end()) {
      continue;
    }
    MeshdataPtr m = mAggregateObjects[child_uuid]->mMeshdata;
    std::string meshName = mLoc->mesh(child_uuid);
    lock.unlock();

    if (!m || meshName == "") continue;

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
              if (!tex_it->uri.empty()) {
                  Transfer::URI orig_tex_uri;

                  // We need to handle relative and absolute URIs:

                  // If we can decode a URL, then we may need to handle relative URLs.
                  Transfer::URL parent_url(meshName);
                  if (!parent_url.empty()) {
                      // The URL constructor will figure out absolute/relative, or just fail
                      // if it can't construct a valid URL.
                      Transfer::URL deriv_url(parent_url.context(), tex_it->uri);
                      if (!deriv_url.empty())
                          orig_tex_uri = Transfer::URI(deriv_url.toString());
                  }

                  // Otherwise, try to decode as a plain URI, ignoring the original URI. This
                  // will either succeed with a full URI or fail and return an
                  // empty URI
                  if (orig_tex_uri.empty())
                      orig_tex_uri = Transfer::URI(tex_it->uri);

                  textureSet[tex_it->uri] = orig_tex_uri.toString();
              }
      }
      // Copy Lights
      agg_mesh->lights.insert(agg_mesh->lights.end(),
          m->lights.begin(),
          m->lights.end());
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
  for (std::tr1::unordered_map<String, String>::iterator it = textureSet.begin(); it != textureSet.end(); it++)
      agg_mesh->textures.push_back( it->first );

  for (uint32 i= 0; i < children.size(); i++) {
    UUID child_uuid = children[i]->mUUID;
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    assert( mAggregateObjects.find(child_uuid) != mAggregateObjects.end()) ;
    mAggregateObjects[child_uuid]->mMeshdata = std::tr1::shared_ptr<Meshdata>();
  }


  String localMeshName = boost::lexical_cast<String>(aggObject->mTreeLevel) + "_aggregate_mesh_" + uuid.toString() + ".dae";
  //Simplify the mesh...
  mMeshSimplifier.simplify(agg_mesh, 5000);

  //... and now create the collada file, upload to the CDN and update LOC.
  // We have two paths here, the real CDN upload and the old, local approach
  // where we dump the file and run a script to "upload" it, which may just mean
  // moving it somewhere locally
  if (mOAuth && !mCDNUsername.empty()) {
      // TODO(ewencp,tazim) Because we have to return true here and it
      // seems like things rely on all the data filled in, we have to
      // hack our way around the async upload process here. This is
      // bad for a number of reasons, not least because this could
      // potentially block up threads for quite awhile. However, this
      // isn't currently *that* bad since we know we're on a different
      // strand anyway.

      std::stringstream model_ostream(std::ofstream::out | std::ofstream::binary);
      bool converted = mModelsSystem->convertVisual(agg_mesh, "colladamodels", model_ostream);

      Transfer::UploadRequest::StringMap files;
      files[localMeshName] = model_ostream.str();

      String upload_path = "aggregates/" + localMeshName;
      Transfer::UploadRequest::StringMap params;
      params["username"] = mCDNUsername;
      params["title"] = String("Aggregate Mesh ") + uuid.toString();
      params["main_filename"] = localMeshName;
      params["ephemeral"] = "1";
      params["ttl_time"] = boost::lexical_cast<String>(mModelTTL.seconds());
      // Translate subfile map to JSON
      namespace json = json_spirit;
      json::Value subfiles = json::Object();
      for (std::tr1::unordered_map<String, String>::iterator it = textureSet.begin(); it != textureSet.end(); it++) {

          String subfile_name = it->first;
          {
              // The upload expects only the basename, i.e. the actual
              // filename. So if we referred to a URL
              // (meerkat://foo/bar/texture.png) or filename with
              // directories (/foo/bar/texture.png) we'd need to
              // extract just the filename.

              // Use URL to strip of any URL parts to get a normal looking path
              Transfer::URL subfile_name_as_url(subfile_name);
              if (!subfile_name_as_url.empty())
                  subfile_name = subfile_name_as_url.fullpath();

              // Then extract just the last element, i.e. the filename
              std::size_t filename_pos = subfile_name.rfind("/");
              if (filename_pos != String::npos)
                  subfile_name = subfile_name.substr(filename_pos+1);
          }

          String tex_path = Transfer::URL(it->second).fullpath();
          {
              // Sigh. Yet again, we need to modify the order of the filename
              // and version number again.

              std::size_t upload_filename_pos = tex_path.rfind("/");
              assert(upload_filename_pos != String::npos);
              std::size_t upload_num_pos = tex_path.rfind("/", upload_filename_pos-1);
              assert(upload_num_pos != String::npos);

              String num_part = tex_path.substr(upload_num_pos, (upload_filename_pos-upload_num_pos));
              String filename_part = tex_path.substr(upload_filename_pos+1);
              tex_path = tex_path.substr(0, upload_num_pos+1) + filename_part + num_part;
          }

          // Explicitly override the separator to ensure we don't
          // use parts of the filename to generate a hierarchy,
          // i.e. so that foo.png remains foo.png instead of
          // becoming foo { png : value }.
          subfiles.put(subfile_name, tex_path, '\0');
      }
      params["subfiles"] = json::write(subfiles);

      AtomicValue<bool> finished(false);
      Transfer::URI generated_uri;
      Transfer::TransferRequestPtr req(
          new Transfer::UploadRequest(
              mOAuth,
              files, upload_path, params, 1.0f,
              std::tr1::bind(
                  &AggregateManager::handleUploadFinished, this,
                  std::tr1::placeholders::_1, std::tr1::placeholders::_2,
                  &finished, &generated_uri
              )
          )
      );
      mTransferPool->addRequest(req);

      // Busy wait until request finishes
      while(!finished.read()) {
          Timer::sleep(Duration::milliseconds(100));
      }

      if (generated_uri.empty()) {
          AGG_LOG(error, "Failed to upload aggregate mesh " << localMeshName << ", composed of these children meshes:");

          boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
          for (uint32 i= 0; i < children.size(); i++) {
              UUID child_uuid = children[i]->mUUID;
              if ( mAggregateObjects.find(child_uuid) == mAggregateObjects.end())
                  continue;
              String meshName = mLoc->mesh(child_uuid);
              AGG_LOG(error, "   " << meshName);
          }
          return true;
      }

      // The current CDN URL layout is kind of a pain. We'll get back something
      // like:
      // meerkat://localhost/echeslack/apiupload/multimtl.dae/13
      // and the target model will look something like:
      // meerkat://localhost/echeslack/apiupload/multimtl.dae/original/13/multimtl.dae
      // so we need to extract the number at the end so we can insert it between
      // the format and the filename.
      String cdnMeshName = generated_uri.toString();
      // Store this info and mark it for TTL refresh 3/4 through the TTL. We
      // only want the path part (i.e. /username/foo/model.dae/0) and not the
      // protocol + host part (i.e. meerkat://foo.com:8000).
      aggObject->cdnBaseName = Transfer::URL(cdnMeshName).fullpath();
      aggObject->refreshTTL = mLoc->context()->recentSimTime() + (mModelTTL*.75);
      // And extract the URL to hand out to users
      std::size_t upload_num_pos = cdnMeshName.rfind("/");
      assert(upload_num_pos != String::npos);
      String mesh_num_part = cdnMeshName.substr(upload_num_pos+1);
      cdnMeshName = cdnMeshName.substr(0, upload_num_pos);
      cdnMeshName = cdnMeshName + "/original/" + mesh_num_part + "/" + localMeshName;
      agg_mesh->uri = cdnMeshName;

      //Update loc
      mLoc->updateLocalAggregateMesh(uuid, cdnMeshName);

  }
  else {
      std::string cdnMeshName = "file:///home/tahir/Desktop/aggregates_meshes/" + localMeshName;
      agg_mesh->uri = cdnMeshName;

      String modelFilename = std::string("/home/tahir/Desktop/aggregates_meshes/") + localMeshName;
      std::ofstream model_ostream(modelFilename.c_str(), std::ofstream::out | std::ofstream::binary);
      bool converted = mModelsSystem->convertVisual(agg_mesh, "colladamodels", model_ostream);
      model_ostream.close();
      if (!converted) {
          AGG_LOG(error, "Failed to save aggregate mesh " << localMeshName << ", it won't be displayed.");
          // Here the return value isn't success, it's "should I remove this
          // aggregate object from the queue for processing." Failure to save is
          // effectively fatal for the aggregate, so tell it to get removed.
          return true;
      }

      //Update loc
      mLoc->updateLocalAggregateMesh(uuid, cdnMeshName);
  }


  //Keep the meshstore's memory usage under control.
  {
    boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);
    if (mMeshStore.size() > 200) {
      mMeshStore.erase(mMeshStore.begin());
    }
  }

  aggObject->mLeaves.clear();

  return true;
}

void AggregateManager::handleUploadFinished(Transfer::UploadRequestPtr request, const Transfer::URI& path, AtomicValue<bool>* finished_out, Transfer::URI* generated_uri_out) {
    *generated_uri_out = path;
    *finished_out = true;
}

void AggregateManager::metadataFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName, uint8 attemptNo,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)
{
  if (response != NULL) {
      AGG_LOG(detailed, ( Timer::now() - t )  << " : metadataFinished SUCCESS\n");

    const Transfer::RemoteFileMetadata metadata = *response;

    Transfer::TransferRequestPtr req(new Transfer::ChunkRequest(response->getURI(), metadata,
                                               response->getChunkList().front(), 1.0,
                                             std::tr1::bind(&AggregateManager::chunkFinished, this, t,uuid, child_uuid, meshName,
                                                              std::tr1::placeholders::_1,
                                                              std::tr1::placeholders::_2) ) );

    mTransferPool->addRequest(req);
  }
  else if (attemptNo < 5) {
    AGG_LOG(warn, "Failed metadata download: Retrying...: Response time: "   << ( Timer::now() - t ) << "\n");
    Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, t, uuid, child_uuid, meshName,
                                       attemptNo+1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

    mTransferPool->addRequest(req);

  }
}

void AggregateManager::chunkFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName,
                                       std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                       std::tr1::shared_ptr<const Transfer::DenseData> response)
{
    if (response != NULL) {
      AGG_LOG(detailed, "Time spent downloading: " << (Timer::now() - t) << "\n");

      boost::mutex::scoped_lock aggregateObjectsLock(mAggregateObjectsMutex);
      if (mAggregateObjects[child_uuid]->mMeshdata == MeshdataPtr() ) {

        VisualPtr v = mModelsSystem->load(request->getMetadata(), request->getMetadata().getFingerprint(), response);
        // FIXME handle non-Meshdata formats
        MeshdataPtr m = std::tr1::dynamic_pointer_cast<Meshdata>(v);

        mAggregateObjects[child_uuid]->mMeshdata = m;

        aggregateObjectsLock.unlock();

        {
          boost::mutex::scoped_lock meshStoreLock(mMeshStoreMutex);

          mMeshStore[request->getURI().toString()] = m;

          AGG_LOG(detailed, "Stored mesh in mesh store for: " <<  request->getURI().toString() << "\n");
        }
      }
    }
    else {
      AGG_LOG(warn, "ChunkFinished fail... retrying\n");
      Transfer::TransferRequestPtr req(
                                       new Transfer::MetadataRequest( Transfer::URI(meshName), 1.0, std::tr1::bind(
                                       &AggregateManager::metadataFinished, this, t, uuid, child_uuid, meshName,
                                       1, std::tr1::placeholders::_1, std::tr1::placeholders::_2)));

      mTransferPool->addRequest(req);
    }
}

std::vector<AggregateManager::AggregateObjectPtr >& AggregateManager::getChildren(const UUID& uuid) {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
  static std::vector<AggregateObjectPtr> emptyVector;

  if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
    return emptyVector;
  }

  std::vector<AggregateObjectPtr>& children = mAggregateObjects[uuid]->mChildren;

  return children;
}

void AggregateManager::getLeaves(const std::vector<UUID>& individualObjects) {
  for (uint32 i=0; i<individualObjects.size(); i++) {
    const  UUID& indl_uuid = individualObjects[i];
    UUID uuid = indl_uuid;

    std::tr1::shared_ptr<AggregateObject> obj = mAggregateObjects[uuid];

    if (!mLoc->contains(uuid)) continue;

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
            UUID& child_uuid = aggObject->mChildren[i]->mUUID;

            if (!mLoc->contains(child_uuid)) continue;

            BoundingSphere3f bnds = mLoc->bounds(child_uuid);
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

        if (returner || aggObject->mNumFailedGenerationAttempts > 20) {
          aggObject->mNumFailedGenerationAttempts = 0;
          it->second.pop_front();
        }
        else {
          aggObject->mNumFailedGenerationAttempts++;
        }

        break;
      }
    }

    mDirtyAggregateObjects.clear();

    if (mObjectsByPriority.size() > 0) {
      Duration dur = (returner) ? Duration::milliseconds(1.0) : Duration::milliseconds(25.0);
      mAggregationStrand->post(
          dur,
          std::tr1::bind(&AggregateManager::generateMeshesFromQueue, this, curTime),
          "AggregateManager::generateMeshesFromQueue"
      );
    }
}

void AggregateManager::updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel) {
    //mAggregateObjectsMutex MUST be locked BEFORE calling this function.

    /*Check for the rare case where an aggregate may be removed (through removeAggregate)
      before it is cleared from its parents' list of children (through removeChild). */
    if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
      return;
    }

    mAggregateObjects[uuid]->mTreeLevel = treeLevel;

    for (uint32 i = 0; i < mAggregateObjects[uuid]->mChildren.size(); i++) {
      updateChildrenTreeLevel(mAggregateObjects[uuid]->mChildren[i]->mUUID, treeLevel+1);
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

bool AggregateManager::findChild(std::vector<AggregateManager::AggregateObjectPtr>& v,
                                 const UUID& uuid)
{
  for (uint32 i=0; i < v.size(); i++) {
    if (v[i]->mUUID == uuid) {
      return true;
    }
  }

  return false;
}

void AggregateManager::removeChild(std::vector<AggregateManager::AggregateObjectPtr>& v,
                                   const UUID& uuid)
{
  for (uint32 i=0; i < v.size(); i++) {
    if (v[i]->mUUID == uuid) {
      v.erase(v.begin() + i);
      return;
    }
  }
}

void AggregateManager::removeStaleLeaves() {
  boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

  std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher >::iterator it =
                                                           mAggregateObjects.begin();
  std::vector<UUID> markedForRemoval;

  while (it != mAggregateObjects.end()) {
    if (it->second->leaf && it->second.use_count() == 1) {
      markedForRemoval.push_back(it->first);
    }

    it++;
  }

  for (uint32 i = 0; i < markedForRemoval.size(); i++) {
    mAggregateObjects.erase(markedForRemoval[i]);
  }


  mAggregationStrand->post(
        Duration::seconds(60),
        std::tr1::bind(&AggregateManager::removeStaleLeaves, this),
        "AggregateManager::removeStaleLeaves"
  );

}


namespace {
// Return true if the service is a default value
bool ServiceIsDefault(const String& s) {
    if (!s.empty() && s != "http" && s != "80")
        return false;
    return true;
}
// Return empty if this is already empty or the default for this
// service. Otherwise, return the old value
String ServiceIfNotDefault(const String& s) {
    if (!ServiceIsDefault(s))
        return s;
    return "";
}
}


void AggregateManager::sendKeepAlives() {
    // Don't bother unless we're uploading to the real CDN
    if (!mOAuth || mCDNUsername.empty())
        return;

    Time tnow = mLoc->context()->recentSimTime();

    // This could definitely be more efficient...
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    for(AggregateObjectsMap::iterator it = mAggregateObjects.begin(); it != mAggregateObjects.end(); it++) {
        if (it->second->refreshTTL != Time::null() &&
            it->second->refreshTTL < tnow &&
            !it->second->cdnBaseName.empty())
        {
            String keep_alive_path = "/api/keepalive" + it->second->cdnBaseName;
            // Currently there's no keepalive support in the Transfer
            // libraries, so we construct and send the http request ourselves.

            Network::Address cdn_addr(mOAuth->hostname, ServiceIfNotDefault(mOAuth->service));
            String full_oauth_hostinfo = mOAuth->hostname;
            if (!ServiceIsDefault(mOAuth->service))
                full_oauth_hostinfo += ":" + mOAuth->service;

            Transfer::HttpManager::Headers headers;
            headers["Host"] = full_oauth_hostinfo;

            Transfer::HttpManager::QueryParameters query_params;
            query_params["username"] = mCDNUsername;
            query_params["ttl"] = boost::lexical_cast<String>(mModelTTL.seconds());

            AGG_LOG(detailed, "Requesting TTL refresh for " << it->first);
            Transfer::OAuthHttpManager oauth_http(mOAuth);
            oauth_http.get(
                cdn_addr, keep_alive_path,
                std::tr1::bind(&AggregateManager::handleKeepAliveResponse, this, it->first, _1, _2, _3),
                headers, query_params
            );
        }
    }
}

void AggregateManager::handleKeepAliveResponse(const UUID& objid,
    std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> response,
    Transfer::HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error)
{
    // Check a bunch of error conditions, leaving the refresh TTL setting for
    // the next iteration is something went wrong.

    if (error == Transfer::HttpManager::REQUEST_PARSING_FAILED) {
        AGG_LOG(error, "Request parsing failed during aggregate TTL refresh (" << objid << ")");
        return;
    } else if (error == Transfer::HttpManager::RESPONSE_PARSING_FAILED) {
        AGG_LOG(error, "Response parsing failed during aggregate TTL refresh (" << objid << ")");
        return;
    } else if (error == Transfer::HttpManager::BOOST_ERROR) {
        AGG_LOG(error, "A boost error happened during aggregate TTL refresh (" << objid << "). Boost error = " << boost_error.message());
        return;
    } else if (error != Transfer::HttpManager::SUCCESS) {
        AGG_LOG(error, "An unknown error happened during aggregate TTL refresh. (" << objid << ")");
        return;
    }

    if (response->getStatusCode() != 200) {
        AGG_LOG(error, "HTTP status code = " << response->getStatusCode() << " instead of 200 during aggregate TTL refresh (" << objid << ")");
        return;
    }

    Transfer::DenseDataPtr response_data = response->getData();
    if (response_data && response_data->size() != 0) {
        AGG_LOG(error, "Got non-empty response durring aggregate TTL refresh: " << response_data->asString());
        return;
    }

    // If all these passed, then we were successful. Setup next refresh time
    AGG_LOG(detailed, "Successfully refreshed TTL for " << objid);
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);
    AggregateObjectsMap::iterator it = mAggregateObjects.find(objid);
    if (it == mAggregateObjects.end()) return;
    it->second->refreshTTL = mLoc->context()->recentSimTime() + (mModelTTL*.75);
}



}
