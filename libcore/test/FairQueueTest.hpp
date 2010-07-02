/*  Sirikata
 *  FairQueueTest.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_FAIR_QUEUE_TEST_HPP_
#define _SIRIKATA_FAIR_QUEUE_TEST_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/queue/FairQueue.hpp>
#include <cxxtest/TestSuite.h>

class FairQueueTest : public CxxTest::TestSuite
{
public:
    struct SizedElem {
        uint32 val;

        SizedElem(uint32 v)
         : val(v) {}

        uint32 size() const {
            return val;
        }
    };
    typedef Queue<SizedElem*> SizedElemQueue;

// Simple macro to evaluate popped results: takes a queue
#define ASSERT_FAIR_QUEUE_POP(queue, expected_key, expected_val)        \
        {                                                               \
            uint32 result_key;                                          \
            SizedElem* result = test_queue.pop(&result_key);            \
            TS_ASSERT_EQUALS(result_key, expected_key);                 \
            TS_ASSERT_EQUALS(result->val, expected_val);                \
            delete result;                                              \
        }

    // Equal weights, different sizes
    void testEqualWeightDifferentSizes(void) {
        FairQueue<SizedElem, uint32, SizedElemQueue> test_queue;

        test_queue.addQueue(new SizedElemQueue(1 << 28), 0, 1.f);
        test_queue.addQueue(new SizedElemQueue(1 << 28), 1, 1.f);
        test_queue.addQueue(new SizedElemQueue(1 << 28), 2, 1.f);

        test_queue.push(0, new SizedElem(1));
        test_queue.push(1, new SizedElem(2));
        test_queue.push(2, new SizedElem(3));
        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 1);
        ASSERT_FAIR_QUEUE_POP(test_queue, 1, 2);
        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 3);
    }


    // Equal weights, different sizes, multiple items per queue
    void testEqualWeightDifferentSizesMultiple(void) {
        FairQueue<SizedElem, uint32, SizedElemQueue> test_queue;

        test_queue.addQueue(new SizedElemQueue(1 << 28), 0, 1.f);
        test_queue.addQueue(new SizedElemQueue(1 << 28), 1, 1.f);
        test_queue.addQueue(new SizedElemQueue(1 << 28), 2, 1.f);

        test_queue.push(0, new SizedElem(1));
        test_queue.push(0, new SizedElem(1));
        test_queue.push(0, new SizedElem(2));

        test_queue.push(1, new SizedElem(3));
        test_queue.push(1, new SizedElem(3));

        test_queue.push(2, new SizedElem(5));
        test_queue.push(2, new SizedElem(2));
        test_queue.push(2, new SizedElem(1));

        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 1); // t = 1
        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 1); // t = 2
        ASSERT_FAIR_QUEUE_POP(test_queue, 1, 3); // t = 3
        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 2); // t = 4
        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 5); // t = 5
        ASSERT_FAIR_QUEUE_POP(test_queue, 1, 3); // t = 6
        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 2); // t = 7
        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 1); // t = 8
    }

    // Different weights, different sizes, multiple items per queue
    void testDifferentWeightsDifferentSizesMultiple(void) {
        FairQueue<SizedElem, uint32, SizedElemQueue> test_queue;

        test_queue.addQueue(new SizedElemQueue(1 << 28), 0, 0.5f);
        test_queue.addQueue(new SizedElemQueue(1 << 28), 1, 1.f);
        test_queue.addQueue(new SizedElemQueue(1 << 28), 2, 2.f);

        test_queue.push(0, new SizedElem(1));
        test_queue.push(0, new SizedElem(1));
        test_queue.push(0, new SizedElem(2));

        test_queue.push(1, new SizedElem(3));
        test_queue.push(1, new SizedElem(3));

        test_queue.push(2, new SizedElem(2));
        test_queue.push(2, new SizedElem(8));
        test_queue.push(2, new SizedElem(8));

        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 2); // t = 1
        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 1); // t = 2
        ASSERT_FAIR_QUEUE_POP(test_queue, 1, 3); // t = 3
        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 1); // t = 4
        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 8); // t = 5
        ASSERT_FAIR_QUEUE_POP(test_queue, 1, 3); // t = 6
        ASSERT_FAIR_QUEUE_POP(test_queue, 0, 2); // t = 8
        ASSERT_FAIR_QUEUE_POP(test_queue, 2, 8); // t = 9
    }
};

#endif //_SIRIKATA_FAIR_QUEUE_TEST_HPP_
