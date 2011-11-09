// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualObjectQueryProcessor.hpp"
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"

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
}

void ManualObjectQueryProcessor::stop() {
}

void ManualObjectQueryProcessor::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) {
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
