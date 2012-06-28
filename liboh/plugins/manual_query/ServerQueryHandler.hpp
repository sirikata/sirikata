// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OH_MQ_SERVER_QUERY_HANDLER_HPP_
#define _SIRIKATA_OH_MQ_SERVER_QUERY_HANDLER_HPP_

#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/oh/SpaceNodeSession.hpp>
#include <sirikata/pintoloc/OrphanLocUpdateManager.hpp>
#include <sirikata/pintoloc/ReplicatedLocationServiceCache.hpp>
#include <sirikata/core/prox/Defs.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace Sirikata {
namespace OH {
namespace Manual {

class ManualObjectQueryProcessor;

/** This class manages queries registered with servers. It accepts aggregated
 *  requests from the parent ManualObjectQueryProcessor, registers them with the
 *  server, and manages receiving results and forwarding them for further
 *  processing. It takes care of other details like orphan location updates so
 *  the parent just gets a stream of updates.
 */
class ServerQueryHandler :
        public SpaceNodeSessionListener,
        OrphanLocUpdateManager::Listener<OHDP::SpaceNodeID>
{
public:
    ServerQueryHandler(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, Network::IOStrandPtr strand);

    // Service Interface
    virtual void start();
    virtual void stop();


    // Helper that marks a server with another connected object and may register
    // a query
    void incrementServerQuery(OHDP::SpaceNodeID node);
    // Helper that tries to remove the server query pointed at by the
    // iterator, checking if it is referenced at all yet and sending a
    // message to kill the query
    void decrementServerQuery(OHDP::SpaceNodeID node);


    // ObjectQueryHandler callbacks - handle notifications about local queries
    // in the tree so we know how to move the cut on the space server up or down.
    void queriersAreObserving(const OHDP::SpaceNodeID& snid, const ObjectReference& objid);
    void queriersStoppedObserving(const OHDP::SpaceNodeID& snid, const ObjectReference& objid);


    // OrphanLocUpdateManager::Listener Interface
    virtual void onOrphanLocUpdate(const OHDP::SpaceNodeID& observer, const LocUpdate& lu, ProxIndexID iid);

private:
    ObjectHostContext* mContext;
    ManualObjectQueryProcessor* mParent;
    Network::IOStrandPtr mStrand;


    // Track nodes which are no longer observed by any queriers,
    // making them candidates for coarsening.
    struct UnobservedNodeTimeout {
        UnobservedNodeTimeout(const ObjectReference& id, Time _expires)
         : objid(id),
           expires(_expires)
        {}
        ObjectReference objid;
        Time expires;
    };
    struct objid_tag {};
    struct expires_tag {};
    typedef boost::multi_index_container<
        UnobservedNodeTimeout,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique< boost::multi_index::tag<objid_tag>, BOOST_MULTI_INDEX_MEMBER(UnobservedNodeTimeout,ObjectReference,objid) >,
            boost::multi_index::ordered_non_unique< boost::multi_index::tag<expires_tag>, BOOST_MULTI_INDEX_MEMBER(UnobservedNodeTimeout,Time,expires) >
            >
        > UnobservedNodeTimeouts;
    typedef UnobservedNodeTimeouts::index<objid_tag>::type UnobservedNodesByID;
    typedef UnobservedNodeTimeouts::index<expires_tag>::type UnobservedNodesByExpiration;

    // Queries we've registered with servers so that we can resolve
    // object queries
    struct ServerQueryState {
        ServerQueryState(Context* ctx_, Network::IOStrandPtr strand_, OHDPSST::Stream::Ptr base, Network::IOTimerPtr unobs_timer)
         : ctx(ctx_),
           strand(strand_),
           nconnected(0),
           base_stream(base),
           prox_stream(),
           prox_stream_requested(false),
           outstanding(),
           writing(false),
           unobservedTimer(unobs_timer)
        {
        }
        ~ServerQueryState() {
            for(IndexOrphanLocUpdateMap::iterator it = orphans.begin(); it != orphans.end(); it++)
                it->second->stop();
            unobservedTimer->cancel();
        }

        // Returns true if the *query* can be removed (not if this
        // object can be removed, that is tracked by the lifetime of
        // the session)
        bool canRemove() const {
            return nconnected == 0;
        }

        void createLocCache(ProxIndexID iid) {
            if (objects.find(iid) == objects.end())
                objects[iid] = ReplicatedLocationServiceCachePtr(new ReplicatedLocationServiceCache(strand));
            if (orphans.find(iid) == orphans.end())
                orphans[iid] = OrphanLocUpdateManagerPtr(new OrphanLocUpdateManager(ctx, ctx->mainStrand, Duration::seconds(10)));
        }

        ReplicatedLocationServiceCachePtr getLocCache(ProxIndexID iid) {
            assert(objects.find(iid) != objects.end());
            return objects[iid];
        }
        OrphanLocUpdateManagerPtr getOrphanLocUpdateManager(ProxIndexID iid) {
            assert(orphans.find(iid) != orphans.end());
            return orphans[iid];
        }

        void removeLocCache(ProxIndexID iid) {
            assert(objects.find(iid) != objects.end());
            objects.erase(iid);
            assert(orphans.find(iid) != orphans.end());
            orphans.erase(iid);
        }

        // Strand for ReplicatedLocationServiceCaches
        Context* ctx;
        Network::IOStrandPtr strand;
        // # of connected objects
        int32 nconnected;
        OHDPSST::Stream::Ptr base_stream;
        OHDPSST::Stream::Ptr prox_stream;
        // Whether we've requested the prox_stream
        bool prox_stream_requested;
        // Outstanding data to be sent
        std::queue<String> outstanding;
        // Whether we're in the process of sending messages
        bool writing;

        typedef std::map<ProxIndexID, ReplicatedLocationServiceCachePtr> IndexObjectCacheMap;
        IndexObjectCacheMap objects;
        typedef std::map<ProxIndexID, OrphanLocUpdateManagerPtr> IndexOrphanLocUpdateMap;
        IndexOrphanLocUpdateMap orphans;

        UnobservedNodeTimeouts unobservedTimeouts;
        Network::IOTimerPtr unobservedTimer;
    };
    typedef std::tr1::shared_ptr<ServerQueryState> ServerQueryStatePtr;
    typedef std::tr1::unordered_map<OHDP::SpaceNodeID, ServerQueryStatePtr, OHDP::SpaceNodeID::Hasher> ServerQueryMap;
    ServerQueryMap mServerQueries;

    // These track which loccaches + orphan managers we've created to handle
    // orphans but never got a prox update before the orphans became
    // outdated. We use a relatively inefficient implementation because we can
    // cleanup rarely and there won't be that many entries in the list ever.
    typedef std::pair<OHDP::SpaceNodeID, ProxIndexID> SpaceNodeProxIndex;
    std::vector<SpaceNodeProxIndex> mCachesForOrphans;
    Poller mCleanupOrphansPoller;


    // SpaceNodeSessionListener Interface
    virtual void onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream);
    virtual void onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id);

    // Helper that updates a server query, possibly sending an
    // initialization message to the server to setup the query
    void updateServerQuery(ServerQueryMap::iterator serv_it, bool is_new);



    // Proximity
    // Helpers for sending different types of basic requests
    void sendInitRequest(ServerQueryMap::iterator serv_it);
    void sendRefineRequest(ServerQueryMap::iterator serv_it, const ObjectReference& agg);
    void sendRefineRequest(ServerQueryMap::iterator serv_it, const std::vector<ObjectReference>& aggs);
    void sendCoarsenRequest(ServerQueryMap::iterator serv_it, const ObjectReference& agg);
    void sendCoarsenRequest(ServerQueryMap::iterator serv_it, const std::vector<ObjectReference>& aggs);
    void sendDestroyRequest(ServerQueryMap::iterator serv_it);

    // Send a message to prox, triggering new stream as necessary
    void sendProxMessage(ServerQueryMap::iterator serv_it, const String& msg);
    // Utility that triggers writing some more prox data. As long as more is
    // available, it'll keep looping.
    void writeSomeProxData(ServerQueryStatePtr data);
    // Callback from creating proximity substream
    void handleCreatedProxSubstream(const OHDP::SpaceNodeID& snid, int success, OHDPSST::Stream::Ptr prox_stream);
    // Data read callback for prox substreams -- translate to proximity events
    void handleProximitySubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr prox_stream, String* prevdata, uint8* buffer, int length);
    // Handle decode proximity message
    void handleProximityMessage(const OHDP::SpaceNodeID& snid, const String& payload);

    // Location
    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(const OHDP::SpaceNodeID& snid, int err, OHDPSST::Stream::Ptr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(const OHDP::SpaceNodeID& snid, OHDPSST::Stream::Ptr s, std::stringstream* prevdata, uint8* buffer, int length);
    bool handleLocationMessage(const OHDP::SpaceNodeID& snid, const std::string& payload);
    // Cleanup data associated with orphans that never got a prox message
    void cleanupOrphans();

    // Cut management
    void processExpiredNodes(const OHDP::SpaceNodeID& snid);
};

} // namespace Manual
} // namespace OH
} // namespace Sirikata

#endif //_SIRIKATA_OH_MQ_SERVER_QUERY_HANDLER_HPP_
