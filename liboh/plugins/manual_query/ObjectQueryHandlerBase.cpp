// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ObjectQueryHandlerBase.hpp"

#include <sirikata/core/network/IOStrandImpl.hpp>

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#define MQOQH_LOG(level,msg) SILOG(manual-query-object-query-handler,level,msg)

namespace Sirikata {
namespace OH {
namespace Manual {

ObjectQueryHandlerBase::ObjectQueryHandlerBase(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, const OHDP::SpaceNodeID& space, Network::IOStrandPtr prox_strand)
 : mContext(ctx),
   mSpaceNodeID(space),
   mParent(parent),
   mProxStrand(prox_strand)
{
}

ObjectQueryHandlerBase::~ObjectQueryHandlerBase() {
}


// MAIN Thread

void ObjectQueryHandlerBase::handleAddObjectLocSubscription(Liveness::Token alive, const ObjectReference& subscriber, const ObjectReference& observed) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    //if (!mLocService->contains(observed)) return;
    //mLocService->subscribe(subscriber, observed);
}

void ObjectQueryHandlerBase::handleRemoveObjectLocSubscription(Liveness::Token alive, const ObjectReference& subscriber, const ObjectReference& observed) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

    //mLocService->unsubscribe(subscriber, observed);
}

void ObjectQueryHandlerBase::handleRemoveAllObjectLocSubscription(Liveness::Token alive, const ObjectReference& subscriber) {
    if (!alive) return;
    Liveness::Lock lck(alive);
    if (!lck) return;

    //mLocService->unsubscribe(subscriber);
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
