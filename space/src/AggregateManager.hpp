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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

#include <sirikata/space/LocationService.hpp>


#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/ModelsSystem.hpp>

#include <sirikata/core/util/MeshSimplifier.hpp>


namespace Sirikata {

class AggregateManager {
private:

  SpaceContext* mContext;
  LocationService* mLoc;

  ModelsSystem* mModelsSystem;

  MeshSimplifier mMeshSimplifier;


  typedef struct AggregateObject{
    UUID mUUID;
    UUID mParentUUID;

    std::vector<UUID> mChildren;
    std::tr1::shared_ptr<Meshdata> mMeshdata;

    Time mLastGenerateTime;

    AggregateObject(const UUID& uuid, const UUID& parentUUID) :
      mUUID(uuid), mParentUUID(parentUUID), mLastGenerateTime(Time::null()),
      mTreeLevel(0)
    {
      mMeshdata = std::tr1::shared_ptr<Meshdata>();
    }

    uint16 mTreeLevel;

  } AggregateObject;


  boost::mutex mAggregateObjectsMutex;
  std::tr1::unordered_map<UUID, std::tr1::shared_ptr<AggregateObject>, UUID::Hasher > mAggregateObjects;

  boost::mutex mMeshStoreMutex;
  std::tr1::unordered_map<String, MeshdataPtr> mMeshStore;

  std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
  Transfer::TransferMediator *mTransferMediator;

  bool mThreadRunning;

  boost::mutex mUploadQueueMutex;

  std::map<UUID, std::tr1::shared_ptr<Meshdata> > mUploadQueue;

  boost::condition_variable mCondVar;

  Time mAggregateGenerationStartTime;

  std::tr1::unordered_map<UUID, std::tr1::shared_ptr<AggregateObject>, UUID::Hasher> mDirtyAggregateObjects;

  std::map<uint16, std::deque<std::tr1::shared_ptr<AggregateObject> > > mObjectsByPriority;

  std::vector<UUID>& getChildren(const UUID& uuid);

  void generateMeshesFromQueue(Time postTime);

  void updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel);

  void generateAggregateMeshAsync(const UUID uuid, Time postTime);

  void uploadQueueServiceThread();


public:

  AggregateManager(SpaceContext* ctx, LocationService* loc) ;

  ~AggregateManager();

  void addAggregate(const UUID& uuid);

  void removeAggregate(const UUID& uuid);

  void addChild(const UUID& uuid, const UUID& child_uuid) ;

  void removeChild(const UUID& uuid, const UUID& child_uuid);


  void uploadMesh(const UUID& uuid, std::tr1::shared_ptr<Meshdata> meshptr);

  void generateAggregateMesh(const UUID& uuid, const Duration& delayFor = Duration::milliseconds(1.0f) );

  void metadataFinished(const UUID uuid, const UUID child_uuid, std::string meshName,
                        std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                        std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response)  ;

  void chunkFinished(const UUID uuid, const UUID child_uuid,std::tr1::shared_ptr<Transfer::ChunkRequest> request,
                                       std::tr1::shared_ptr<const Transfer::DenseData> response);

};

}

#endif
