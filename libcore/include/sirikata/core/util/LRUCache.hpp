// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_UTIL_LRUCACHE_HPP_
#define _SIRIKATA_CORE_UTIL_LRUCACHE_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

namespace Sirikata {

/** A simple templated LRU cache. You specify the key and value types and the
 *  maximum size and use get() and insert() to use and specify cache values. You
 *  must provide a default value which is returned when no entry is found: this
 *  just keeps the interface simpler by not requiring get() and insert() to
 *  return multiple values (a bool to indicate success/failure and the
 *  value). Calling get() updates the most recent access time and insert() will
 *  return an evicted value if the cache ran out of space.
 */
template<typename KeyType,
    typename ValueType,
    class HashType = std::tr1::hash<KeyType>,
    class PredType = std::equal_to<KeyType> >
class LRUCache {
public:
    LRUCache(uint32 max_size, const ValueType& default_value)
     : mMaxSize(max_size),
       mDefaultValue(default_value),
       mLastAccess(0),
       mEntries()
    {}

    const uint32 size() const { return mEntries.size(); }
    const uint32 maxSize() const { return mMaxSize; }

    ValueType get(const KeyType& key) {
        CacheEntriesByKey& by_key = mEntries.template get<key_tag>();
        typename CacheEntriesByKey::iterator key_it = by_key.find(key);
        if (key_it == by_key.end())
            return mDefaultValue;

        ValueType result = key_it->value;
        // Update access timestamp
        by_key.replace(key_it, CacheEntry(key_it->key, key_it->value, mLastAccess++));
        return result;
    }

    // Insert a value. If a value is evicted, it is returned. It is important to
    // inspect this value if you need to know when an entry is evicted, e.g. if
    // you are storing raw pointers to data that need to be cleaned up when they
    // are no longer referenced.
    ValueType insert(const KeyType& key, const ValueType& value) {
        CacheEntriesByKey& by_key = mEntries.template get<key_tag>();
        typename CacheEntriesByKey::iterator key_it = by_key.find(key);
        // Update an existing entry
        if (key_it != by_key.end()) {
            by_key.replace(key_it, CacheEntry(key, value, mLastAccess++));
            return mDefaultValue;
        }

        // Otherwise, insert a new entry
        by_key.insert(CacheEntry(key, value, mLastAccess++));

        // And check for eviction
        if (by_key.size() > mMaxSize) {
            CacheEntriesByTimestamp& by_ts = mEntries.template get<timestamp_tag>();
            // First entry should have smallest timestamp
            typename CacheEntriesByTimestamp::iterator ts_it = by_ts.begin();
            ValueType result = ts_it->value;
            by_ts.erase(ts_it);
            return result;
        }

        //  If we didn't evict, indicate the caller doesn't need to do anything.
        return mDefaultValue;
    }

private:
    typedef uint64 Timestamp;

    struct CacheEntry {
        CacheEntry(const KeyType& key_, const ValueType& value_, Timestamp t_)
         : key(key_), value(value_), timestamp(t_)
        {}

        KeyType key;
        ValueType value;
        Timestamp timestamp; // Last used
    };

    struct key_tag {};
    struct timestamp_tag {};
    typedef boost::multi_index_container<
        CacheEntry,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique< boost::multi_index::tag<key_tag>, BOOST_MULTI_INDEX_MEMBER(CacheEntry,KeyType,key), HashType, PredType >,
            boost::multi_index::ordered_unique< boost::multi_index::tag<timestamp_tag>, BOOST_MULTI_INDEX_MEMBER(CacheEntry,Timestamp,timestamp) >
            >
        > CacheEntries;
    typedef typename CacheEntries::template index<key_tag>::type CacheEntriesByKey;
    typedef typename CacheEntries::template index<timestamp_tag>::type CacheEntriesByTimestamp;

    // Maximum number of entries
    const uint32 mMaxSize;
    // Default value to indicate non-existant entry
    ValueType mDefaultValue;
    // Last used timestamp, source of new
    Timestamp mLastAccess;
    // The actual cache
    CacheEntries mEntries;
}; // class LRUCache

} // namespace Sirikata

#endif //_SIRIKATA_CORE_UTIL_LRUCACHE_HPP_
