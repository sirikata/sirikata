/*  Sirikata Tests -- Sirikata Test Suite
 *  EventTest.hpp
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
#include "task/WorkQueue.hpp"
#include "task/EventManager.hpp"
#include "task/Time.hpp"
using namespace Sirikata;
class EventSystemTestSuite : public CxxTest::TestSuite
{
    Task::ThreadSafeWorkQueue mPersistentQueue;
    Task::GenEventManager mPersistentManager;
    Task::GenEventManager *mManager;
    Task::WorkQueue *mQueue;
    int mCount;
    bool mFail;

    class EventA:public Task::Event{
    public:
        int mMessage;
        EventA(int message):Event(Task::IdPair("Test",0)),mMessage(message){}
    };
    class EventB:public Task::Event{
    public:
        int mMessage;
        EventB(int message):Event(Task::IdPair("Test",1)),mMessage(message){}
    };
    // FIXME: SecondaryId "" is not currently guaranteed to be distinct from 0.
    class EventC:public Task::Event{
    public:
        float mMessage;
        EventC(float message):Event(Task::IdPair("Test",-1)),mMessage(message){}
    };
    class EventD:public Task::Event{
    public:
        float mMessage;
        EventD(float message):Event(Task::IdPair("Test","secondary")),mMessage(message){}
    };
    class EventE:public Task::Event{
    public:
        float mMessage;
        EventE(float message):Event(Task::IdPair("test",0)),mMessage(message){}
    };
public:
    EventSystemTestSuite() : mPersistentManager(&mPersistentQueue) {
    }
    static EventSystemTestSuite * createSuite( void ) {
        EventSystemTestSuite * mts=new EventSystemTestSuite();
        return mts;
    }
    static void destroySuite(EventSystemTestSuite * k) {
        delete k;
    }
    void setUp( void )
    {
        mCount=0;
        mFail=false;
        mQueue = new Task::ThreadSafeWorkQueue;
        mManager= new Task::GenEventManager(mQueue);
    }
    void tearDown( void )
    {
        delete mManager;
        delete mQueue;
    }
    Task::EventResponse doNotCall(Task::GenEventManager::EventPtr){
        TS_FAIL("Called incorrect function");
        mFail=true;
        return Task::EventResponse::del();
    }
    Task::EventResponse oneShotTest(Task::GenEventManager::EventPtr){
        mCount++;
        return Task::EventResponse::del();
    }
    Task::EventResponse manyShotTest(Task::GenEventManager::EventPtr){
        mCount++;
        return Task::EventResponse::nop();
    }
    void deliveryABCDE( int whichevent )
    {
        Task::GenEventManager::EventPtr a(whichevent==0
                                          ? static_cast<Task::Event*>(new EventA(1))
                                          : (whichevent==1
                                             ? static_cast<Task::Event*>(new EventB(1))
                                             : (whichevent==2
                                                ? static_cast<Task::Event*>(new EventC(1))
                                                : (whichevent==3
                                                   ? static_cast<Task::Event*>(new EventD(1))
                                                   : static_cast<Task::Event*>(new EventE(1))))));
        Task::GenEventManager::EventPtr b(whichevent==1
                                          ? static_cast<Task::Event*>(new EventA(2))
                                          : static_cast<Task::Event*>(new EventB(3)));
        Task::GenEventManager::EventPtr c(whichevent==2
                                          ? static_cast<Task::Event*>(new EventA(3))
                                          : static_cast<Task::Event*>(new EventC(3)));
        Task::GenEventManager::EventPtr d(whichevent==3
                                          ? static_cast<Task::Event*>(new EventA(4))
                                          : static_cast<Task::Event*>(new EventD(4)));
        Task::GenEventManager::EventPtr e(whichevent==4
                                          ? static_cast<Task::Event*>(new EventA(5))
                                          : static_cast<Task::Event*>(new EventE(5)));
        Task::SubscriptionId a_removeId,bid,cid,did,eid;
		using std::tr1::placeholders::_1;
        mManager->subscribe(a->getId(),
                            std::tr1::bind(&EventSystemTestSuite::oneShotTest,this,_1));
        bid=mManager->subscribeId(b->getId(),
                            std::tr1::bind(&EventSystemTestSuite::doNotCall,this,_1));
        cid=mManager->subscribeId(c->getId(),
                            std::tr1::bind(&EventSystemTestSuite::doNotCall,this,_1));
        did=mManager->subscribeId(d->getId(),
                            std::tr1::bind(&EventSystemTestSuite::doNotCall,this,_1));
        eid=mManager->subscribeId(e->getId(),
                            std::tr1::bind(&EventSystemTestSuite::doNotCall,this,_1));
        //fire the first handler
        mManager->fire(a);
        //this second firing should be a noop
        mManager->fire(a);

        mManager->getWorkQueue()->dequeueAll();

        int handler_A_should_have_been_called_exactly_once = mCount;
        TS_ASSERT_EQUALS(handler_A_should_have_been_called_exactly_once,1);
        TS_ASSERT(mFail==false&&"Wrong handler got the signal");

        mManager->unsubscribe(bid);
        mManager->unsubscribe(cid);
        mManager->unsubscribe(did);
        mManager->unsubscribe(eid);
        mCount = 0;
        a_removeId = mManager->subscribeId(a->getId(),
                                                          std::tr1::bind(&EventSystemTestSuite::oneShotTest,this,_1));
        mManager->unsubscribe(a_removeId);
        mManager->fire(a);
        mManager->fire(b);
        mManager->fire(c);
        mManager->fire(d);
        mManager->fire(e);

        mManager->getWorkQueue()->dequeueAll();

        int unsubscribe_should_be_0 = mCount;
        TS_ASSERT_EQUALS(unsubscribe_should_be_0, 0);
    }
    void testDeliveryA( void ) {
        deliveryABCDE(0);
    }

    void testDeliveryB( void ) {
        deliveryABCDE(1);
    }

    void testDeliveryC( void ) {
        deliveryABCDE(2);
    }

    void testDeliveryD( void ) {
        deliveryABCDE(3);
    }

    void testDeliveryE( void ) {
        deliveryABCDE(4);
    }
};
