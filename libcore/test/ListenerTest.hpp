/*  Sirikata Tests -- Sirikata Test Suite
 *  ListenerTest.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

class ListenerTestClass {
public:
    int total;
    ListenerTestClass() {total=1;}
    virtual ~ListenerTestClass(){}
    virtual void notify(int i) {
        total*=i;
    }
    virtual void notify0() {
        total+=1;
    }
};

class ListenerTest :  public CxxTest::TestSuite
                   ,public Sirikata::MarkovianProvider1<ListenerTestClass*,int>
{
    class Test :public ListenerTestClass{
    public:
        Test() {total=0;}
        virtual void notify(int i) {
            total+=i;
        }
        
    };
public:
    ListenerTest():Sirikata::MarkovianProvider1<ListenerTestClass*,int>(10){}

    void setUp( void )
    {
    }
    void tearDown( void )
    {
    }
    template <typename T> class TestCallAddRemove :public Sirikata::Provider<T>{
        int callCount;
    public:
        TestCallAddRemove() {
            callCount=0;
        }
        virtual void listenerAdded(T ){callCount+=1;}
        virtual void listenerRemoved(T ){callCount+=16;}
        virtual void firstListenerAdded(T ){callCount+=256;}
        virtual void lastListenerRemoved(T ){callCount+=256*16;}
        
        TestCallAddRemove& test(T aa, T bb, T cc, T dd) {
            T a(aa);
            T b(bb);
            T c(cc);
            T d(dd);
            this->notify(&ListenerTestClass::notify,65535);
            this->addListener(a);
            this->addListener(b);
            this->addListener(c);
            this->notify(&ListenerTestClass::notify,8);
            this->removeListener(a);
            this->addListener(d);
            this->notify(&ListenerTestClass::notify,4);
            this->removeListener(b);
            this->notify(&ListenerTestClass::notify,2);
            this->removeListener(d);
            this->notify(&ListenerTestClass::notify,1);
            this->notify(&ListenerTestClass::notify0);
            this->removeListener(c);
            this->notify(&ListenerTestClass::notify,16);
            TS_ASSERT_EQUALS(a->total,dynamic_cast<Test*>(&*a)?8:8);
            TS_ASSERT_EQUALS(b->total,dynamic_cast<Test*>(&*b)?12:32);
            TS_ASSERT_EQUALS(c->total,dynamic_cast<Test*>(&*c)?16:65);
            TS_ASSERT_EQUALS(d->total,dynamic_cast<Test*>(&*d)?6:8);
            TS_ASSERT_EQUALS(callCount,4+4*16+256+256*16);
            return *this;
        }
    };
    void testListenerCallAddRemove( void ) {
        ListenerTestClass* a=new Test;
        ListenerTestClass* b=new ListenerTestClass;
        ListenerTestClass* c=new Test;
        ListenerTestClass* d=new ListenerTestClass;
        TestCallAddRemove<ListenerTestClass*>().test(a,b,c,d);
        delete a;delete b;delete c;delete d;
    }
    void testSharedListenerCallAddRemove( void ) {
        std::tr1::shared_ptr<ListenerTestClass> a(new Test),b(new ListenerTestClass),c(new Test),d(new Test);
        TestCallAddRemove<std::tr1::shared_ptr<ListenerTestClass> >().test(a,b,c,d);        
    }
    void testStatelessListenerCallAddRemove( void ) {
        Test * a=(new Test),*b=(new Test),*c=(new Test),*d=(new Test);
        

        this->addListener(a);
        this->addListener(b);
        this->addListener(c);
        this->notify(8);
        this->addListener(d);
        this->removeListener(a);
        this->notify(4);
        this->removeListener(b);
        this->notify(2);
        this->removeListener(d);
        this->notify(1);
        this->Provider<ListenerTestClass*>::notify(&ListenerTestClass::notify0);
        this->removeListener(c);
        this->notify(16);
        TS_ASSERT_EQUALS(a->total,18);
        TS_ASSERT_EQUALS(b->total,22);
        TS_ASSERT_EQUALS(c->total,26);
        TS_ASSERT_EQUALS(d->total,14);
        delete a;delete b;delete c; delete d;
    }
};
