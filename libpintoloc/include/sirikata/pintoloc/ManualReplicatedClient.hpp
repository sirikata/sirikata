// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPINTO_MANUAL_REPLICATED_CLIENT_HPP_
#define _SIRIKATA_LIBPINTO_MANUAL_REPLICATED_CLIENT_HPP_

#include <sirikata/pintoloc/ReplicatedLocationServiceCache.hpp>
#include <sirikata/pintoloc/OrphanLocUpdateManager.hpp>
#include <sirikata/pintoloc/TimeSynced.hpp>
#include <sirikata/core/prox/Defs.hpp>
#include <sirikata/core/service/Service.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <boost/functional/hash.hpp>

#include <sirikata/core/util/SerializationCheck.hpp>

namespace Sirikata {
namespace Pinto {
namespace Manual {

/** ReplicatedClient manage interaction with a server that provides a
 *  replicated query data structure. It manages processing LocUpdates,
 *  handling orphans and putting the data into a LocationServiceCache,
 *  and figures out what commands to send to the server to maintain
 *  the cut (based on inputs about which nodes are being used). It can
 *  support multiple indices being replicated by the server,
 *  e.g. during rebuilding or when static/dynamic trees are split.
 */
class SIRIKATA_LIBPINTOLOC_EXPORT ReplicatedClient :
        public Service, OrphanLocUpdateManager::Listener, SerializationCheck
{
  public:
    /** Create a ReplicatedClient.
     *  \param sync TimeSynced to convert times. This class takes ownership and
     *               will delete it.
     *  \param server_id string identifier used to uniquely identify
     *               the server this data is replicated from. Used in
     *               reporting to allow you to distinguish between
     *               different replicated clients.
     */
    ReplicatedClient(Context* ctx, Network::IOStrandPtr strand, TimeSynced* sync, const String& server_id);
    ReplicatedClient(Context* ctx, Network::IOStrand* strand, TimeSynced* sync, const String& server_id);
    virtual ~ReplicatedClient();

    // Service Interface
    virtual void start();
    virtual void stop();

    // EXTERNAL EVENTS -- These should be injected by the user of the
    // class

    // Init or destroy a query on the server. These will send format and request
    // messages are sent. Some users may not need to do this -- in some cases
    // these events are implicit based on connections.
    void initQuery();
    void destroyQuery();

    void proxUpdate(const Sirikata::Protocol::Prox::ProximityResults& results);
    void proxUpdate(const Sirikata::Protocol::Prox::ProximityUpdate& update);
    void locUpdate(const Sirikata::Protocol::Loc::LocationUpdate& update);

    // Notifications about local queries in the tree so we know how to
    // move the cut on the space server up or down.
    void queriersAreObserving(ProxIndexID indexid, const ObjectReference& objid);
    void queriersStoppedObserving(ProxIndexID indexid, const ObjectReference& objid);
    void replicatedNodeRemoved(ProxIndexID indexid, const ObjectReference& objid);

    // OrphanLocUpdateManager::Listener Interface (public because
    // OrphanLocUpdateManager requires it)
    virtual void onOrphanLocUpdate(const LocUpdate& lu, ProxIndexID iid);

  protected:
    /** Invoked when a new index (new LocationServiceCache) is created. */
    virtual void onCreatedReplicatedIndex(ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects) = 0;
    /** Invoked when a an index (LocationServiceCache) is destroyed because it
     *  has no objects left.
     */
    virtual void onDestroyedReplicatedIndex(ProxIndexID proxid) = 0;
    /** Invoked when this class needs to send a message to control the
     *  traversal of the tree on the remote server. This is the only "output" of
     *  this class besides the data placed in the
     *  ReplicatedLocationServiceCache.
     */
    virtual void sendProxMessage(const String& payload) = 0;

  private:

    // Handlers in proper strand
    void handleProxUpdateResults(const Sirikata::Protocol::Prox::ProximityResults& results);
    void handleProxUpdate(const Sirikata::Protocol::Prox::ProximityUpdate& update);
    void handleLocUpdate(const Sirikata::Protocol::Loc::LocationUpdate& update);

    // Returns true if the cache was actually created
    bool createLocCache(ProxIndexID iid);
    // Creates just the orhpan loc update manager, used when getting
    // unknown loc updates but we don't want the loc cache yet
    void createOrphanLocUpdateManager(ProxIndexID iid);
    ReplicatedLocationServiceCachePtr getLocCache(ProxIndexID iid);
    OrphanLocUpdateManagerPtr getOrphanLocUpdateManager(ProxIndexID iid);
    void removeLocCache(ProxIndexID iid);

    // Unique ID for objects replicated by this client. Objects might
    // appear in multiple indices, so we need to combine index and
    // object IDs
    struct IndexObjectReference {
        IndexObjectReference() : indexid(-1), objid() {}
        IndexObjectReference(ProxIndexID idx, const ObjectReference& obj)
         : indexid(idx), objid(obj)
        {}

        bool operator<(const IndexObjectReference& rhs) const {
            return (indexid < rhs.indexid ||
                (indexid == rhs.indexid && objid < rhs.objid));
        }

        bool operator==(const IndexObjectReference& rhs) const {
            return (indexid == rhs.indexid && objid == rhs.objid);
        }
        class Hasher {
        public:
            size_t operator()(const IndexObjectReference& ior) const {
                size_t seed = 0;
                boost::hash_combine(seed, ior.indexid);
                boost::hash_combine(seed, ior.objid.hash());
                return seed;
            }
        };

        ProxIndexID indexid;
        ObjectReference objid;
    };
    // Track nodes which are no longer observed by any queriers,
    // making them candidates for coarsening.
    struct UnobservedNodeTimeout {
        UnobservedNodeTimeout(const IndexObjectReference& id, Time _expires)
         : objid(id),
           expires(_expires)
        {}
        IndexObjectReference objid;
        Time expires;
    };
    struct objid_tag {};
    struct expires_tag {};
    typedef boost::multi_index_container<
        UnobservedNodeTimeout,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique< boost::multi_index::tag<objid_tag>, BOOST_MULTI_INDEX_MEMBER(UnobservedNodeTimeout,IndexObjectReference,objid) >,
            boost::multi_index::ordered_non_unique< boost::multi_index::tag<expires_tag>, BOOST_MULTI_INDEX_MEMBER(UnobservedNodeTimeout,Time,expires) >
            >
        > UnobservedNodeTimeouts;
    typedef UnobservedNodeTimeouts::index<objid_tag>::type UnobservedNodesByID;
    typedef UnobservedNodeTimeouts::index<expires_tag>::type UnobservedNodesByExpiration;


    Context* mContext;
    // This is unfortunate, but we need to be able to accept both raw
    // IOStrand*'s and IOStrandPtrs, so we store both, and sometimes the
    // IOStrandPtr is just empty (we always use the sanely-named raw ptr).
    Network::IOStrandPtr doNotUse___mStrand;
    Network::IOStrand* mStrand;
    TimeSynced* mSync;
    const String mServerID;

    typedef std::map<ProxIndexID, ReplicatedLocationServiceCachePtr> IndexObjectCacheMap;
    IndexObjectCacheMap mObjects;
    typedef std::map<ProxIndexID, OrphanLocUpdateManagerPtr> IndexOrphanLocUpdateMap;
    IndexOrphanLocUpdateMap mOrphans;

    typedef std::tr1::unordered_set<IndexObjectReference, IndexObjectReference::Hasher> ObservedNodeSet;
    ObservedNodeSet mObservedNodes;

    UnobservedNodeTimeouts mUnobservedTimeouts;
    Network::IOTimerPtr mUnobservedTimer;


    // These track which loccaches + orphan managers we've created to handle
    // orphans but never got a prox update before the orphans became
    // outdated. We use a relatively inefficient implementation because we can
    // cleanup rarely and there won't be that many entries in the list ever.

    std::vector<ProxIndexID> mCachesForOrphans;
    Poller mCleanupOrphansPoller;




    // Proximity
    // Helpers for sending different types of basic requests
    void sendRefineRequest(const ProxIndexID proxid, const ObjectReference& agg);
    void sendRefineRequest(const ProxIndexID proxid, const std::vector<ObjectReference>& aggs);
    void sendCoarsenRequest(const ProxIndexID proxid, const ObjectReference& agg);
    void sendCoarsenRequest(const ProxIndexID proxid, const std::vector<ObjectReference>& aggs);

    // Cleanup data associated with orphans that never got a prox message
    void cleanupOrphans();

    // Cut management
    void processExpiredNodes();
};

/** A convenient version of Replicated client which attaches an ID of
 *  user-defined type to the ReplicatedClient and dispatches events to a
 *  listener that includes a pointer to the ReplicatedClientWithID so the
 *  identifying information can be extracted.
 */
template<typename IDType>
class ReplicatedClientWithID : public ReplicatedClient {
public:
    class Parent {
    public:
        virtual ~Parent() {}

        virtual void onCreatedReplicatedIndex(ReplicatedClientWithID* client, const IDType& id, ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects) = 0;
        virtual void onDestroyedReplicatedIndex(ReplicatedClientWithID* client, const IDType& id, ProxIndexID proxid) = 0;
        // Invoked when this ReplicateClient needs to send a message to the
        // server it's replicating the tree from.
        virtual void sendReplicatedClientProxMessage(ReplicatedClientWithID* client, const IDType& id, const String& msg) = 0;
    };

    ReplicatedClientWithID(Context* ctx, Network::IOStrandPtr strand, Parent* parent, TimeSynced* sync, const String& server_id, const IDType& id_)
     : ReplicatedClient(ctx, strand, sync, server_id),
       mParent(parent),
       mID(id_)
    {}
    ReplicatedClientWithID(Context* ctx, Network::IOStrand* strand, Parent* parent, TimeSynced* sync, const String& server_id, const IDType& id_)
     : ReplicatedClient(ctx, strand, sync, server_id),
       mParent(parent),
       mID(id_)
    {}

    virtual ~ReplicatedClientWithID() {}

    const IDType& id() const { return mID; }

protected:
    virtual void onCreatedReplicatedIndex(ProxIndexID proxid, ReplicatedLocationServiceCachePtr loccache, ServerID sid, bool dynamic_objects) {
        mParent->onCreatedReplicatedIndex(this, mID, proxid, loccache, sid, dynamic_objects);
    }
    virtual void onDestroyedReplicatedIndex(ProxIndexID proxid) {
        mParent->onDestroyedReplicatedIndex(this, mID, proxid);
    }

    virtual void sendProxMessage(const String& payload) {
        mParent->sendReplicatedClientProxMessage(this, mID, payload);
    }

    Parent* mParent;
    IDType mID;
};

} // namespace Manual
} // namespace Pinto
} // namespace Sirikata

#endif //#ifndef _SIRIKATA_LIBPINTO_MANUAL_REPLICATED_CLIENT_HPP_
