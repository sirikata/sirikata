// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualReplicatedRequestManager.hpp"

namespace Sirikata {

ManualReplicatedRequestManager::ManualReplicatedRequestManager(Context* ctx, Network::IOStrandPtr strand)
 : mContext(ctx),
   doNotUse___mStrand(strand),
   mStrand(strand.get()),
   mRequestIndex(),
   mExpiryTimer(
       Network::IOTimer::create(
           strand.get(),
           std::tr1::bind(&ManualReplicatedRequestManager::processExpiredRequests, this)
       )
   )
{
}

ManualReplicatedRequestManager::ManualReplicatedRequestManager(Context* ctx, Network::IOStrand* strand)
 : mContext(ctx),
   doNotUse___mStrand(),
   mStrand(strand),
   mRequestIndex(),
   mExpiryTimer(
       Network::IOTimer::create(
           strand,
           std::tr1::bind(&ManualReplicatedRequestManager::processExpiredRequests, this)
       )
   )
{
}

ManualReplicatedRequestManager::~ManualReplicatedRequestManager() {
}

void ManualReplicatedRequestManager::start() {

}

void ManualReplicatedRequestManager::stop() {
    mExpiryTimer->cancel();
}




void ManualReplicatedRequestManager::addFailedRefine(ProxQuery* query, const ObjectReference& objid) {
    mRequestIndex.insert(Request(query, objid, mContext->recentSimTime() + Duration::seconds(5)));
    // And restart the timeout for the most recent
    Duration next_timeout = mRequestIndex.get<expires_tag>().begin()->expires - mContext->recentSimTime();
    mExpiryTimer->cancel();
    mExpiryTimer->wait(next_timeout);
}

void ManualReplicatedRequestManager::lookupWaitingQueries(const ObjectReference& objid, std::vector<ProxQuery*>* queries_out) {
    RequestsByObject& by_object = mRequestIndex.get<objid_tag>();

    std::pair<RequestsByObject::iterator, RequestsByObject::iterator> obj_range = by_object.equal_range(objid);
    for(RequestsByObject::iterator it = obj_range.first; it != obj_range.second; it++)
        queries_out->push_back(it->query);
    by_object.erase(obj_range.first, obj_range.second);
}

void ManualReplicatedRequestManager::removeQuerier(ProxQuery* query) {
    RequestsByQuery& by_query = mRequestIndex.get<query_tag>();
    by_query.erase(query);
}



void ManualReplicatedRequestManager::processExpiredRequests() {
    // This just clears out expired entries since there's nothing we can do
    // unless the user asks for the requests so they can fulfill them
    Time curt = mContext->recentSimTime();
    RequestsByExpiration& by_expires = mRequestIndex.get<expires_tag>();
    while(!by_expires.empty() &&
        by_expires.begin()->expires < curt)
    {
        by_expires.erase(by_expires.begin());
    }

    if (!by_expires.empty()) {
        Duration next_timeout = mRequestIndex.get<expires_tag>().begin()->expires - mContext->recentSimTime();
        mExpiryTimer->wait(next_timeout);
    }
}


} // namespace Sirikata
