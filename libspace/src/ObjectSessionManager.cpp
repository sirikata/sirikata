// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/ObjectSessionManager.hpp>
#include <sirikata/core/odp/SST.hpp>

namespace Sirikata {

ObjectSession::~ObjectSession() {
    // Force closure, there's no way to get data to the object anymore...
    if (mSSTStream) mSSTStream->close(true);
}

} // namespace Sirikata
