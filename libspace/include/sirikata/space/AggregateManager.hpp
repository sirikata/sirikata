/*  Sirikata
 *  AggregateManager.hpp
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


#ifndef _AGGREGATE_MANAGER_HPP
#define _AGGREGATE_MANAGER_HPP

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

#include <sirikata/space/LocationService.hpp>


#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/ModelsSystem.hpp>

#include <sirikata/mesh/MeshSimplifier.hpp>

#include <sirikata/core/transfer/HttpManager.hpp>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT AggregateManager {
private:

  Thread* mAggregationThread;

  Network::IOService* mAggregationService;
  Network::IOStrand* mAggregationStrand;
  Network::IOWork* mIOWork;

  LocationService* mLoc;
  ModelsSystem* mModelsSystem;
  Sirikata::Mesh::MeshSimplifier mMeshSimplifier;

  typedef struct AggregateObject{
    UUID mUUID;
    UUID mParentUUID;

    std::vector< std::tr1::shared_ptr<struct AggregateObject>  > mChildren;

    // Whether this is actually a leaf object (i.e. added implicitly as
    // AggregateObject when added as a child of a true aggregate).
    bool leaf;

    Time mLastGenerateTime;

    bool generatedLastRound;

    Mesh::MeshdataPtr mMeshdata;

      AggregateObject(const UUID& uuid, const UUID& parentUUID, bool is_leaf) :
       mUUID(uuid), mParentUUID(parentUUID),
       leaf(is_leaf),
       mLastGenerateTime(Time::null()),
       mTreeLevel(0),  mNumObservers(0),
       mNumFailedGenerationAttempts(0),
       cdnBaseName(),
       refreshTTL(Time::null())
    {
      mMeshdata = Mesh::MeshdataPtr();
      generatedLastRound = false;
      mDistance = 0.01;
    }

    uint16 mTreeLevel;
    uint32 mNumObservers;
    uint32 mNumFailedGenerationAttempts;

    std::vector<UUID> mLeaves;
    double mDistance;  //MINIMUM distance at which this object could be part of a cut


      // The basename returned by the CDN. This points at the entire asset
      // rather than the particular mesh filename. Should include a version
      // number. Used for refreshing TTLs.
      String cdnBaseName;
      // Time at which we should try to refresh the TTL, should be set
      // a bit less than the actual timeout.
      Time refreshTTL;

  } AggregateObject;
  typedef std::tr1::shared_ptr<AggregateObject> AggregateObjectPtr;

  void getLeaves(const std::vector<UUID>& mIndividualObjects);


  boost::mutex mAggregateObjectsMutex;
  typedef std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher > AggregateObjectsMap;
  AggregateObjectsMap mAggregateObjects;

  boost::mutex mMeshStoreMutex;
  std::tr1::unordered_map<String, Mesh::MeshdataPtr> mMeshStore;

  std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
  Transfer::TransferMediator *mTransferMediator;

  Time mAggregateGenerationStartTime;

  std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher> mDirtyAggregateObjects;
  std::map<float, std::deque<AggregateObjectPtr > > mObjectsByPriority;

  std::vector<AggregateObjectPtr>& getChildren(const UUID& uuid) ;


  Transfer::OAuthParamsPtr mOAuth;
  const String mCDNUsername;
  Duration mModelTTL;
  Poller* mCDNKeepAlivePoller;

  void updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel);
  void addDirtyAggregates(UUID uuid);

  void generateMeshesFromQueue(Time postTime);
  void generateAggregateMeshAsyncIgnoreErrors(const UUID uuid, Time postTime, bool generateSiblings = true);
  bool generateAggregateMeshAsync(const UUID uuid, Time postTime, bool generateSiblings = true);
  void aggregationThreadMain();

  // Helper that handles the upload callback and sets flags to let the request
  // from the aggregation thread to continue
  void handleUploadFinished(Transfer::UploadRequestPtr request, const Transfer::URI& path, AtomicValue<bool>* finished_out, Transfer::URI* generated_uri_out);

  // Helper for cleaning out parent state from child, or deleting it if it is an
  // abandoned leaf object (non-aggregate). Returns true if the object was
  // removed.
  bool cleanUpChild(const UUID& parent_uuid, const UUID& child_id);

  void removeStaleLeaves();


  // Look for any aggregates that need a keep-alive sent to the CDN
  // and try to send them.
  void sendKeepAlives();
  void handleKeepAliveResponse(const UUID& objid,
      std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> response,
      Transfer::HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error);

public:

  AggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username);

  ~AggregateManager();

  void addAggregate(const UUID& uuid);

  void removeAggregate(const UUID& uuid);

  void addChild(const UUID& uuid, const UUID& child_uuid) ;

  void removeChild(const UUID& uuid, const UUID& child_uuid);

  void aggregateObserved(const UUID& objid, uint32 nobservers);

  bool findChild(std::vector<AggregateObjectPtr>& v, const UUID& uuid) ;

  void removeChild(std::vector<AggregateObjectPtr>& v, const UUID& uuid) ;


  // This version requires locking to get at the AggregateObjectPtr
  // for the object. This isn't safe if you already hold that lock.
  void generateAggregateMesh(const UUID& uuid, const Duration& delayFor = Duration::milliseconds(1.0f) );
  // This version doesn't require a lock.
  void generateAggregateMesh(const UUID& uuid, AggregateObjectPtr aggObject, const Duration& delayFor = Duration::milliseconds(1.0f) );

  void metadataFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName,uint8 attemptNo,
                        std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                        std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)  ;

  void chunkFinished(Time t, const UUID uuid, const UUID child_uuid, std::string meshName, std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                      std::tr1::shared_ptr<const Transfer::DenseData> response);


};

}

#endif
