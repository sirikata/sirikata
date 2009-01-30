/*  Sirikata Tests -- Sirikata Testing Framework
 *  ThreadSafeQueueTest.hpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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
#include <boost/shared_ptr.hpp>
#include "util/ThreadSafeQueue.hpp"
class ThreadSafeQueueTest : public CxxTest::TestSuite
{
    class MyClass {
        int a;
        int b;
        int c;
        int d;
        int e;
    public:
        MyClass(int i) {
            a=i;
            b=i*2;
            c=i*i;
            d=i+1;
            e=-i;
        }
    };
    LockFreeQueue<boost::shared_ptr<MyClass> > * mQueue;
public:
    void setUp( void ) {
        mQueue= new    LockFreeQueue<boost::shared_ptr<MyClass> >();
    }
    void tearDown (void ) {
        delete mQueue;
        mQueue=NULL;
    }
    void testEnqueue( void ){
        boost::shared_ptr<MyClass> a(new MyClass(0));
        boost::shared_ptr<MyClass> b(new MyClass(1));
        boost::shared_ptr<MyClass> c(new MyClass(2));
        boost::shared_ptr<MyClass> d(new MyClass(3));
        boost::shared_ptr<MyClass> e(new MyClass(4));
        boost::shared_ptr<MyClass> f(new MyClass(5));
        boost::shared_ptr<MyClass> g(new MyClass(6));
        boost::shared_ptr<MyClass> h(new MyClass(7));
        boost::shared_ptr<MyClass> array[8];
        array[0]=a;array[1]=b;array[2]=c;array[3]=d;array[4]=e;array[5]=f;array[6]=g;array[7]=h;
        mQueue->push(a);
        mQueue->push(b);
        mQueue->push(c);
        mQueue->push(d);
        mQueue->push(e);
        mQueue->push(f);
        mQueue->push(g);
        boost::shared_ptr<MyClass> result;
        for (unsigned int i=0;i<8;++i) {
            TS_ASSERT(mQueue->pop(result));
            TS_ASSERT_EQUALS(result,array[i]);
        }
        TS_ASSERT(!mQueue->pop(result));
    }

};
