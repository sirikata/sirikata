// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/twitter/TermBloomFilter.hpp>
#include <boost/asio.hpp> // hton/ntoh

namespace Sirikata {
namespace Twitter {

TermBloomFilter::TermBloomFilter(uint32 buckets, uint16 hashes)
 : mFilterBuckets(buckets),
   mFilterBytes((buckets + 7)/8),
   mFilter(new unsigned char[mFilterBytes]),
   mNumHashes(hashes)
{
    // Get number of required bits, buffer with extra 3 to make %
    // operation skew distribution less (instead of minimal coverage +
    // some overage, use more bits to get, e.g., 8x coverage + some
    // overage, making the % make the number of values mapped to a
    // single buckets 8 vs. 9 rather than 1 vs. 2). Then make sure we
    // round up the number of bytes.
    uint32 bits_required = int(log((float)buckets)/log(2.f)) + 1;
    // Can't handle more than uint32. That's a ton o' buckets anyway,
    // so this shouldn't be an issue.
    assert(bits_required <= 32);
    uint32 bits_buffered = bits_required + 3;
    mHashBytesLen = (bits_buffered + 7) / 8;
    // And actually we're more strict than <= 32 bits because of the
    // buffering. But even then, we really shouldn't hit this limit.
    assert(mHashBytesLen <= 4);
}

TermBloomFilter::~TermBloomFilter() {
    delete[] mFilter;
}

TermBloomFilter::TermBloomFilter(const TermBloomFilter& rhs)
 : mFilterBuckets(rhs.mFilterBuckets),
   mFilterBytes(rhs.mFilterBytes),
   mFilter(new unsigned char[mFilterBytes]),
   mNumHashes(rhs.mNumHashes),
   mHashBytesLen(rhs.mHashBytesLen)
{
    memcpy(mFilter, rhs.mFilter, mFilterBytes);
}

TermBloomFilter& TermBloomFilter::operator=(const TermBloomFilter& rhs) {
    assert(mFilterBuckets == rhs.mFilterBuckets);
    assert(mFilterBytes == rhs.mFilterBytes);
    assert(mNumHashes == rhs.mNumHashes);
    assert(mHashBytesLen == rhs.mHashBytesLen);

    assert(mFilter != NULL);
    memcpy(mFilter, rhs.mFilter, mFilterBytes);

    return *this;
}

void TermBloomFilter::insert(const String& term) {
    MultiHashingState state;
    for(uint16 nhash = 0; nhash < mNumHashes; nhash++) {
        uint32 hash_val = computeMoreHashBits(state, term) % mFilterBuckets;
        uint32 bucket_byte_idx = hash_val/8, bucket_bit_idx = hash_val % 8;
        mFilter[bucket_byte_idx] = ( mFilter[bucket_byte_idx] | (((unsigned char)1)<<bucket_bit_idx) );
    }
}

bool TermBloomFilter::lookup(const String& term) {
    MultiHashingState state;
    for(uint16 nhash = 0; nhash < mNumHashes; nhash++) {
        uint32 hash_val = computeMoreHashBits(state, term) % mFilterBuckets;
        uint32 bucket_byte_idx = hash_val/8, bucket_bit_idx = hash_val % 8;
        if ( ( mFilter[bucket_byte_idx] & (((unsigned char)1)<<bucket_bit_idx) ) == 0 )
            return false;
    }
    return true;
}

void TermBloomFilter::serialize(String& output) {
    output.resize(sizeof(uint32) + sizeof(uint16) + bytesSize());
    char* outbuf = &output[0];

    *((uint32*)outbuf) = htonl(size()); outbuf += sizeof(uint32);
    *((uint16*)outbuf) = htons(hashes()); outbuf += sizeof(uint16);
    memcpy(outbuf, mFilter, mFilterBytes);
}

TermBloomFilter TermBloomFilter::deserialize(const String& input) {
    char* inbuf = (char*)&input[0];

    uint32 buckets = ntohl(*((uint32*)inbuf)); inbuf += sizeof(uint32);
    uint32 nhashes = ntohs(*((uint16*)inbuf)); inbuf += sizeof(uint16);
    TermBloomFilter result(buckets, nhashes);
    assert(input.size() == (sizeof(uint32) + sizeof(uint16) + result.bytesSize()));
    memcpy(result.mFilter, inbuf, result.mFilterBytes);
    return result;
}

uint32 TermBloomFilter::computeMoreHashBits(MultiHashingState& state, const String& term) {
    uint32 result = 0;

    for(int i = 0; i < mHashBytesLen; i++) {
        if (state.bytes_left == 0) {
            // Update the hash with the data again
            state.md5.update((unsigned char*)&term[0], term.size());
            // Make a copy to do finalization + get data
            MD5 copied(state.md5);
            copied.finalize();
            memcpy(state.bytes, copied.raw_digest(), MD5_DIGEST_LENGTH);
            state.bytes_left = MD5_DIGEST_LENGTH;
        }
        assert((result & (((uint32)state.bytes[MD5_DIGEST_LENGTH-state.bytes_left]) << (i*8))) == 0);
        result = result | (((uint32)state.bytes[MD5_DIGEST_LENGTH-state.bytes_left]) << (i*8));
        state.bytes_left--;
    }

    return result;
}


} // namespace Twitter
} // namespace Sirikata
