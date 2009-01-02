/*     Iridium Transfer -- Content Transfer management system
 *  CacheLayer.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Iridium nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*  Created on: Dec 31, 2008 */

#ifndef IRIDIUM_CacheLayer_HPP__
#define IRIDIUM_CacheLayer_HPP__

#include <vector>

#include "TransferLayer.hpp"
#include "TransferManager.hpp"

namespace Iridium {
/** CacheLayer.hpp -- CacheLayer subclass of TransferLayer, and CachePolicy */
namespace Transfer {


class CachePolicy {
public:

	virtual ~CachePolicy() {
	}

	virtual bool use(const FileId &id) = 0;
	virtual void invalidate(const FileId &id) = 0;
	virtual void insertAndUse(const FileId &id, size_t size) = 0;

	/// @returns whether the requiredSpace could be allocated.
	virtual bool allocateSpace(size_t requiredSpace) = 0;
};

/// Superclass of any TransferLayer that also serves as a caching layer.
class CacheLayer : TransferLayer {
protected:

	typedef void *CacheInfoPtr;

	CachePolicy *mCachePolicy;
	virtual void populateCache(const FileId &fileId, const DenseData &respondData) = 0;
public:
	virtual ~CacheLayer() {
	}

	CacheLayer(TransferManager *cacheMgr, CachePolicy *policy, CacheLayer *respondTo, TransferLayer *tryNext)
		: TransferLayer(cacheMgr, respondTo,tryNext), mCachePolicy(policy) {
	}
};


}
}

#endif /* IRIDIUM_CacheLayer_HPP__ */
