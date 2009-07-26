/*  Sirikata Tests -- Sirikata Test Suite
 *  ProxTest.hpp
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

#include "network/TCPStream.hpp"
#include "network/TCPStreamListener.hpp"
#include "network/IOServiceFactory.hpp"
#include "util/ObjectReference.hpp"
#include "Test_Subscription.pbj.hpp"
#include "util/PluginManager.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "util/AtomicTypes.hpp"
#include <subscription/Platform.hpp>
#include "subscription/Server.hpp"
#include "subscription/SubscriptionClient.hpp"
#include "subscription/Broadcast.hpp"
#include <cxxtest/TestSuite.h>
#include <boost/thread.hpp>
#include <time.h>
using namespace Sirikata;
using namespace Sirikata::Network;
class SubscriptionTest : public CxxTest::TestSuite
{
    Network::IOService*mSubIO;
    Network::IOService*mBroadIO;
    AtomicValue<int32> mSubInitStage;
    AtomicValue<int32> mBroadInitStage;
    Subscription::Server*mServer;
    Subscription::SubscriptionClient *mSub;
    Subscription::Broadcast *mBroad;
    void subscriptionThread() {
        Network::IOServiceFactory::runService(mSubIO);
    }
    void broadcastThread() {
        Network::IOServiceFactory::runService(mSubIO);
    }
public:
    SubscriptionTest() {
        mSubIO=Network::IOServiceFactory::makeIOService();
        mBroadIO=Network::IOServiceFactory::makeIOService();
        mBroad = new Subscription::Broadcast(mBroadIO);
        mSub = new Subscription::SubscriptionClient(mSubIO);
        
    }
    static SubscriptionTest*createSuite() {
        return new SubscriptionTest;
    }
    static void destroySuite(SubscriptionTest*st) {
        delete st;
    }
    ~SubscriptionTest() {
        
        Network::IOServiceFactory::stopService(mSubIO);
        Network::IOServiceFactory::stopService(mBroadIO);
        delete mBroad;
        delete mSub;
        Network::IOServiceFactory::destroyIOService(mSubIO);
        Network::IOServiceFactory::destroyIOService(mBroadIO);
        
    }
    void testSingleSubscribe(){}
};
