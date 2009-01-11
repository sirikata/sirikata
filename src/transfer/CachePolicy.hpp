/*     Iridium Transfer -- Content Transfer management system
 *  CachePolicy.hpp
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
/*  Created on: Jan 5, 2009 */

#ifndef IRIDIUM_CachePolicy_HPP__
#define IRIDIUM_CachePolicy_HPP__

#include <vector>

#include "URI.hpp"

namespace Iridium {
namespace Transfer {

/// Critical to the functioning of CacheLayer--makes decisions which pieces of data to keep and which to throw out.
//template <class CacheInfo>
template <class CacheMap>
class CachePolicyMap {
public:
	typedef typename CacheMap::write_iterator write_iterator;

	typedef void *Data;

	/// Virtual destructor since children will allocate class members.
	virtual ~CachePolicyMap() {
	}

	/**
	 *  Marks the entry as used
	 *  @param id    The FileId corresponding to the data.
	 *  @param data  The opaque data corresponding to this policy.
	 */
	virtual void use(const Fingerprint &id, Data &data, size_t size) = 0;

	/**
	 *  Marks the entry as used, and update the space usage
	 *  @param id    The FileId corresponding to the data.
	 *  @param data  The opaque data corresponding to this policy.
	 *  @param size  The amount of space actually used for this element.
	 */
	virtual void useAndUpdate(const Fingerprint &id, Data &data, size_t oldsize, size_t newsize) = 0;

	/**
	 *  Deletes the opaque data (and anything other corresponding info)
	 *
	 *  @param id    The FileId corresponding to the data.
	 *  @param data  The opaque data corresponding to this policy.
	 */
	virtual void destroy(const Fingerprint &id, const Data &data, size_t size) = 0;

	/**
	 *  Allocates opaque data (and anything other corresponding info)
	 *  At this point, the allocation must not fail.  It is up to the
	 *  CacheLayer to respect the decision from allocateSpace().
	 *
	 *  @param id    The FileId corresponding to the data.
	 *  @param size  The amount of space allocated (should be the same
	 *               as was earlier passed to allocateSpace).
	 *  @returns     The corresponding opaque data.
	 */
	virtual Data create(const Fingerprint &id, size_t size) = 0;

	/**
	 *  Allocates a certain number of bytes for use in a new cache entry.
	 *
	 *  @param requiredSpace  the amount of space this single entry needs
	 *  @param iter           A locked CacheMap::write_iterator
	 *                        (to use to delete large entries)
	 *  @returns              whether the space could be allocated
	 */
	virtual bool allocateSpace(
			size_t requiredSpace,
			write_iterator &writer) = 0;
};


// Hack to hold off instantiation until CacheMap::write_iterator is defined.
// Anyone know how to create a forward reference to an inner class?
class CacheMap;
typedef CachePolicyMap<CacheMap> CachePolicy;

}
}

#include "CacheLayer.hpp"

#endif /* IRIDIUM_CachePolicy_HPP__ */
