// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/network/IOServicePool.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOStrand.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/util/DynamicLibrary.hpp>
#include <sirikata/core/task/Time.hpp>
#include <cxxtest/TestSuite.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/lexical_cast.hpp>
#include <sirikata/core/util/Timer.hpp>

using namespace Sirikata::Network;
using namespace Sirikata;
class TCPSstCloseTest : public CxxTest::TestSuite
{
    enum {
        NUM_TEST_STREAMS=3
    };
    IOServicePool* mSendService;
    IOStrand* mSendStrand;
    IOServicePool* mRecvService;
    IOStrand* mRecvStrand;
    String mPort;
    int mBytes;
    int mChunks;
    StreamListener * mListener;
    ///only close streams higher than moffset
    int mOffset;
    int mSendReceiveMap[NUM_TEST_STREAMS];
    Stream * mSenders[NUM_TEST_STREAMS];
    Stream * mReceivers[NUM_TEST_STREAMS];
    //number of bytes received *2 + 1 if closed
    AtomicValue<int> mReceiversData[NUM_TEST_STREAMS];
    void listenerConnectionCallback(int id, Stream::ConnectionStatus stat, const std::string&reason) {
        if (stat!=Stream::Connected) {
            if(mSendReceiveMap[id]==-1) {
                SILOG(ssttest,error,"Connection failure for stream "<<id<<" with reason "<<reason);
                TS_ASSERT(mSendReceiveMap[id]!=-1);
            }else{
                mReceiversData[mSendReceiveMap[id]]+=1;
            }
            //printf("%d closed=%d\n",id,mReceiversData[id].read());
/*
            void *bt[16384];
            int size=backtrace(bt,16384);
            backtrace_symbols_fd(bt,size,0);
*/
        }
    }
    void connectionCallback(int id, Stream::ConnectionStatus stat, const std::string&reason) {
        if (stat!=Stream::Connected) {
            TS_ASSERT(false);
            printf ("CONNECTION FAIL %s\n",reason.c_str());
        }else {
            //printf ("ACTIVE\n");
        }
    }
    void listenerDataRecvCallback(Stream *s,int id, const Chunk&data, const Stream::PauseReceiveCallback& pauseReceive) {
        if (mSendReceiveMap[id]==-1) {
            mSendReceiveMap[id]=(unsigned char)data[0];
        }
        static bool pause=true;
        if (rand()<RAND_MAX/5||pause)
            pause=!pause;
        if (pause) {
            mRecvService->service()->post(std::tr1::bind(&Stream::readyRead,s), "TCPSSTCloseTest");
            pauseReceive();
            return;
        }
        mReceiversData[mSendReceiveMap[id]]+=(int)(data.size()*2);
    }

    void listenerNewStreamCallback (Stream * newStream, Stream::SetCallbacks& setCallbacks) {
        if (newStream) {
            for (int i=0;i<NUM_TEST_STREAMS;++i) {
                if (!mReceivers[i]) {
                    mReceivers[i]=newStream;
                    using std::tr1::placeholders::_1;
                    using std::tr1::placeholders::_2;
                    setCallbacks(std::tr1::bind(&TCPSstCloseTest::listenerConnectionCallback,this,i,_1,_2),
                        std::tr1::bind(&TCPSstCloseTest::listenerDataRecvCallback,this,newStream,i,_1,_2),
                                 &Stream::ignoreReadySendCallback);
                    break;
                }
            }
        }
    }
public:
    static TCPSstCloseTest*createSuite() {
        return new TCPSstCloseTest;
    }
    static void destroySuite(TCPSstCloseTest*sst) {
        delete sst;
    }
private:
    Sirikata::PluginManager plugins;

    TCPSstCloseTest() {
        plugins.load( "tcpsst" );

        uint32 randport = 3000 + (uint32)(Sirikata::Task::LocalTime::now().raw() % 20000);
        mPort = boost::lexical_cast<std::string>(randport);

        mBytes=65536;
        mChunks=3;
        mOffset=1;
        mSendService = new IOServicePool("TCPSstCloseTest Send", 4);
        mSendStrand = mSendService->service()->createStrand("TCPSstCloseTest Send");
        mRecvService = new IOServicePool("TCPSstCloseTest Receive", 4);
        mRecvStrand = mRecvService->service()->createStrand("TCPSstCloseTest Receive");
        mListener = StreamListenerFactory::getSingleton().getDefaultConstructor()(mRecvStrand,StreamListenerFactory::getSingleton().getDefaultOptionParser()(String()));
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        mListener->listen(Address("127.0.0.1",mPort),std::tr1::bind(&TCPSstCloseTest::listenerNewStreamCallback,this,_1,_2));

        mRecvService->run();
        Timer::sleep(Duration::seconds(1));
    }
public:
    void closeStreamRun(bool fork, bool doSleep=false, const std::string &fragmentPacketLevel="0") {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        for(int i=0;i<NUM_TEST_STREAMS;++i) {
            mReceivers[i]=mSenders[i]=NULL;
            mSendReceiveMap[i]=-1;
            mReceiversData[i]=0;
        }

        for(int i=0;i<NUM_TEST_STREAMS;++i) {
            if (i==0||!fork) {
                mSenders[i]=StreamFactory::getSingleton().getDefaultConstructor()(mSendStrand,StreamFactory::getSingleton().getDefaultOptionParser()(String("--parallel-sockets=1 --websocket-draft-76=false --test-fragment-packet-level=")+fragmentPacketLevel));
                mSenders[i]->connect(Address("127.0.0.1",mPort),
                                     &Stream::ignoreSubstreamCallback,
                                     std::tr1::bind(&TCPSstCloseTest::connectionCallback,this,i,_1,_2),
                                     &Stream::ignoreReceivedCallback,
                                     &Stream::ignoreReadySendCallback);
            }else {
                mSenders[i]=mSenders[0]->clone(  std::tr1::bind(&TCPSstCloseTest::connectionCallback,this,i,_1,_2),
                                     &Stream::ignoreReceivedCallback,
                                     &Stream::ignoreReadySendCallback);

            }

        }
        mSendService->reset();
        mSendService->run();
        Timer::sleep(Duration::seconds(1));

        int sentSoFar=0;
        for (int c=0;c<mChunks;++c) {
            int willHaveSent=(c+1)*mBytes/mChunks;

            Chunk cur(willHaveSent-sentSoFar);
            for (size_t j=0;j<cur.size();++j) {
                cur[j]=j+sentSoFar;
            }
            for (int i=0;i<NUM_TEST_STREAMS;++i) {
                if (cur.size()) {
                    cur[0]=i%256;
                    if (cur.size()>1) {
                        cur[1]=i/256%256;
                    }
                    if (cur.size()>2) {
                        cur[2]=i/256/256%256;
                    }
                    if (cur.size()>3) {
                        cur[3]=i/256/256/256%256;
                    }
                }
                bool retval=mSenders[i]->send(MemoryReference(cur),ReliableUnordered);
                TS_ASSERT(retval);

            }
            if (doSleep) {
                Timer::sleep(Duration::seconds(1));
            }
            sentSoFar=willHaveSent;
        }
        if (!doSleep) {
            Timer::sleep(Duration::seconds(1));
        }
        //SILOG(tcpsst,error,"CLOSING");
        for (int i=mOffset;i<NUM_TEST_STREAMS;++i) {
            mSenders[i]->close();
        }
        //SILOG(tcpsst,error,"CLOSED");
        bool done=true;
        int counter=0;
        do {
            done=true;
            for (int i=mOffset;i<NUM_TEST_STREAMS;++i) {
                if (mReceiversData[i].read()!=mBytes*2+1) {
                    done=false;
                    if (counter>4998) {
                        SILOG(ssttest,error,"Data "<<i<<" only "<<mReceiversData[i].read()<<" != "<<mBytes*2+1);
                    }
                }
            }
            if (counter>4990&&!done) {
                Timer::sleep(Duration::seconds(1));
            }
            ++counter;
        }while (counter<5000&&!done);
        TS_ASSERT(done);
        for (int i=0;i<NUM_TEST_STREAMS;++i) {
            delete mSenders[i];
            mSenders[i]=NULL;
        }
        for (int i=0;i<NUM_TEST_STREAMS;++i) {
            delete mReceivers[i];
            mReceivers[i]=NULL;
        }
    }
    void testCloseAllStream() {
        mOffset=0;
        closeStreamRun(false);
    }
    void testCloseAllStreams() {
        mOffset=0;
        closeStreamRun(true);
    }
    void testCloseSomeStream() {
        mOffset=1;
        closeStreamRun(false);
    }
    void testCloseSomeStreams() {
        mOffset=1;
        closeStreamRun(true);
    }
    void testCloseSomeStreamSleep() {
        mOffset=1;
        closeStreamRun(false,true);
    }
    void testCloseSomeStreamsSleep() {
        mOffset=1;
        closeStreamRun(true,true);
    }


    void testCloseAllStreamFrag() {
        mOffset=0;
        closeStreamRun(false,false,"1");
    }
    void testCloseAllStreamsFrag() {
        mOffset=0;
        closeStreamRun(true,false,"2");
    }
    void testCloseSomeStreamFrag() {
        mOffset=1;
        closeStreamRun(false,false,"1");
    }
    void testCloseSomeStreamsFrag() {
        mOffset=1;
        closeStreamRun(true,false,"2");
    }
    void testCloseSomeStreamSleepFrag() {
        mOffset=1;
        closeStreamRun(false,true,"1");
    }
    void testCloseSomeStreamsSleepFrag() {
        mOffset=1;
        closeStreamRun(true,true,"2");
    }
    ~TCPSstCloseTest() {

        delete mListener;
        mSendService->join();
        mRecvService->join();
        delete mSendStrand;
        delete mRecvStrand;
        delete mSendService;
        delete mRecvService;

    }
};
