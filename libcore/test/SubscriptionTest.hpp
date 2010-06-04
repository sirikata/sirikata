/*  Sirikata Tests -- Sirikata Test Suite
 *  SubscriptionTest.hpp
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

#include <sirikata/core/util/Thread.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/ObjectReference.hpp>
#include "Test_Subscription.pbj.hpp"
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <space/Platform.hpp>
#include "space/Space.hpp"
#include "space/Subscription.hpp"
#include <cxxtest/TestSuite.h>
#include <time.h>
using namespace Sirikata;
using namespace Sirikata::Network;
static unsigned char g_tarray[16]={1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,0};
static unsigned char g_oarray[16]={2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,0};
class SubscriptionTest : public CxxTest::TestSuite
{
public:
    static SubscriptionTest*createSuite() {
        return new SubscriptionTest;
    }
    static void destroySuite(SubscriptionTest*st) {
        delete st;
    }
private:
/*
    bool mDisconnected;
    Network::IOService*mSubIO;
    Network::IOService*mBroadIO;
    AtomicValue<int32> mSubInitStage[3];
    char key[1024];
    MemoryReference stdref;
    void subscriptionCallback(int whichIndex,const Network::Chunk&c){
        if (!c.empty()) {
            TS_ASSERT_SAME_DATA(&c[0],key,1024);
        } else {
            TS_ASSERT("Size 0 memory chunk"&&0);
        }
        ++mSubInitStage[whichIndex];
    }
    void subscriptionDiscon(int whichIndex){
        mSubInitStage[whichIndex]+=(1<<30);
    }
    AtomicValue<int32> mBroadInitStage;
    std::tr1::shared_ptr<Subscription::Server>mServer;
    Subscription::SubscriptionClient *mSub;
    Subscription::Broadcast *mBroad;
    Thread *mSubThread;
    Thread *mBroadThread;
    UUID tBroadcastUUID;
    Subscription::Broadcast::BroadcastStream *tBroadcast;
    UUID oBroadcastUUID;
    Subscription::Broadcast::BroadcastStream *oBroadcast;
    std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription> t100ms;
    std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription> t10ms;
    std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription> t0ms;

    std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription> otherTLS;
    void subscriptionThread() {
        mSubIO->run();
    }
    void broadcastThread() {
        mBroadIO->run();
    }
    void broadcastCallback(Network::Stream::ConnectionStatus status, const std::string&reason) {
        if (status!=Network::Stream::Connected) {
            mDisconnected=true;
        }else {

        }
    }
    Network::Address mBroadcastAddress;
    Network::Address mSubscriptionAddress;
    Subscription::Protocol::Subscribe mSubscriptionMessage;
public:
    SubscriptionTest():stdref(&key,1024),tBroadcastUUID(g_tarray,sizeof(g_tarray)),oBroadcastUUID(g_oarray,sizeof(g_oarray)), mBroadcastAddress("127.0.0.1","7949"),mSubscriptionAddress("127.0.0.1","7948"){
        for (unsigned int i=0;i<sizeof(mSubInitStage)/sizeof(mSubInitStage[0]);++i) {
            mSubInitStage[i]=AtomicValue<int32>(0);
        }
        mDisconnected=false;
        oBroadcast=tBroadcast=NULL;
        mSubIO=Network::IOServiceFactory::makeIOService();
        mBroadIO=Network::IOServiceFactory::makeIOService();
        mBroad = new Subscription::Broadcast(mBroadIO,"");
        mSub = new Subscription::SubscriptionClient(mSubIO,"");
        OptionSet*options=StreamListenerFactory::getSingleton().getDefaultOptionParser()(String());
        std::tr1::shared_ptr<Subscription::Server> tempServer(new Subscription::Server(mBroadIO,
                                                                                       Network::StreamListenerFactory::getSingleton()
                                                                                         .getDefaultConstructor()(mBroadIO,options),
                                                                                       mBroadcastAddress,
                                                                                       Network::StreamListenerFactory::getSingleton()
                                                                                         .getDefaultConstructor()(mSubIO,options),
                                                                                       mSubscriptionAddress,
                                                                                       Duration::seconds(3.0),
                                                                                       1024*1024));
        mServer=tempServer;
        mSubThread=new Thread(std::tr1::bind(&SubscriptionTest::subscriptionThread,this));
        mBroadThread=new Thread(std::tr1::bind(&SubscriptionTest::broadcastThread,this));
        mSubscriptionMessage.mutable_broadcast_address().set_hostname(mSubscriptionAddress.getHostName());
        mSubscriptionMessage.mutable_broadcast_address().set_service(mSubscriptionAddress.getService());
        mSubscriptionMessage.set_broadcast_name(tBroadcastUUID);

    }
    ~SubscriptionTest() {
        t100ms=std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription>();
        t10ms=std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription>();
        t0ms=std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription>();
        otherTLS=std::tr1::shared_ptr<Subscription::SubscriptionClient::IndividualSubscription>();
        delete tBroadcast;
        delete oBroadcast;
#ifdef _WIN32
            Sleep(300);
#else
            sleep(1);
#endif

            mSubIO->stop();
            mBroadIO->stop();
        mBroadThread->join();
        mSubThread->join();
        mServer=std::tr1::shared_ptr<Subscription::Server>();
        delete mBroad;
        delete mSub;
        Network::IOServiceFactory::destroyIOService(mSubIO);
        Network::IOServiceFactory::destroyIOService(mBroadIO);

        delete mBroadThread;
        delete mSubThread;
    }
    void _testSingleSubscribe(){
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        memset(key,1,1024);
        using namespace Sirikata::Subscription;
        mSubscriptionMessage.set_update_period(Duration::milliseconds(100.0));//FIXME
        mSubscriptionMessage.set_broadcast_name(tBroadcastUUID);
        std::string serialized;
        mSubscriptionMessage.SerializeToString(&serialized);
        t10ms=mSub->subscribe(serialized,
                              std::tr1::bind(&SubscriptionTest::subscriptionCallback,this,1,_1),
                              std::tr1::bind(&SubscriptionTest::subscriptionDiscon,this,1));
        tBroadcast=mBroad->establishBroadcast(mBroadcastAddress,
                                              tBroadcastUUID,
                                              std::tr1::bind(&SubscriptionTest::broadcastCallback,this,_2,_3));
        oBroadcast=mBroad->establishBroadcast(mBroadcastAddress,
                                              oBroadcastUUID,
                                              std::tr1::bind(&SubscriptionTest::broadcastCallback,this,_2,_3));
        (*tBroadcast)->send(stdref,Network::ReliableOrdered);

        for (int i=0;i<93;++i) {
#ifdef _WIN32
            Sleep(i<90?100:3000);
#else
            sleep(i<90?0:1);
#endif
            if (mSubInitStage[1].read()==1)
                break;
        }
        TS_ASSERT_EQUALS(mSubInitStage[1].read(),1);
    }
*/
};
