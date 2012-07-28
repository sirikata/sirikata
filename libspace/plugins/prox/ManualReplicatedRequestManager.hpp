// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBPROX_MANUAL_REPLICATED_REQUEST_MANAGER_HPP_
#define _SIRIKATA_LIBPROX_MANUAL_REPLICATED_REQUEST_MANAGER_HPP_

#include <sirikata/pintoloc/ProxSimulationTraits.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/service/Context.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <prox/manual/Query.hpp>

namespace Sirikata {

/** ManualReplicatedRequestManager tracks requests to adjust cuts and tries to
 *  make sure they get applied. Essentially it masks failure to apply updates
 *  when an update fails not because something isn't along the cut and can be
 *  refined/coarsened, but rather because adjusting it requires grabbing
 *  additional data from another server and that data isn't available yet. You
 *  input the requests when they fail and events for when the children of a node
 *  becomes available. This class deals with generating notifications when a
 *  match between a request and available data is made, or expiring the request
 *  after a short period of time.
 *
 *  Of course, this really only makes sense for refinement since coarsening
 *  should *always* be successful. Additional commands might eventually need to
 *  be added.
 */
class ManualReplicatedRequestManager :
        public Service
{
  public:
    typedef Prox::ManualQuery<ObjectProxSimulationTraits> ProxQuery;

    ManualReplicatedRequestManager(Context* ctx, Network::IOStrandPtr strand);
    ManualReplicatedRequestManager(Context* ctx, Network::IOStrand* strand);
    virtual ~ManualReplicatedRequestManager();

    // Service Interface
    virtual void start();
    virtual void stop();

    // Add a failed refinement request.
    void addFailedRefine(ProxQuery* query, const ObjectReference& objid);
    // Lookup any queries waiting on the object to be refined. Note that when
    // you call this, you should be asking for the queries waiting on the
    // *parent* of node/object that you just added since that's the node for
    // which data is now available.
    void lookupWaitingQueries(const ObjectReference& objid, std::vector<ProxQuery*>* queries_out);
    // Clear out any requests by the given querier
    void removeQuerier(ProxQuery* query);

  private:

    // Track each request for refinement
    struct Request {
        Request(ProxQuery* _query, const ObjectReference& _objid, Time _expires)
         : query(_query),
           objid(_objid),
           expires(_expires)
        {}
        ProxQuery* query;
        ObjectReference objid;
        Time expires;
    };
    struct query_tag {};
    struct objid_tag {};
    struct expires_tag {};
    typedef boost::multi_index_container<
        Request,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_non_unique< boost::multi_index::tag<query_tag>, BOOST_MULTI_INDEX_MEMBER(Request,ProxQuery*,query) >,
            boost::multi_index::hashed_non_unique< boost::multi_index::tag<objid_tag>, BOOST_MULTI_INDEX_MEMBER(Request,ObjectReference,objid), ObjectReference::Hasher >,
            boost::multi_index::ordered_non_unique< boost::multi_index::tag<expires_tag>, BOOST_MULTI_INDEX_MEMBER(Request,Time,expires) >
            >
        > RequestIndex;
    typedef RequestIndex::index<query_tag>::type RequestsByQuery;
    typedef RequestIndex::index<objid_tag>::type RequestsByObject;
    typedef RequestIndex::index<expires_tag>::type RequestsByExpiration;


    Context* mContext;
    // This is unfortunate, but we need to be able to accept both raw
    // IOStrand*'s and IOStrandPtrs, so we store both, and sometimes the
    // IOStrandPtr is just empty (we always use the sanely-named raw ptr).
    Network::IOStrandPtr doNotUse___mStrand;
    Network::IOStrand* mStrand;

    RequestIndex mRequestIndex;
    Network::IOTimerPtr mExpiryTimer;



    void processExpiredRequests();
};

} // namespace Sirikata

#endif //_SIRIKATA_LIBPROX_MANUAL_REPLICATED_REQUEST_MANAGER_HPP_
