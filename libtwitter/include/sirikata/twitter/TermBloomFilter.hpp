// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBTWITTER_TERM_BLOOM_FILTER_HPP_
#define _SIRIKATA_LIBTWITTER_TERM_BLOOM_FILTER_HPP_

#include <sirikata/twitter/Platform.hpp>
#include <sirikata/core/util/Md5.hpp>

namespace Sirikata {
namespace Twitter {

/** A bloom filter for textual terms. It takes care of the hashing, bloom
 *  filter insertion and querying, and can aggregate filters as long as they are
 *  identically sized.
 */
class SIRIKATA_TWITTER_EXPORT TermBloomFilter {
public:
    TermBloomFilter(uint32 buckets, uint16 hashes);
    ~TermBloomFilter();

    TermBloomFilter(const TermBloomFilter& rhs);
    TermBloomFilter& operator=(const TermBloomFilter& rhs);

    const uint32 size() { return mFilterBuckets; }
    const uint16 hashes() { return mNumHashes; }

    /** Insert the term into the bloom filter, setting each of it's hash buckets
     *  to true.
     */
    void insert(const String& term);
    /** Looks up the term in the bloom filter, returning true if it might have
     *  been inserted and false if it definitely has not.
     */
    bool lookup(const String& term);

    /** Serializes the bloom filter. */
    void serialize(String& output);
    /** Deserialize a bloom filter. NOTE: This overwrites any existing entries. */
    void deserialize(const String& input);


    /** Merge another bloom filter into this one. This just bitwise ORs the two
     *  filters, effectively updating this filter to include the terms from
     *  both.
     */
    void mergeIn(const TermBloomFilter& rhs);

private:
    TermBloomFilter(const String& serialized);

    const uint32 bytesSize() { return mFilterBytes; }

    // State tracked during multi-hashing. This allows us to
    // efficiently compute multiple hashes, and also, by storing it
    // separately, allows us to keep the computation of multiple
    // hashes and what each function (insert, lookup) needs to do with
    // them separate.
    struct MultiHashingState {
        MultiHashingState()
         : md5(), nfinalized(0), bytes_left(0)
        {}

        MD5 md5;
        // Number of times we've called finalized, which tells us
        // where to get more data from as we repeat the
        uint32 nfinalized;
        // Bits that have been computed but not all consumed yet.
        uint8 bytes[MD5_DIGEST_LENGTH];
        uint8 bytes_left;
    };

    // Get another hash value, possibly computing more underlying
    // large hash values if they are needed.
    uint32 computeMoreHashBits(MultiHashingState& state, const String& term);


    // Note: it'd be great to just use something like boost::dynamic_bitset but
    // it doesn't make the data accessible in an efficient way for
    // serialization.

    // Size of the filter
    const uint32 mFilterBuckets;
    // Size of filter in bytes, rounded up
    const uint32 mFilterBytes;
    unsigned char* mFilter;

    // Number of buckets saved by mFilter;
    const uint16 mNumHashes;

    // Number of bytes we use for each hash, selected to use fewer
    // bytes if we have fewer buckets so we get more out of each md5
    uint8 mHashBytesLen;
}; // class TermBloomFilter

} // namespace Twitter
} // namespace Sirikata

#endif //_SIRIKATA_LIBTWITTER_TERM_BLOOM_FILTER_HPP_
