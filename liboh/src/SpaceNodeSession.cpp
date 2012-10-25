// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/SpaceNodeSession.hpp>
#include <sirikata/core/ohdp/SST.hpp>

namespace Sirikata {

void SpaceNodeSessionManager::fireSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) {
    notify(&SpaceNodeSessionListener::onSpaceNodeSessionEnded, id);
    SpaceNodeSessionMap::const_iterator it = mSNSessions.find(id);
    if (it != mSNSessions.end()) {
        // Force closure of the stream
        if (it->second) it->second->close(true);
        mSNSessions.erase(it);
    }
}


} // namespace Sirikata
