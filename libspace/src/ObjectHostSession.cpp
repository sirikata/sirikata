// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/ObjectHostSession.hpp>
#include <sirikata/core/ohdp/SST.hpp>

namespace Sirikata {

// Just this so we don't include SSTImpl.hpp
ObjectHostSession::~ObjectHostSession() {
    if (mSSTStream) mSSTStream->close(true);
}

} // namespace Sirikata
