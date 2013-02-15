// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _MESH_AGGREGATE_MANAGER_HPP_
#define _MESH_AGGREGATE_MANAGER_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/core/transfer/TransferData.hpp>
#include <sirikata/core/transfer/RemoteFileMetadata.hpp>
#include <sirikata/core/transfer/TransferPool.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>

#include <sirikata/space/LocationService.hpp>


#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/mesh/ModelsSystem.hpp>
#include <sirikata/mesh/MeshSimplifier.hpp>
#include <sirikata/mesh/Filter.hpp>

#include <sirikata/core/transfer/HttpManager.hpp>

#include <sirikata/core/command/Command.hpp>

#include <boost/thread/locks.hpp>
#include <prox/base/ZernikeDescriptor.hpp>




namespace Sirikata {

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


class SIRIKATA_SPACE_EXPORT MeshAggregateManager : public AggregateManager {
private:

  enum{MAX_NUM_GENERATION_THREADS=16};
  uint16 mNumGenerationThreads;
  Thread* mAggregationThreads[MAX_NUM_GENERATION_THREADS];
  Network::IOService* mAggregationServices[MAX_NUM_GENERATION_THREADS];
  Network::IOStrand* mAggregationStrands[MAX_NUM_GENERATION_THREADS];
  Network::IOWork* mIOWorks[MAX_NUM_GENERATION_THREADS];


  typedef struct LocationInfo {
  private:
      boost::shared_mutex mutex;

    Vector3f mCurrentPosition;
    AggregateBoundingInfo mBounds;
    Quaternion mCurrentOrientation;
    String mMesh;

  public:
    LocationInfo(Vector3f curPos, AggregateBoundingInfo bnds,
                 Quaternion curOrient, String msh) :
      mCurrentPosition(curPos), mBounds(bnds),
      mCurrentOrientation(curOrient), mMesh(msh)
    {
    }

      Vector3f currentPosition() {
          boost::shared_lock<boost::shared_mutex> read_lock(mutex);
          return mCurrentPosition;
      }
      AggregateBoundingInfo bounds() {
          boost::shared_lock<boost::shared_mutex> read_lock(mutex);
          return mBounds;
      }
      Quaternion currentOrientation() {
          boost::shared_lock<boost::shared_mutex> read_lock(mutex);
          return mCurrentOrientation;
      }
      String mesh() {
          boost::shared_lock<boost::shared_mutex> read_lock(mutex);
          return mMesh;
      }

      void currentPosition(Vector3f v) {
          boost::unique_lock<boost::shared_mutex> write_lock(mutex);
          mCurrentPosition = v;
      }
      void bounds(AggregateBoundingInfo v) {
          boost::unique_lock<boost::shared_mutex> write_lock(mutex);
          mBounds = v;
      }
      void currentOrientation(Quaternion v) {
          boost::unique_lock<boost::shared_mutex> write_lock(mutex);
          mCurrentOrientation = v;
      }
      void mesh(const String& v) {
          boost::unique_lock<boost::shared_mutex> write_lock(mutex);
          mMesh = v;
      }
  } LocationInfo;
  std::tr1::shared_ptr<LocationInfo> getCachedLocInfo(const UUID& uuid) ;

  class LocationServiceCache {
    public:
      LocationServiceCache() { }

      std::tr1::shared_ptr<LocationInfo> getLocationInfo(const UUID& uuid) {
        if (mLocMap.find(uuid) == mLocMap.end()){
          return std::tr1::shared_ptr<LocationInfo>();
        }

        return mLocMap[uuid];
      }

      void insertLocationInfo(const UUID& uuid, std::tr1::shared_ptr<LocationInfo> locinfo) {
        mLocMap[uuid] = locinfo;
      }

      void removeLocationInfo(const UUID& uuid) {
        mLocMap.erase(uuid);
      }

    private:
      std::tr1::unordered_map<UUID, std::tr1::shared_ptr<LocationInfo> , UUID::Hasher> mLocMap;
  };

  LocationServiceCache mLocationServiceCache;
  boost::mutex mLocCacheMutex;
  LocationService* mLoc;

  //Part of the LocationServiceListener interface.
  virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient,
                              const AggregateBoundingInfo& bounds, const String& mesh, const String& physics,
                                const String& query_data);
  virtual void localObjectRemoved(const UUID& uuid, bool agg) ;
  virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
  virtual void localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
  virtual void localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) ;
  virtual void localMeshUpdated(const UUID& uuid, bool agg, const String& newval) ;
  virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient,
      const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& query_data);
  virtual void replicaObjectRemoved(const UUID& uuid);
  virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
  virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
  virtual void replicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval);
  virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);


  boost::mutex mModelsSystemMutex;
  ModelsSystem* mModelsSystem;
  Sirikata::Mesh::MeshSimplifier mMeshSimplifier;
  boost::mutex mCenteringFilterMutex;
  Sirikata::Mesh::Filter* mCenteringFilter;

  boost::mutex mSquashFilterMutex;
  Sirikata::Mesh::Filter* mSquashFilter;

  struct AggregateObject;
  typedef std::tr1::shared_ptr<AggregateObject> AggregateObjectPtr;
  struct AggregateObject{
    UUID mUUID;
    std::set<UUID> mParentUUIDs;
  private:
      boost::mutex mChildrenMutex;
      std::vector< std::tr1::shared_ptr<struct AggregateObject>  > mChildren;
  public:
      std::vector<AggregateObjectPtr> getChildrenCopy(){
          boost::mutex::scoped_lock lock(mChildrenMutex);
          return mChildren;
      }
      std::size_t childrenSize() {
          boost::mutex::scoped_lock lock(mChildrenMutex);
          return mChildren.size();
      }
      void addChild(AggregateObjectPtr child) {
          boost::mutex::scoped_lock lock(mChildrenMutex);
          mChildren.push_back(child);
      }
      bool hasChild(const UUID& child) {
          boost::mutex::scoped_lock lock(mChildrenMutex);
          for (uint32 i=0; i < mChildren.size(); i++)
              if (mChildren[i]->mUUID == child) return true;
          return false;
      }
      void removeChild(const UUID& child) {
          boost::mutex::scoped_lock lock(mChildrenMutex);
          for (uint32 i=0; i < mChildren.size(); i++) {
              if (mChildren[i]->mUUID == child) {
                  mChildren.erase(mChildren.begin() + i);
                  return;
              }
          }
      }
    // Whether this is actually a leaf object (i.e. added implicitly as
    // AggregateObject when added as a child of a true aggregate).
    bool leaf;
    Time mLastGenerateTime;
    bool generatedLastRound;
    Mesh::MeshdataPtr mMeshdata;

    AggregateObject(const UUID& uuid, const UUID& parentUUID, bool is_leaf) :
      mUUID(uuid),
      leaf(is_leaf),
      mLastGenerateTime(Time::null()),
      mTreeLevel(0),  mNumObservers(0),
      mNumFailedGenerationAttempts(0),
      geometricError(0), mSerializedSize(0),
      cdnBaseName(),
      mAtlasPath(""),
      refreshTTL(Time::null())
    {
      mParentUUIDs.insert(parentUUID);
      mMeshdata = Mesh::MeshdataPtr();
      generatedLastRound = false;
      mDistance = 0.01;
      mTriangleCount = 0;
    }

    uint16 mTreeLevel;
    uint32 mNumObservers;
    uint32 mNumFailedGenerationAttempts;
    double mDistance;  //MINIMUM distance at which this object could be part of a cut
    uint32 mTriangleCount;
    float64 geometricError;
    uint32 mSerializedSize;
    //std::tr1::unordered_set<std::tr1::shared_ptr<AggregateObject> > mLeaves;

    boost::mutex mAtlasAndUploadMutex;
    // The basename returned by the CDN. This points at the entire asset
    // rather than the particular mesh filename. Should include a version
    // number. Used for refreshing TTLs.
    String cdnBaseName;
    String mAtlasPath;
    // Time at which we should try to refresh the TTL, should be set
    // a bit less than the actual timeout.
    Time refreshTTL;

  };


  //Lists of all aggregate objects and dirty aggregate objects.
  boost::mutex mAggregateObjectsMutex;
  typedef std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher > AggregateObjectsMap;
  AggregateObjectsMap mAggregateObjects;
  Time mAggregateGenerationStartTime;
  std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher> mDirtyAggregateObjects;

  boost::mutex mQueuedObjectsMutex;
  std::tr1::unordered_map<UUID, std::pair<uint32,uint32>, UUID::Hasher> mQueuedObjects;

  boost::mutex mObjectsByPriorityLocks[MAX_NUM_GENERATION_THREADS];
  std::map<float, std::deque<AggregateObjectPtr > > mObjectsByPriority[MAX_NUM_GENERATION_THREADS];

  //Variables related to downloading and in-memory caching meshes
  boost::mutex mMeshStoreMutex;
  std::tr1::unordered_map<String, Mesh::MeshdataPtr> mMeshStore;
  std::map<int, String> mMeshStoreOrdering;
  int mCurrentInsertionNumber;

  std::tr1::unordered_map<String, Prox::ZernikeDescriptor> mMeshDescriptors;
  std::tr1::shared_ptr<Transfer::TransferPool> mTransferPool;
  Transfer::TransferMediator *mTransferMediator;
  boost::mutex mResourceDownloadTasksMutex;
  std::tr1::unordered_map<String, Transfer::ResourceDownloadTaskPtr> mResourceDownloadTasks;
  bool mAtlasingNeeded;
  uint32 mSizeOfSeenTextures;
  std::tr1::unordered_set<String> mSeenTextureHashes;

  boost::mutex mTextureNameToHashMapMutex;
  std::tr1::unordered_map<String, String> mTextureNameToHashMap;

  void addToInMemoryCache(const String& meshName, const Mesh::MeshdataPtr mdptr);

  //CDN upload-related variables
  Transfer::OAuthParamsPtr mOAuth;
  const String mCDNUsername;
  Duration mModelTTL;
  Poller* mCDNKeepAlivePoller;
  String mLocalPath;
  String mLocalURLPrefix;
  bool mSkipGenerate;
  bool mSkipUpload;

  //CDN upload threads' variables
  enum{MAX_NUM_UPLOAD_THREADS = 16};
  uint16 mNumUploadThreads;
  Thread* mUploadThreads[MAX_NUM_UPLOAD_THREADS];
  Network::IOService* mUploadServices[MAX_NUM_UPLOAD_THREADS];
  Network::IOStrand* mUploadStrands[MAX_NUM_UPLOAD_THREADS];
  Network::IOWork* mUploadWorks[MAX_NUM_UPLOAD_THREADS];
  void uploadThreadMain(uint8 i);


  // Stats.
  // Raw number of aggregate updates that could cause regeneration,
  // e.g. add/remove children, child moved, child mesh changed, etc.
  AtomicValue<uint32> mRawAggregateUpdates;
  // Number of aggregate generation requests we've queued up, even if they
  // haven't finished.
  AtomicValue<uint32> mAggregatesQueued;
  // Number of aggregates that were actually generated, i.e. the mesh was saved
  // and uploading scheduled.
  AtomicValue<uint32> mAggregatesGenerated;
  // Number of aggregates which, even after retries, failed to generate
  AtomicValue<uint32> mAggregatesFailedToGenerate;
  // Number of aggregates successfully uploaded to the CDN
  AtomicValue<uint32> mAggregatesUploaded;
  // Number of aggregate uploads which failed
  AtomicValue<uint32> mAggregatesFailedToUpload;
  // Some stats need locks
  boost::mutex mStatsMutex;
  // Cumulative time spent generating aggregates (across all threads,
  // so it could be, e.g., 4x wall clock time). Doesn't include failed
  // calls to generateAggregateMeshAsync
  Duration mAggregateCumulativeGenerationTime;
  // And uploading them. Doesn't include failed uploads. This includes
  // the time spent serializing the mesh.
  Duration mAggregateCumulativeUploadTime;
  // And their size after being serialized.
  uint64 mAggregateCumulativeDataSize;

  //Various utility functions
  bool findChild(std::vector<AggregateObjectPtr>& v, const UUID& uuid) ;
  void removeChild(std::vector<AggregateObjectPtr>& v, const UUID& uuid) ;
  void iRemoveChild(const UUID& uuid, const UUID& child_uuid);
  void getLeaves(const std::vector<UUID>& mIndividualObjects);
  void addLeavesUpTree(UUID leaf_uuid, UUID uuid);
  bool isAggregate(const UUID& uuid);
  float32 minimumDistanceForCut(AggregateObjectPtr aggObj);
  void setAggregatesTriangleCount();
  //void setChildrensTriangleCount(AggregateObjectPtr aggObj);
  String serializeHashToURIMap(std::tr1::shared_ptr<std::tr1::unordered_map<String, std::vector<String> > > hashToURIMap);

  //FIXME: meshDiff and mObservers are for experiemental puirposes only.
  float meshDiff(const UUID& uuid, std::tr1::shared_ptr<Mesh::Meshdata> md1, std::tr1::shared_ptr<Mesh::Meshdata> md2);
  std::set<UUID> mObservers;


  void pca_get_rotate_matrix(Mesh::MeshdataPtr mesh, Matrix4x4f& rot_mat, Matrix4x4f& rot_mat_inv);

  void getVertexAndFaceList(Mesh::MeshdataPtr md,
                            std::tr1::unordered_set<Vector3f, Vector3f::Hasher>& positionVectors,
                            std::tr1::unordered_set<FaceContainer, FaceContainer::Hasher>& faceSet
                           );

  //Function related to generating and updating aggregates.
  void updateChildrenTreeLevel(const UUID& uuid, uint16 treeLevel);
  void addDirtyAggregates(UUID uuid);
  void queueDirtyAggregates(Time postTime);
  void generateMeshesFromQueue(uint8 i);


  enum {
      GEN_SUCCESS=1,
      CHILDREN_NOT_YET_GEN=2,
      NEWER_REQUEST=3, // newer timestamped request for aggregation
      MISSING_LOC_INFO=4,
      MISSING_CHILD_LOC_INFO=5,
      MISSING_CHILD_MESHES=6
  };
  uint32 generateAggregateMeshAsync(const UUID uuid, Time postTime, bool generateSiblings = true);
  Mesh::MeshdataPtr generateAggregateMeshAsyncFromLeaves(const UUID uuid, Time postTime);
  uint32 checkLocAndMeshInfo(const UUID& uuid, std::vector<AggregateObjectPtr>& children,
         std::tr1::unordered_map<UUID, std::tr1::shared_ptr<LocationInfo> , UUID::Hasher>& currentLocMap);
  uint32 checkMeshesAvailable(const UUID& uuid, const Time& curTime, std::vector<AggregateObjectPtr>& children,
                              std::tr1::unordered_map<UUID, std::tr1::shared_ptr<LocationInfo> , UUID::Hasher>& currentLocMap);
  uint32 checkTextureHashesAvailable(std::vector<AggregateObjectPtr>& children,
                                     std::tr1::unordered_map<String, String>& textureToHashMap);
  void startDownloadsForAtlasing(const UUID& uuid, Mesh::MeshdataPtr agg_mesh, AggregateObjectPtr aggObject, String localMeshName,
                                                 std::tr1::unordered_map<String, String>& textureSet,
                                                 std::tr1::unordered_map<String, Mesh::MeshdataPtr>& textureToModelMap);
  void replaceCityEngineTextures(Mesh::MeshdataPtr m) ;
  void deduplicateMeshes(std::vector<AggregateObjectPtr>& children, bool isLeafAggregate,
                       String* meshURIs, std::vector<Matrix4x4f>& replacementAlignmentTransforms);


  void aggregationThreadMain(uint8 i);
  void updateAggregateLocMesh(UUID uuid, String mesh);


  //Functions related to uploading aggregates
  void uploadAggregateMesh(Mesh::MeshdataPtr agg_mesh,String atlas_name,
                           std::tr1::shared_ptr<char> atlas_buffer,
                           uint32 atlas_length,
			   AggregateObjectPtr aggObject,
                           std::tr1::unordered_map<String, String> textureSet, uint32 retryAttempt, Time uploadStartTime);
  // Helper that handles the upload callback and sets flags to let the request
  // from the aggregation thread to continue
  void handleUploadFinished(Transfer::UploadRequestPtr request, const Transfer::URI& path, Mesh::MeshdataPtr agg_mesh,
			    String atlas_name,
                            std::tr1::shared_ptr<char> atlas_buffer,
                            uint32 atlas_length,
			    AggregateObjectPtr aggObject, std::tr1::unordered_map<String, String> textureSet,
                            uint32 retryAttempt, const Time& uploadStartTime);
  // Look for any aggregates that need a keep-alive sent to the CDN
  // and try to send them.
  void sendKeepAlives();
  void handleKeepAliveResponse(const UUID& objid,
             std::tr1::shared_ptr<Transfer::HttpManager::HttpResponse> response,
             Transfer::HttpManager::ERR_TYPE error, const boost::system::error_code& boost_error);



  // Helper for cleaning out parent state from child, or deleting it if it is an
  // abandoned leaf object (non-aggregate). Returns true if the object was
  // removed.
  bool cleanUpChild(const UUID& parent_uuid, const UUID& child_id);
  void removeStaleLeaves();

  // Command handlers
  void commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);


public:

  /** Create an AggregateManager.
   *
   *  \oauth OAuth upload parameters for the CDN. If omitted, uploads will not
   *         be attempted
   *  \username User name for account your OAuth params let you upload to on the the CDN
   *  \param local_path if non-empty and OAuth values are not specified,
   *         generate local meshes that create file:/// URLs. Useful to avoid
   *         uploading to the CDN, but in distributed systems will obviously
   *         cause failures to download meshes on other nodes.
   *  \param local_url_prefix used in conjunction with local_path. If provided,
   *         specifies the URL prefix to prepend to the filename to get the full
   *         mesh URL, allowing you to publicize the meshes by using something
   *         other than a file:// URL, e.g. if you run a web server on your
   *         host.
   *  \param n_gen_threads number of threads to use for creating aggregate meshes
   *  \param n_upload_threads number of threads to use for uploading
   *  \param skip_gen if true, skip the generation phase, but pretend it
   *         was successful. Useful for testing without spending CPU on
   *         generating meshes. Forces skip_upload to be true
   *  \param skip_upload if true, skip the upload phase, but pretend it was
   *         successful. Useful for testing, but since the mesh cache is
   *         limited, this will eventually cause later aggregates to fail to be
   *         generated
   */
  MeshAggregateManager( LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username);

  ~MeshAggregateManager();

  void addLeafObject(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const Transfer::URI& mesh);
  void removeLeafObject(const UUID& uuid);

  void addAggregate(const UUID& uuid);

  void removeAggregate(const UUID& uuid);

  void addChild(const UUID& uuid, const UUID& child_uuid) ;

  void removeChild(const UUID& uuid, const UUID& child_uuid);

  void aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren);

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

  /*void textureMetadataFinished(String texname, Mesh::MeshdataPtr md, AggregateObjectPtr aggObj,
                                          std::tr1::unordered_map<String, String> textureSet,
                                          std::tr1::shared_ptr<std::tr1::unordered_map<String, int> > downloadedTexturesMap,
                                          int retryAttempt,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response); */

  void textureChunkFinished(String texname, Transfer::Fingerprint hashprint, uint32 offset, uint32 length, Mesh::MeshdataPtr agg_mesh, AggregateObjectPtr aggObj,
                                          std::tr1::unordered_map<String, String> textureSet,
                                          std::tr1::shared_ptr<std::tr1::unordered_map<String, int> > downloadedTexturesMap,
                                          std::tr1::shared_ptr<std::tr1::unordered_map<String, std::vector<String> > > hashToURIMap,
                                          int retryAttempt,
                                          Transfer::ResourceDownloadTaskPtr dlPtr,
					  std::tr1::shared_ptr<Transfer::TransferRequest> request,
                                          std::tr1::shared_ptr<const Transfer::DenseData> response);
  void textureDownloadedForCounting(String texname, uint32 retryAttempt,
                                          std::tr1::shared_ptr<Transfer::MetadataRequest> request,
                                          std::tr1::shared_ptr<Transfer::RemoteFileMetadata> response);

  //FIXME:The following  lines are also for experimental purposes only.
  //void showCutErrorSum(uint32);
  float64 mErrorSum;
  uint32 mErrorSequenceNumber;
  int64 mSizeSum;
  int32 mCutLength;
  std::tr1::unordered_map<String, int64> mMeshSizeMap;
  bool noMoreGeneration;



};

}

#endif
