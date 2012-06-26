// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_UTIL_UNIQUE_ID_HPP_
#define _SIRIKATA_CORE_UTIL_UNIQUE_ID_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
#include <boost/thread.hpp>
#endif

namespace Sirikata {

/** Generates unique IDs of 16-bits. Values will wrap around, so they are not
 *  strictly unique -- you should make sure you use a large enough space to
 *  avoid duplicates after wraparound. For convenience, the value 0 is not
 *  used.
 */
class SIRIKATA_EXPORT UniqueID16 {
public:
    uint16 operator()() {
        uint16 retval;
        do {
            retval = ++sSource;
        } while (retval == 0);
        return (retval % 0xFFFF);
    }

private:
    // No 16-bit atomics because not all platforms have support for 16-bit
    // compare and swap and 16-bit += int16. Instead, use 32-bit and just % the
    // values (note the different implementation above compared to the 32 and 64
    // bit versions).
    static AtomicValue<uint32> sSource;
}; // class UniqueID16

/** Generates unique IDs of 32-bits. Values will wrap around, so they are not
 *  strictly unique -- you should make sure you use a large enough space to
 *  avoid duplicates after wraparound. For convenience, the value 0 is not
 *  used.
 */
class SIRIKATA_EXPORT UniqueID32 {
public:
    uint32 operator()() {
        uint32 retval;
        do {
            retval = ++sSource;
        } while (retval == 0);
        return retval;
    }

private:
    static AtomicValue<uint32> sSource;
}; // class UniqueID32

/** Generates unique IDs of 64-bits. Values will wrap around, so they are not
 *  strictly unique -- you should make sure you use a large enough space to
 *  avoid duplicates after wraparound. For convenience, the value 0 is not
 *  used.
 */
class SIRIKATA_EXPORT UniqueID64 {
public:
    uint64 operator()() {
        uint64 retval;
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        {
            boost::lock_guard<boost::mutex> lck(mMutex);
#endif
            do {
                retval = ++sSource;
            } while (retval == 0);
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
        }
#endif
        return retval;
    }

private:
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
    static boost::mutex mMutex;
    static uint64 sSource;
#else
    static AtomicValue<uint64> sSource;
#endif
}; // class UniqueID64

} // namespace Sirikata


#endif //_SIRIKATA_CORE_UTIL_UNIQUE_ID_HPP_
