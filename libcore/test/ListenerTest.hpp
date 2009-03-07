/*  Sirikata Tests -- Sirikata Test Suite
 *  AtomicTest.hpp
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
#include <cxxtest/TestSuite.h>
#include "util/ListenerProvider.hpp"

using namespace Sirikata;
class ListenerTest : public CxxTest::TestSuite
{
public:
    void setUp( void )
    {
    }
    void tearDown( void )
    {
    }
    class Test {
    public:
        int total;
        Test() {total=0;}
        void notify(int i) {
            total+=i;
        }
    };

    void testListenerCallAddRemove( void ) {
        Test * a=(new Test),*b=(new Test),*c=(new Test),*d=(new Test);
        
        Provider<Test*> provider;
        provider.addListener(a);
        provider.addListener(b);
        provider.addListener(c);
        provider.addListener(d);
        provider.notify(8);
        provider.removeListener(a);
        provider.notify(4);
        provider.removeListener(b);
        provider.notify(2);
        provider.removeListener(d);
        provider.notify(1);
        provider.removeListener(c);
        provider.notify(16);
        TS_ASSERT_EQUALS(a->total,8);
        TS_ASSERT_EQUALS(b->total,12);
        TS_ASSERT_EQUALS(c->total,15);
        TS_ASSERT_EQUALS(d->total,14);
        delete a;delete b;delete c; delete d;
    }
    void testSharedListenerCallAddRemove( void ) {
        std::tr1::shared_ptr<Test> a(new Test),b(new Test),c(new Test),d(new Test);
        
        Provider<std::tr1::shared_ptr<Test> > provider;
        provider.addListener(a);
        provider.addListener(b);
        provider.addListener(c);
        provider.addListener(d);
        provider.notify(8);
        provider.removeListener(b);
        provider.notify(4);
        provider.removeListener(a);
        provider.notify(2);
        provider.removeListener(d);
        provider.notify(1);
        provider.removeListener(c);
        provider.notify(16);
        TS_ASSERT_EQUALS(a->total,12);
        TS_ASSERT_EQUALS(b->total,8);
        TS_ASSERT_EQUALS(c->total,15);
        TS_ASSERT_EQUALS(d->total,14);
        
    }
    void testStatelessListenerCallAddRemove( void ) {
        Test * a=(new Test),*b=(new Test),*c=(new Test),*d=(new Test);
        
        StatelessProvider1<Test*,int> provider(10);
        provider.addListener(a);
        provider.addListener(b);
        provider.addListener(c);
        provider.notify(8);
        provider.addListener(d);
        provider.removeListener(a);
        provider.notify(4);
        provider.removeListener(b);
        provider.notify(2);
        provider.removeListener(d);
        provider.notify(1);
        provider.removeListener(c);
        provider.notify(16);
        TS_ASSERT_EQUALS(a->total,18);
        TS_ASSERT_EQUALS(b->total,22);
        TS_ASSERT_EQUALS(c->total,25);
        TS_ASSERT_EQUALS(d->total,14);
        delete a;delete b;delete c; delete d;
    }
};
