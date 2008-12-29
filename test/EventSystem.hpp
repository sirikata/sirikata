#include <cxxtest/TestSuite.h>
#include "task/EventManager.hpp"
#include "task/Time.hpp"
using namespace Iridium;
class EventSystemTestSuite : public CxxTest::TestSuite 
{
    Task::GenEventManager mPersistentManager;
    Task::GenEventManager *mManager;
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
        EventC(float message):Event(Task::IdPair("Test","")),mMessage(message){}
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
    EventSystemTestSuite(){
        
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
        mManager= new Task::GenEventManager();
    }
    void tearDown( void ) 
    {
        delete mManager;
    }
    Task::EventResponse doNotCall(Task::GenEventManager::EventPtr){
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
    void testDeliveryA( void )
    {
        Task::GenEventManager::EventPtr a(new EventA(1));
        Task::GenEventManager::EventPtr b(new EventB(2));
        Task::GenEventManager::EventPtr c(new EventC(3));
        Task::GenEventManager::EventPtr d(new EventD(4));
        Task::GenEventManager::EventPtr e(new EventE(5));
        mManager->subscribe(a->getId(),
                            boost::bind(&EventSystemTestSuite::oneShotTest,this,_1));
        mManager->subscribe(b->getId(),
                            boost::bind(&EventSystemTestSuite::doNotCall,this,_1));
        mManager->subscribe(c->getId(),
                            boost::bind(&EventSystemTestSuite::doNotCall,this,_1));
        mManager->subscribe(d->getId(),
                            boost::bind(&EventSystemTestSuite::doNotCall,this,_1));
        mManager->subscribe(e->getId(),
                            boost::bind(&EventSystemTestSuite::doNotCall,this,_1));
        mManager->fire(a);
    
	mManager->temporary_processEventQueue(Task::AbsTime::null()); // FIXME: This function will change.
	// call twice just to make sure.
	mManager->temporary_processEventQueue(Task::AbsTime::null());

	int handler_A_should_have_been_called_exactly_once = mCount;
        TS_ASSERT_EQUALS(handler_A_should_have_been_called_exactly_once,1);
        TS_ASSERT(mFail==false&&"Wrong handler got the signal");            

    int a_removeId = mManager->subscribeId(a->getId(),
                            boost::bind(&EventSystemTestSuite::oneShotTest,this,_1));
	mManager->unsubscribe(a_removeId);
	mCount = 0;
	mManager->fire(a);
	mManager->fire(b);
	mManager->fire(c);
	mManager->fire(d);
	mManager->fire(e);
	
	mManager->temporary_processEventQueue(Task::AbsTime::null());

        TS_ASSERT(mFail==true&&"Not enough handlers passed");
	int unsubscribe_should_be_0 = mCount;
        TS_ASSERT_EQUALS(unsubscribe_should_be_0, 0);
  }
};

