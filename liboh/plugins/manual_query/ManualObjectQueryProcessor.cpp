// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualObjectQueryProcessor.hpp"
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"

#define QPLOG(lvl, msg) SILOG(manual-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Manual {

ManualObjectQueryProcessor* ManualObjectQueryProcessor::create(ObjectHostContext* ctx, const String& args) {
    return new ManualObjectQueryProcessor(ctx);
}


ManualObjectQueryProcessor::ManualObjectQueryProcessor(ObjectHostContext* ctx)
 : mContext(ctx)
{
}

ManualObjectQueryProcessor::~ManualObjectQueryProcessor() {
}

void ManualObjectQueryProcessor::start() {
    mContext->objectHost->SpaceNodeSessionManager::addListener(static_cast<SpaceNodeSessionListener*>(this));
    mContext->objectHost->ObjectNodeSessionProvider::addListener(static_cast<ObjectNodeSessionListener*>(this));
}

void ManualObjectQueryProcessor::stop() {
    mContext->objectHost->ObjectNodeSessionProvider::removeListener(static_cast<ObjectNodeSessionListener*>(this));
    mContext->objectHost->SpaceNodeSessionManager::removeListener(static_cast<SpaceNodeSessionListener*>(this));
}

void ManualObjectQueryProcessor::onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) {
    QPLOG(detailed, "New space node session " << id);
}

void ManualObjectQueryProcessor::onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) {
}

void ManualObjectQueryProcessor::onObjectNodeSession(const SpaceID& space, const ObjectReference& sporef, const OHDP::NodeID& id) {
    QPLOG(detailed, "New object-space node session: " << sporef << " connected to " << space << "-" << id);
}

void ManualObjectQueryProcessor::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) {
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
