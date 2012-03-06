// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <cxxtest/TestSuite.h>
#include <sirikata/core/util/CircularBuffer.hpp>

class CircularBufferTest : public CxxTest::TestSuite
{
    typedef Sirikata::CircularBuffer<int32> IntCBuffer;
    typedef IntCBuffer::OptionalReturnData CBufReturn;
public:

    void testPushPopValue() {
        IntCBuffer buf(10);

        buf.push(1);
        buf.push(2);

        TS_ASSERT_EQUALS(buf.capacity(), 10);
        TS_ASSERT_EQUALS(buf.size(), 2);
        TS_ASSERT_EQUALS(buf.front(), 1);
        TS_ASSERT_EQUALS(buf.back(), 2);
        TS_ASSERT(!buf.empty());
        TS_ASSERT(!buf.full());
    }

    void testEmpty() {
        IntCBuffer buf(10);
        TS_ASSERT(buf.empty());

        buf.push(1);

        TS_ASSERT(!buf.empty());

        buf.pop();

        TS_ASSERT(buf.empty());
    }

    void testOverflow() {
        // Test that things function properly when the buffer overflows
        IntCBuffer buf(4);

        TS_ASSERT(buf.empty());

        buf.push(1);
        buf.push(2);
        buf.push(3);
        buf.push(4);

        TS_ASSERT(buf.full());

        CBufReturn popped = buf.push(5);
        TS_ASSERT(popped.first);
        TS_ASSERT_EQUALS(popped.second, 1);
        TS_ASSERT_EQUALS(buf.front(), 2);
        TS_ASSERT_EQUALS(buf.back(), 5);
        TS_ASSERT(buf.full());
    }

    void testIndexing() {
        // Test that the indexing operator works properly
        IntCBuffer buf(4);

        // After some initialization
        buf.push(1);
        buf.push(2);
        buf.push(3);
        buf.push(4);
        TS_ASSERT_EQUALS(buf.size(), 4);
        TS_ASSERT_EQUALS(buf[0], 1);
        TS_ASSERT_EQUALS(buf[3], 4);

        // After popping some elements so the initial index differs
        buf.pop();
        buf.pop();
        TS_ASSERT_EQUALS(buf.size(), 2);
        TS_ASSERT_EQUALS(buf[0], 3);
        TS_ASSERT_EQUALS(buf[1], 4);

        // After wrapping around
        buf.push(5);
        buf.push(6);
        buf.push(7);
        buf.push(8);
        TS_ASSERT_EQUALS(buf.size(), 4);
        TS_ASSERT_EQUALS(buf[0], 5);
        TS_ASSERT_EQUALS(buf[3], 8);
    }
};
