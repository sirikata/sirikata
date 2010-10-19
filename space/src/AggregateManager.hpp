

#ifndef _AGGREGATE_MANAGER_HPP
#define _AGGREGATE_MANAGER_HPP

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>

#include <sirikata/space/LocationService.hpp>


#include <sirikata/proxyobject/Meshdata.hpp>
#include <sirikata/proxyobject/ModelsSystem.hpp>

namespace Sirikata {

class AggregateManager {
private:

  SpaceContext* mContext;
  LocationService* mLoc;

  ModelsSystem* mModelsSystem;

  typedef struct AggregateObject{
    UUID mUUID;
    UUID mParentUUID;

    std::vector<UUID> mChildren;
    std::tr1::shared_ptr<Meshdata> mMeshdata;

    Time mLastGenerateTime;

    AggregateObject(const UUID& uuid, const UUID& parentUUID) :
      mUUID(uuid), mParentUUID(parentUUID), mLastGenerateTime(Time::null())
    {
      mMeshdata = std::tr1::shared_ptr<Meshdata>();
    }
    
  } AggregateObject;


  boost::mutex mAggregateObjectsMutex;
  std::tr1::unordered_map<UUID, std::tr1::shared_ptr<AggregateObject>, UUID::Hasher > mAggregateObjects;

  boost::mutex mMeshStoreMutex;
  std::tr1::unordered_map<String, MeshdataPtr> mMeshStore; 

  std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
  Transfer::TransferMediator *mTransferMediator;

  bool mThreadRunning;



  std::vector<UUID> mEmptyVector;
  std::vector<UUID>& getChildren(const UUID& uuid) {
    boost::mutex::scoped_lock lock(mAggregateObjectsMutex);

    if (mAggregateObjects.find(uuid) == mAggregateObjects.end()) {
      return mEmptyVector;
    }

    std::vector<UUID>& children = mAggregateObjects[uuid]->mChildren;

    return children;
  }

  void generateAggregateMeshAsync(const UUID uuid, Time postTime);



  boost::mutex mUploadQueueMutex;

  std::map<UUID, std::tr1::shared_ptr<Meshdata> > mUploadQueue;

  boost::condition_variable mCondVar;

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
