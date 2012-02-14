// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_UTIL_CIRCULAR_BUFFER_HPP_
#define _SIRIKATA_CORE_UTIL_CIRCULAR_BUFFER_HPP_

#include <sirikata/core/util/Platform.hpp>


namespace Sirikata {

/** A circular buffer. You specify a maximum number of elements for the buffer
 *  to hold and push/pop items into/out of it. If you exceed the capacity, the
 *  oldest samples still remaining are left in the buffer, and you can
 *  optionally have the value returned when you push another element.
 */
template<typename DataType>
class CircularBuffer {
public:
    /** Data type for optionally returning data. The bool indicates whether any
     *  data is returned, the second value is the value.
     */
    typedef std::pair<bool, DataType> OptionalReturnData;

    /** Create an empty CircularBuffer with the given capacity. */
    CircularBuffer(std::size_t cap)
     : mData(cap),
       mStart(0), mCount(0)
    {
    }
    /** Create a full CircularBuffer with the given capacity, with all values
     *  initialized to v.
     */
    CircularBuffer(std::size_t cap, const DataType& v)
     : mData(cap, v),
       mStart(0), mCount(cap)
    {
    }

    std::size_t size() const { return mCount; }
    std::size_t capacity() const { return mData.size(); }
    bool empty() const { return mCount == 0; }
    bool full() const { return mCount == capacity(); }

    /** Push the given value and, if the buffer overflows, return a copy */
    OptionalReturnData push(const DataType& v) {
        OptionalReturnData ret(false, DataType());
        if (full()) {
            ret.first = true;
            ret.second = pop();
        }

        mData[safeIdx(mStart, mCount)] = v;
        mCount++;

        return ret;
    }

    /** Pop an element. */
    DataType pop() {
        std::size_t old_start_idx = mStart;
        mStart = safeIdx(mStart, 1);
        mCount = safeIdx(mCount, -1);
        return mData[old_start_idx];
    }

    const DataType& front() const {
        assert(!empty());
        return mData[mStart];
    }
    DataType& front() {
        assert(!empty());
        return mData[mStart];
    }

    const DataType& back() const {
        assert(!empty());
        return mData[safeIdx(mStart+mCount, -1)];
    }
    DataType& back() {
        assert(!empty());
        return mData[safeIdx(mStart+mCount, -1)];
    }

    DataType& operator[](std::size_t i) {
        assert(i < size());
        return mData[safeIdx(mStart, i)];
    }
    const DataType& operator[](std::size_t i) const {
        assert(i < size());
        return mData[safeIdx(mStart, i)];
    }

private:
    inline std::size_t safeIdx(std::size_t i, int32 offset) const {
        while(offset < 0)
            offset += capacity();
        return ((i+offset) % capacity());
    }

    std::vector<DataType> mData;
    // The first valid element
    std::size_t mStart;
    // Current number of elements
    std::size_t mCount;
}; // class CircularBuffer

} // namespace Sirikata


#endif //_SIRIKATA_CORE_UTIL_CIRCULAR_BUFFER_HPP_
