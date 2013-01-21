// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_SPACE_TWITTER_AGGREGATE_MANAGER_HPP_
#define _SIRIKATA_SPACE_TWITTER_AGGREGATE_MANAGER_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>

#include <sirikata/core/command/Command.hpp>
#include <sirikata/core/transfer/ResourceDownloadTask.hpp>
#include <sirikata/core/transfer/HttpManager.hpp>
#include <sirikata/core/transfer/URL.hpp>

#include <sirikata/core/util/LRUCache.hpp>
#include <json_spirit/json_spirit.h>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT TwitterAggregateManager : public AggregateManager {
  public:

    /** Create an AggregateManager. */
    TwitterAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username);
    ~TwitterAggregateManager();

    virtual void addAggregate(const UUID& uuid);
    virtual void removeAggregate(const UUID& uuid);
    virtual void addChild(const UUID& uuid, const UUID& child_uuid) ;
    virtual void removeChild(const UUID& uuid, const UUID& child_uuid);
    virtual void aggregateObserved(const UUID& objid, uint32 nobservers, uint32 nchildren);
    virtual void generateAggregateMesh(const UUID& uuid, const Duration& delayFor = Duration::milliseconds(1.0f) );

  private:

    LocationService* mLoc;
    Transfer::OAuthParamsPtr mOAuth;

    // For access to all aggregate data structures
    boost::recursive_mutex mAggregateMutex;
    typedef boost::unique_lock<boost::recursive_mutex> Lock;

    // Naming is a bit misleading -- tracks both aggregates and individual
    // objects so the objects can be referenced and their state tracked.
    struct AggregateObject;
    typedef std::tr1::shared_ptr<AggregateObject> AggregateObjectPtr;
    typedef std::set<AggregateObjectPtr> AggregateObjectSet;
    typedef std::multiset<AggregateObjectPtr> AggregateObjectMultiSet;
    struct AggregateObject {
        AggregateObject(const UUID& uuid_, bool is_agg)
         : uuid(uuid_),
           parent(),
           parents(),
           children(),
           aggregate(is_agg),
           loc(),
           mesh(),
           dirty(false),
           dirty_children(),
           processing(false)
        {}

        // Tree structure
        const UUID uuid;
        AggregateObjectPtr parent;
        // Leaf objects can have multiple parents (e.g. because of separate
        // server query and object query trees in the older query processor and
        // rebuilding trees in both implementations). Leaf objects also have
        // other constraints (e.g. no aggregate generation). True aggregates
        // will only use parent, leaves will only use parents.
        AggregateObjectSet parents;
        AggregateObjectSet children;

        // Basic aggregate world state, cached from LocationService
        bool aggregate;
        TimedMotionVector3f loc;
        String mesh;

        // Aggregation state
        bool dirty;
        // Dirty children we're waiting for. This is a multiset because the set
        // of dirty children has to be maintained while processing an aggregate
        // (lest we prematurely add the parent to the ready list for
        // processing), but the set of dirty children can change in the
        // meantime, including marking the currently processing object dirty
        // again and marking parents as dirty. When we need to remove something,
        // we remove only one copy of it to keep the count accurate.
        AggregateObjectMultiSet dirty_children;
        bool processing; // Actively being processed by aggregation thread
    };
    // All aggregates
    typedef std::tr1::unordered_map<UUID, AggregateObjectPtr, UUID::Hasher> AggregateObjectMap;
    AggregateObjectMap mAggregates;
    // Count of dirty aggregates (ready and not ready)
    uint32 mDirtyAggregateCount;
    // Aggregates ready to start the generation process, i.e. have no children
    // blocking them.
    typedef std::deque<AggregateObjectPtr> AggregateQueue;
    AggregateQueue mReadyAggregates;


    Transfer::TransferPoolPtr mTransferPool;


    enum { MAX_NUM_AGGREGATION_THREADS = 16 };
    uint16 mNumAggregationThreads;
    Thread* mAggregationThreads[MAX_NUM_AGGREGATION_THREADS];
    Network::IOService* mAggregationService;
    Network::IOWork* mAggregationWork;

    typedef std::map<Transfer::URL, Transfer::DenseDataPtr> DataMap;
    typedef std::tr1::shared_ptr<json_spirit::Value> JSONValuePtr;
    typedef LRUCache<Transfer::URL, JSONValuePtr, Transfer::URL::Hasher> AggregateDataCache;
    AggregateDataCache mAggregateDataCache;
    boost::recursive_mutex mAggregateDataCacheMutex;

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

    // Command handlers
    void commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid);


    // Returns true if an aggregate with the given ID currently exists.
    bool aggregateExists(const UUID& uuid) const;
    // Gets an aggregate object, or NULL if it doesn't exist
    AggregateObjectPtr getAggregate(const UUID& uuid) const;
    // Update the parent reference and membership in any queues of the parent if
    // it's readiness is affected.
    void setParent(AggregateObjectPtr child, AggregateObjectPtr parent);
    // Ensure the given aggregate is not in the ready list
    void ensureNotInReadyList(AggregateObjectPtr agg);
    // Ensure the given aggregate has been added to the ready list
    void ensureInReadyList(AggregateObjectPtr agg);
    // Mark the given aggregate and all aggregates to the root as dirty. The
    // second parameters indicates if the number of dirty children of the first
    // aggregate should be increased, e.g. due to the addition of a dirty child,
    // or not, e.g. due to removal of a child
    void markDirtyToRoot(AggregateObjectPtr agg, AggregateObjectPtr new_dirty_child);



    // Aggregation threads
    void aggregationThreadMain();
    // Grab ready aggregate from ready list and process it
    void processAggregate();

    // Back in the main thread
    void updateAggregateLocMesh(UUID uuid, String mesh);

    // Other threads
    void finishedDownload(
        Transfer::URL data_url,
        Transfer::ResourceDownloadTaskPtr taskptr, Transfer::TransferRequestPtr request,
        Transfer::DenseDataPtr response, boost::mutex* mutex,
        uint32* children_left, DataMap* children_data,
        boost::condition_variable* all_children_ready);
    void uploadFinished(
        Transfer::HttpManager::HttpResponsePtr response,
        Transfer::HttpManager::ERR_TYPE error,
        const boost::system::error_code& boost_error,
        boost::mutex* mutex, boost::condition_variable* upload_finished
    );
};

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_TWITTER_AGGREGATE_MANAGER_HPP_
