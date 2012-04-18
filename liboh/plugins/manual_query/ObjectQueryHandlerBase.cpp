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

ObjectQueryHandlerBase::ObjectQueryHandlerBase(ObjectHostContext* ctx, ManualObjectQueryProcessor* parent, const OHDP::SpaceNodeID& space, Network::IOStrandPtr prox_strand, OHLocationServiceCachePtr loc_cache)
 : mContext(ctx),
   mParent(parent),
   mSpaceNodeID(space),
   mProxStrand(prox_strand),
   mLocCache(loc_cache)
{
    // Deal with static/dynamic split
    mSeparateDynamicObjects = GetOptionValue<bool>(OPT_MANUAL_QUERY_SPLIT_DYNAMIC);
    mNumQueryHandlers = (mSeparateDynamicObjects ? 2 : 1);
}

ObjectQueryHandlerBase::~ObjectQueryHandlerBase() {
}


const String& ObjectQueryHandlerBase::ObjectClassToString(ObjectClass c) {
    static String static_ = "static";
    static String dynamic_ = "dynamic";
    static String unknown_ = "unknown";

    switch(c) {
      case OBJECT_CLASS_STATIC: return static_; break;
      case OBJECT_CLASS_DYNAMIC: return dynamic_; break;
      default: return unknown_; break;
    }
}

bool ObjectQueryHandlerBase::velocityIsStatic(const Vector3f& vel) {
    // These values are arbitrary, just meant to indicate that the object is,
    // for practical purposes, not moving.
    return (vel.lengthSquared() < 0.01f* 0.01f);
}


// MAIN Thread

void ObjectQueryHandlerBase::handleAddObjectLocSubscription(const ObjectReference& subscriber, const ObjectReference& observed) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    //if (!mLocService->contains(observed)) return;
    //mLocService->subscribe(subscriber, observed);
}

void ObjectQueryHandlerBase::handleRemoveObjectLocSubscription(const ObjectReference& subscriber, const ObjectReference& observed) {
    //mLocService->unsubscribe(subscriber, observed);
}

void ObjectQueryHandlerBase::handleRemoveAllObjectLocSubscription(const ObjectReference& subscriber) {
    //mLocService->unsubscribe(subscriber);
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
