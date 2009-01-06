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
#include <boost/thread/shared_mutex.hpp>


#include "TransferLayer.hpp"
#include "TransferManager.hpp"
#include "CachePolicy.hpp"

namespace Iridium {
/** CacheLayer.hpp -- CacheLayer subclass of TransferLayer, and CachePolicy */
namespace Transfer {

/**
 * Handles locking, and also stores a map that can be used
 * both by the CachePolicy, and by the CacheLayer.
 */
template <class CacheInfo>
class CacheMap {
	typedef typename CachePolicy<CacheInfo>::Data PolicyData;
	typedef std::pair<CacheInfo, PolicyData> MapEntry;
	typedef std::map<Fingerprint, MapEntry> MapClass;

	MapClass mMap;
	boost::shared_mutex mMapLock;

	CachePolicy<CacheInfo> *mPolicy;

public:
	CacheMap(CachePolicy<CacheInfo> *policy) :
		mPolicy(policy) {
	}

	/**
	 * Allocates the requested number of bytes, and erases the
	 * appropriate set of entries using CachePolicy::allocateSpace().
	 *
	 * @param required  The space required for the new entry.
	 * @returns         if the allocation was successful,
	 *                  or false if the entry is not to be cached.
	 */
	inline bool alloc(size_t required) {
		write_iterator writer;
		return mPolicy->allocateSpace(writer, required);
	}

	/**
	 * A read-only iterator.  Not const because the LRU use-count
	 * is allowed to be updated, even though the CacheLayer cannot
	 * be changed.
	 */
	class read_iterator : boost::noncopyable {
		CacheMap<CacheInfo> *mCachemap;
		boost::shared_lock<boost::shared_mutex> mLock;

		typename MapClass::iterator *mIter;
		MapClass *mMap;

	public:
		/// Construct from a CacheMap (contains a scoped read lock)
		read_iterator(CacheMap &m)
			: mCachemap(&m), mLock(&m.mMapLock),
			mMap(&m.mMap), mIter(m.mMap.end()) {
		}

		/// @returns   if this iterator can be dereferenced.
		inline operator bool () const{
			return (mIter != mMap->end());
		}

		/** Moves this iterator to id.
		 * @param id  what to search for
		 * @returns   if the find was successful.
		 */
		inline bool find(const Fingerprint &id) {
			mIter = mMap->find(id);
			return (bool)*this;
		}

		/// @returns the current CacheInfo (does not check validity)
		inline const CacheInfo& operator* () const {
			return (*mIter).second.first;
		}

		/// @returns the current ID (does not check validity)
		inline const Fingerprint &getId() const {
			return (*mIter).first;
		}

		/// @returns the CachePolicy opaque data (does not check validity)
		inline PolicyData &getPolicyInfo() {
			return (*mIter).second.second;
		}

		/// Sets the use bit in the corresponding cache policy.
		inline void use() {
			mCachemap->mPolicy->use(getId(), getPolicyInfo());
		}
	};

	/**
	 * A read-write iterator.  Also contains insert() and erase()
	 * functions which also interact with the appropriate CachePolicy.
	 */
	class write_iterator : boost::noncopyable {
		CacheMap<CacheInfo> *mCachemap;
		boost::unique_lock<boost::shared_mutex> mLock;

		typename MapClass::iterator *mIter;
		MapClass *mMap;

	public:
		/// Construct from a CacheMap (contains a scoped write lock)
		write_iterator(CacheMap &m)
			: mCachemap(&m), mLock(&m.mMapLock),
			mMap(&m.mMap), mIter(m.mMap.end()) {
		}

		/// @returns   if this iterator can be dereferenced.
		inline operator bool () const{
			return (mIter != mMap->end());
		}

		/** Moves this iterator to id.
		 * @param id  what to search for
		 * @returns   if the find was successful.
		 */
		bool find(const Fingerprint &id) {
			mIter = mMap->find(id);
			return (bool)*this;
		}

		/// @returns the current CacheInfo (does not check validity)
		inline CacheInfo& operator* () {
			return (*mIter).second.first;
		}

		/// @returns the current ID (does not check validity)
		inline const Fingerprint &getId() const {
			return (*mIter).first;
		}

		/// @returns the CachePolicy opaque data (does not check validity)
		inline PolicyData &getPolicyInfo() {
			return (*mIter).second.second;
		}

		/// Sets the use bit in the corresponding cache policy.
		inline void use() {
			mCachemap->mPolicy->use(getId(), getPolicyInfo());
		}

		/**
		 * Calls use(), and updates the size of this element.
		 * This function has no other effect if the size is unchanged.
		 *
		 * @param newSize  The new total size of this element.
		 */
		inline void update(size_t newSize) {
			mCachemap->mPolicy->useAndUpdate(getId(),
					getPolicyInfo(), newSize);
		}

		/**
		 * Erases the current iterator.  Note that this iterator is
		 * invalidated at the point you erase it.
		 *
s		 * Also, calls CachePolicy::destroy() and CacheInfo::destroy()
		 */
		void erase() {
			(*this)->destroy();
			mMap->getCacheInfo()->destroy(getId(), getPolicyInfo());
			mMap->erase(mIter);
			mIter = mMap->end();
		}

		/**
		 * Inserts a new entry into the map, unless it already exists.
		 * Follows the semantics of std::map::insert(id).
		 *
		 * The iterator is guaranteed to be valid after this call.
		 * @note  Make sure to call update(totalSize) with the new size.
		 *
		 * @param id      The Fingerprint to insert under (or search for).
		 * @param member  A CacheInfo to insert -- usually the default constructor.
		 * @param size    The amount of space reserved for this entry--used
		 *                only if the entry did not exist before.
		 * @returns       If this element was actually inserted.
		 */
		bool insert(const Fingerprint &id, const CacheInfo &member, size_t size) {
			std::pair<typename MapClass::iterator, bool> ins=
				mMap->insert(MapClass::value_type(id,
						MapEntry(member, PolicyData())));
			mIter = ins.first;

			if (ins.second) {
				(*mIter).second.second = mCachemap->mPolicy->create(id, size);
			}
			return ins.second;
		}
	};

	friend class read_iterator;
	friend class write_iterator;

};

/// Superclass of any TransferLayer that also serves as a caching layer.
class CacheLayer : TransferLayer {
protected:
	/**
	 * Goes up the heirararchy of cache layers filling in data.
	 *
	 * @param fileId  the Fingerprint to store this data in CacheMap.
	 * @param data    Data to be stored in this CacheLayer.
	 * */
	virtual void populateCache(const Fingerprint &fileId, const DenseDataPtr &data) = 0;
	friend class TransferLayer;
public:
	virtual ~CacheLayer() {
	}

	CacheLayer(TransferManager *cacheMgr, CacheLayer *respondTo, TransferLayer *tryNext)
		: TransferLayer(cacheMgr, respondTo, tryNext) {
	}
};

inline void TransferLayer::populateParentCaches(
		const Fingerprint &fileId,
		const DenseDataPtr &data)
{
	if (mRespondTo) {
		mRespondTo->populateCache(fileId, data);
	}
}

}
}

#endif /* IRIDIUM_CacheLayer_HPP__ */
