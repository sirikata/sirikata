// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/util/UniqueID.hpp>

namespace Sirikata {

AtomicValue<uint16> UniqueID16::sSource(0);
AtomicValue<uint32> UniqueID32::sSource(0);

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
boost::mutex UniqueID64::mMutex;
uint64 UniqueID64::sSource = 0;
#else
AtomicValue<uint64> UniqueID64::sSource(0);
#endif

} // namespace Sirikata
