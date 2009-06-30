/*  Sirikata Tests -- Sirikata Test Suite
 *  SstTest.hpp
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
#include "Test_Sirikata.pbj.hpp"
#include "util/PluginManager.hpp"
#include "util/RoutableMessage.hpp"
#include "proximity/Platform.hpp"
#include "proximity/ProximitySystem.hpp"
#include "proximity/ProximityConnection.hpp"
#include "proximity/BridgeProximitySystem.hpp"
#include "proximity/ProximitySystemFactory.hpp"
#include "proximity/ProximityConnectionFactory.hpp"
#include <cxxtest/TestSuite.h>
#include <boost/thread.hpp>
#include <time.h>
using namespace Sirikata;
using namespace Sirikata::Network;
class ProxTest : public CxxTest::TestSuite         , MessageService
{
    void ioThread(){
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        Network::IOService*io=mProxIO=Network::IOServiceFactory::makeIOService();
        mRemoteProxSystem=Proximity::ProximitySystemFactory::getSingleton().getDefaultConstructor()(io,"",&Sirikata::Proximity::ProximitySystem::defaultNoAddressProximityCallback);
        mReadyToConnect=true;
        Network::IOServiceFactory::runService(io);
    }
    void pcThread() {
        mReadyToConnect=true;
        Network::IOServiceFactory::runService(mIO);
        
    }
    Proximity::ProximitySystem* mRemoteProxSystem;
    Proximity::ProximitySystem* mLocalProxSystem;
    Network::IOService *mIO;
    Network::IOService *mProxIO;
    boost::thread*mProxThread;
    boost::thread*mThread;
    AtomicValue<int>mSend;
    volatile bool mAbortTest;
    volatile bool mReadyToConnect;
    static const int NUM_OBJECTS=10;
    UUID mObjectId[NUM_OBJECTS];
    AtomicValue<int>mDeliver[NUM_OBJECTS];
public:
    ProxTest():mIO(IOServiceFactory::makeIOService()),mSend(0),mAbortTest(false),mReadyToConnect(false){
        for (int i=0;i<NUM_OBJECTS;++i) {
            mObjectId[i]=UUID::random();
            mDeliver[i]=0;
        }
        Sirikata::PluginManager plugins;
        plugins.load(
#ifdef __APPLE__
#ifdef NDEBUG
            "libprox.dylib"
#else
            "libprox_d.dylib"
#endif
#else
#ifdef _WIN32
#ifdef NDEBUG
            "prox.dll"
#else
            "prox_d.dll"
#endif
#else
#ifdef NDEBUG
            "libprox.so"
#else
            "libprox_d.so"
#endif
#endif
#endif
            );
    
        mProxThread= new boost::thread(std::tr1::bind(&ProxTest::ioThread,this));
        while (!mReadyToConnect) {}
        Proximity::ProximityConnection*proxCon=Proximity::ProximityConnectionFactory::getSingleton().getDefaultConstructor()(mIO,"");
        mLocalProxSystem=new Proximity::BridgeProximitySystem(proxCon,1); 
        mLocalProxSystem->forwardMessagesTo(this);
        mThread=new boost::thread(std::tr1::bind(&ProxTest::pcThread,this));

    }
    static ProxTest*createSuite() {
        return new ProxTest;
    }
    static void destroySuite(ProxTest*prox) {
        delete prox;
    }
    
    ~ProxTest() {
        IOServiceFactory::stopService(mIO);
        IOServiceFactory::stopService(mProxIO);
        mThread->join();
        mProxThread->join();
        delete mProxThread;
        delete mThread;
        IOServiceFactory::destroyIOService(mIO);
        IOServiceFactory::destroyIOService(mProxIO);
        mIO=NULL;
    }
    void testConnectSend (void )
    {
        using namespace Sirikata::Protocol;
        RetObj robj;
        Vector3d locations[NUM_OBJECTS];
        locations[0]=Vector3d(0,0,0);
        locations[1]=Vector3d(64,0,0);
        locations[2]=Vector3d(-64,0,0);
        locations[3]=Vector3d(64,64,64);
        locations[4]=Vector3d(-64,-64,-64);
        locations[5]=Vector3d(256,0,0);
        locations[6]=Vector3d(0,0,256);
        locations[7]=Vector3d(256,0,256);
        locations[8]=Vector3d(256,256,256);
        locations[9]=Vector3d(-256,-256,-256);
        for (int i=0;i<NUM_OBJECTS;++i) {
            robj.set_object_reference(mObjectId[i]);
            robj.mutable_location().set_timestamp(Time::now());
            robj.mutable_location().set_position(locations[i]);
            robj.mutable_location().set_orientation(Quaternion::identity());
            robj.mutable_location().set_velocity(Vector3f(0,0,0));
            mLocalProxSystem->newObj(robj);
        }
        NewProxQuery npq;
        npq.set_query_id(0);
        npq.set_relative_center(Vector3f(0,0,0));
        npq.set_max_radius(0);
        mLocalProxSystem->newProxQuery(ObjectReference(mObjectId[0]),npq,NULL,0);
        npq.set_query_id(1);
        npq.set_max_radius(64);
        mLocalProxSystem->newProxQuery(ObjectReference(mObjectId[0]),npq,NULL,0);
        npq.set_query_id(2);
        npq.set_max_radius(128);
        mLocalProxSystem->newProxQuery(ObjectReference(mObjectId[0]),npq,NULL,0);
        npq.set_query_id(3);
        npq.set_max_radius(256);
        mLocalProxSystem->newProxQuery(ObjectReference(mObjectId[0]),npq,NULL,0);
        npq.set_query_id(4);
        npq.set_max_radius(512);
        mLocalProxSystem->newProxQuery(ObjectReference(mObjectId[0]),npq,NULL,0);
        for (int i=0;i<100;++i) {
#ifdef _WIN32
            Sleep(i<90?100:3000);
#else
            sleep(i<90?0:1);
#endif
            if(mDeliver[4].read()>=10) break;
        }
        TS_ASSERT_EQUALS(mDeliver[0].read(),1);
        TS_ASSERT_EQUALS(mDeliver[1].read(),3);
        TS_ASSERT_EQUALS(mDeliver[2].read(),5);
        TS_ASSERT_EQUALS(mDeliver[3].read(),7);
        TS_ASSERT_EQUALS(mDeliver[4].read(),10);

    }

    void send(const RoutableMessageHeader&opaqueMessage,MemoryReference body){
        Protocol::MessageBody mesg;
        if (mesg.ParseFromArray(body.data(),body.size())) {
            for (int i=0;i<mesg.message_names_size();++i) {
                SILOG(sirikata,warning,"Send opaque message named: "<<mesg.message_names(i));
                ++mSend;
            }
        }
    }
    bool forwardMessagesTo(MessageService*body) {
        return false;
    }
    bool endForwardingMessagesTo(MessageService*body) {
        return false;
    }
    void processMessage(const ObjectReference*reference, MemoryReference message){
        
        RoutableMessageHeader rmh;
        MemoryReference body=rmh.ParseFromArray(message.data(),message.size());
        if (reference)
            rmh.set_destination_object(*reference);
        this->processMessage(rmh,body);
                
    }
    void processMessage(const RoutableMessageHeader&opaqueMessage, MemoryReference body){
        Protocol::MessageBody mesg;
        if (mesg.ParseFromArray(body.data(),body.size())) {
            for (int i=0;i<mesg.message_names_size();++i) {
                if (mesg.message_names(i)=="ProxCall") {
                    Protocol::ProxCall cb;
                    if (cb.ParseFromString(mesg.message_arguments(i))) {
                        int qid=cb.query_id();
                        if (qid<NUM_OBJECTS){
                            ++mDeliver[qid];
                        }
                    }
                }else {
                    SILOG(sirikata,warning,"Opaque message named: "<<mesg.message_names(i));
                }
            }        
        }
    }
};
