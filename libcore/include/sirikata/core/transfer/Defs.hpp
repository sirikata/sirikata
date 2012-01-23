// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_TRANSFER_DEFS_HPP_
#define _SIRIKATA_CORE_TRANSFER_DEFS_HPP_

#include <sirikata/core/util/Sha256.hpp>

namespace Sirikata {
namespace Transfer {

class ChunkRequest;
class TransferPool;
typedef std::tr1::shared_ptr<TransferPool> TransferPoolPtr;
typedef std::tr1::shared_ptr<ChunkRequest> ChunkRequestPtr;

/// simple file ID class--should make no assumptions about which hash.
typedef SHA256 Fingerprint;

typedef float32 Priority;

} // namespace Transfer
} // namespace Sirikata

#endif //_SIRIKATA_CORE_TRANSFER_DEFS_HPP_
