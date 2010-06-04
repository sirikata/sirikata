/*  Sirikata Transfer -- Content Transfer management system
 *  CacheMap.hpp
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
 *  * Neither the name of Sirikata nor the names of its contributors may
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
/*  Created on: Jan 7, 2009 */

#ifndef SIRIKATA_CacheMap_HPP__
#define SIRIKATA_CacheMap_HPP__

#include "CachePolicy.hpp"
#include "CacheLayer.hpp"
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

namespace Sirikata {
namespace Transfer {

/**
 * Handles locking, and also stores a map that can be used
 * both by the CachePolicy, and by the CacheLayer.
 */
class CacheMap : Noncopyable {
public:
	typedef CacheLayer::CacheEntry *CacheData;

	class read_iterator;
	class write_iterator;

private:
	typedef CachePolicy::Data *PolicyData;
	typedef std::pair<CacheData, std::pair<PolicyData, cache_usize_type> > MapEntry;
	typedef std::map<Fingerprint, MapEntry> MapClass;

	MapClass mMap;
	boost::shared_mutex mMapLock;

	CacheLayer *mOwner;
	CachePolicy *mPolicy;

	inline void destroyCacheLayerEntry(const Fingerprint &id, const CacheData &data, cache_usize_type size) {
		mOwner->destroyCacheEntry(id, data, size);
	}

public:
	CacheMap(CacheLayer *owner, CachePolicy *policy) :
		mOwner(owner), mPolicy(policy) {
	}
    void setOwner(CacheLayer *owner) {//if you can't afford to initialize in initializer list
        mOwner=owner;
    }

	~CacheMap() {
		write_iterator clearIterator(*this);
		clearIterator.eraseAll();
	}

	/**
	 * Allocates the requested number of bytes, and erases the
	 * appropriate set of entries using CachePolicy::allocateSpace().
	 *
	 * @param required  The space required for the new entry.
         * @param writer    Write iterator used to process deletions.
	 * @returns         if the allocation was successful,
	 *                  or false if the entry is not to be cached.
	 */
	inline bool alloc(cache_usize_type required, write_iterator &writer) {
		bool cachable = mPolicy->cachable(required);
		if (!cachable) {
			return false;
		}
		Fingerprint toDelete;
		while (mPolicy->nextItem(required, toDelete)) {
			writer.find(toDelete);
			writer.erase();
		}
		return true;
	}

	/**
	 * A read-only iterator.  Not const because the LRU use-count
	 * is allowed to be updated, even though the CacheLayer cannot
	 * be changed.  A read_iterator locks the map using a
	 * boost::shared_lock.  This means that any number of read_iterator
	 * objects are allowed access at the same time, except when
	 * a write_iterator is in use.
	 */
	class read_iterator {
		CacheMap *mCachemap;
		boost::shared_lock<boost::shared_mutex> mLock;

		MapClass *mMap;
		MapClass::iterator mIter;

	public:
		/// Construct from a CacheMap (contains a scoped read lock)
		read_iterator(CacheMap &m)
			: mCachemap(&m), mLock(m.mMapLock),
			mMap(&m.mMap), mIter(m.mMap.end()) {
		}

		/// @returns   if this iterator can be dereferenced.
		inline operator bool () const{
			return (mIter != mMap->end());
		}

		inline bool iterate () {
			if (mIter == mMap->end()) {
				mIter = mMap->begin();
			} else {
				++mIter;
			}
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
		inline CacheData operator* () const {
			return (*mIter).second.first;
		}

		/// @returns the current ID (does not check validity)
		inline const Fingerprint &getId() const {
			return (*mIter).first;
		}

		/// @returns the stored space usage of this item.
		inline cache_usize_type getSize() const {
			return (*mIter).second.second.second;
		}

		/// @returns the CachePolicy opaque data (does not check validity)
		inline PolicyData getPolicyInfo() {
			return (*mIter).second.second.first;
		}

		/// Sets the use bit in the corresponding cache policy.
		inline void use() {
			mCachemap->mPolicy->use(getId(), getPolicyInfo(), getSize());
		}
	};

	/**
	 * A read-write iterator.  Also contains insert() and erase()
	 * functions which also interact with the appropriate CachePolicy.
	 * The write_iterator aassumes exclusive ownership of the map.
	 * Since creating two write_iterators at once causes deadlock,
	 * make sure to call the alloc() function that takes a write_iterator
	 * argument if you already own one.
	 */
	class write_iterator : Noncopyable {
		CacheMap *mCachemap;
		boost::unique_lock<boost::shared_mutex> mLock;

		MapClass *mMap;
		MapClass::iterator mIter;

	public:
		/// Construct from a CacheMap (contains a scoped write lock)
		write_iterator(CacheMap &m)
			: mCachemap(&m), mLock(m.mMapLock),
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
		inline CacheData &operator* () {
			return (*mIter).second.first;
		}

		/// @returns the current ID (does not check validity)
		inline const Fingerprint &getId() const {
			return (*mIter).first;
		}

		/// @returns the stored space usage of this item.
		inline cache_usize_type getSize() const {
			return (*mIter).second.second.second;
		}

		/// @returns the CachePolicy opaque data (does not check validity)
		inline PolicyData getPolicyInfo() {
			return (*mIter).second.second.first;
		}

		/// Sets the use bit in the corresponding cache policy.
		inline void use() {
			mCachemap->mPolicy->use(getId(), getPolicyInfo(), getSize());
		}

		/**
		 * Calls use(), and updates the size of this element.
		 * This function has no other effect if the size is unchanged.
		 *
		 * @param newSize  The new total size of this element.
		 */
		inline void update(cache_usize_type newSize) {
			cache_usize_type oldSize = getSize();
			(*mIter).second.second.second = newSize;
			mCachemap->mPolicy->useAndUpdate(getId(),
					getPolicyInfo(), oldSize, newSize);
		}

		/**
		 * Erases the current iterator.  Note that this iterator is
		 * invalidated at the point you erase it.
		 *
s		 * Also, calls CachePolicy::destroy() and CacheInfo::destroy()
		 */
		void erase() {
			mCachemap->mPolicy->destroy(getId(), getPolicyInfo(), getSize());
			mCachemap->destroyCacheLayerEntry(getId(), (**this), getSize());
			mMap->erase(mIter);
			mIter = mMap->end();
		}

		/** Iterates through the whole map, destroy()ing everything.  Note that the
		 * write_iterator contains no iterate() method because it is generally not safe.
		 */
		void eraseAll() {
			for (mIter = mMap->begin(); mIter != mMap->end(); ++mIter) {
				mCachemap->mPolicy->destroy(getId(), getPolicyInfo(), getSize());
				mCachemap->destroyCacheLayerEntry(getId(), (**this), getSize());
			}
			mMap->clear();
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
		 * @param size    The amount of space reserved for this entry--used
		 *                only if the entry did not exist before.
		 * @returns       If this element was actually inserted.
		 */
		bool insert(const Fingerprint &id, cache_usize_type size) {
			std::pair<MapClass::iterator, bool> ins=
				mMap->insert(MapClass::value_type(id,
						MapEntry(CacheData(), std::pair<PolicyData, cache_usize_type>(PolicyData(), size))));
			mIter = ins.first;

			if (ins.second) {
				(*mIter).second.second.first = mCachemap->mPolicy->create(id, size);
			}
			return ins.second;
		}
	};

	friend class read_iterator;
	friend class write_iterator;

};

}
}

#endif /* SIRIKATA_CacheMap_HPP__ */
