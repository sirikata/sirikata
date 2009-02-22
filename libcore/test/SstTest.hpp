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

#include "network/TCPDefinitions.hpp"
#include "network/TCPStream.hpp"
#include "network/TCPStreamListener.hpp"
#include <cxxtest/TestSuite.h>
#include <boost/thread.hpp>
#include <time.h>
using namespace Sirikata::Network;
class SstTest : public CxxTest::TestSuite
{
public:
    void runRoutine(Stream* s) {
        static boost::mutex test;
        boost::lock_guard<boost::mutex> connectingMutex(test);
        for (unsigned int i=0;i<mMessagesToSend.size();++i) {
            s->send(Chunk(mMessagesToSend[i].begin(),mMessagesToSend[i].end()),
                    mMessagesToSend[i].size()?(mMessagesToSend[i][0]=='U'?Stream::ReliableUnordered:(mMessagesToSend[i][0]=='X'?Stream::Unreliable:Stream::Reliable)):Stream::Reliable);
        }
    }
    void connectionCallback(int id, Stream::ConnectionStatus stat, const std::string&reason) {
        Chunk connectionReason(2);
        connectionReason[0]='C';
        connectionReason[1]=(stat==Stream::Connected?' ':'X');
        connectionReason.insert(connectionReason.end(),reason.begin(),reason.end());
        if (stat==Stream::ConnectionFailed) {
            mAbortTest=true;
            TS_FAIL(reason);
        }
/*
        mDataMap[id].push_back(connectionReason);
        if (stat!=Stream::Connected)
            ++mDisconCount;
*/
    }
    void dataRecvCallback(Stream *s,int id, const Chunk&data) {
        mDataMap[id].push_back(data);
        ++mCount;
    }
    void connectorDataRecvCallback(Stream *s,int id, const Chunk&data) {
        dataRecvCallback(s,id,data);
    }
    void listenerDataRecvCallback(Stream *s,int id, const Chunk&data) {
        dataRecvCallback(s,id,data);
    }
    void connectorNewStreamCallback (int id,Stream * newStream, Stream::SetCallbacks& setCallbacks) {
        static int newid=0;
        mStreams.push_back(newStream);
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        setCallbacks(std::tr1::bind(&SstTest::connectionCallback,this,newid,_1,_2),
                     std::tr1::bind(&SstTest::connectorNewStreamCallback,this,newid,_1,_2),
                     std::tr1::bind(&SstTest::connectorDataRecvCallback,this,newStream,newid,_1));
        ++newid;
        runRoutine(newStream);
    }
    void listenerNewStreamCallback (int id,Stream * newStream, Stream::SetCallbacks& setCallbacks) {
        static int newid=0;
        mStreams.push_back(newStream);
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        setCallbacks(std::tr1::bind(&SstTest::connectionCallback,this,newid,_1,_2),
                     std::tr1::bind(&SstTest::listenerNewStreamCallback,this,newid,_1,_2),
                     std::tr1::bind(&SstTest::listenerDataRecvCallback,this,newStream,newid,_1));
        ++newid;
        runRoutine(newStream);
    }
    void ioThread(){
        TCPStreamListener s(mIO);
        s.listen(Address("127.0.0.1",mPort),boost::bind(&SstTest::listenerNewStreamCallback,this,0,_1,_2));
        mReadyToConnect=true;
        mIO.run();
    }
    std::string mPort;
    boost::asio::io_service mIO;
    boost::thread *mThread;
    std::vector<Stream*> mStreams;
    std::vector<std::string> mMessagesToSend;
    Sirikata::AtomicValue<int> mCount;
    Sirikata::AtomicValue<int> mDisconCount;
    typedef Sirikata::uint8 uint8;
    std::map<unsigned int, std::vector<Sirikata::Network::Chunk> > mDataMap;
    const char * ENDSTRING;
    volatile bool mAbortTest;
    volatile bool mReadyToConnect;
    void validateSameness(const std::vector<std::vector<uint8> >&netData,
                          const std::vector<std::vector<uint8> >&keyData) {
        size_t i,j;
        for (i=0,j=0;i<netData.size()&&j<keyData.size();) {
            if (netData[i].size()&&netData[i][0]=='C') {
                ++i;
                continue;
            }
            TS_ASSERT_EQUALS(netData[i].size(),keyData[j].size());
            if (!netData[i].empty()) {
                TS_ASSERT_SAME_DATA(&*netData[i].begin(),&*keyData[j].begin(),netData[i].size()*sizeof(netData[i][0]));
            }
            ++i;
            ++j;
        }
        for (;i<netData.size();++i) {
            if (netData[i].size()) {
                TS_ASSERT_EQUALS(netData[i][0],'C');
            }else {
                TS_FAIL("Stray Empty packets");
            }
        }
        TS_ASSERT_EQUALS(j,keyData.size());
    }
    void validateVector(const std::vector<Sirikata::Network::Chunk>&netData,
        const std::vector<std::string>&keyData) {
        std::vector<std::vector<uint8> > orderedNetData;
        std::vector<std::vector<uint8> > unorderedNetData;
        std::vector<std::vector<uint8> > orderedKeyData;
        std::vector<std::vector<uint8> > unorderedKeyData;
        for (size_t i=0;i<netData.size();++i) {
            if (netData[i].size()==0||netData[i][0]=='T') {
                orderedNetData.push_back(std::vector<uint8>(netData[i].begin(),netData[i].end()));
            }else {
                unorderedNetData.push_back(std::vector<uint8>(netData[i].begin(),netData[i].end()));
            }            
        }
        for (size_t i=0;i<keyData.size();++i) {
            if (keyData[i].size()==0||keyData[i][0]=='T') {
                orderedKeyData.push_back(std::vector<uint8>(keyData[i].begin(),keyData[i].end()));
            }else {
                unorderedKeyData.push_back(std::vector<uint8>(keyData[i].begin(),keyData[i].end()));
            }            
        }
        std::sort(unorderedNetData.begin(),unorderedNetData.end());
        std::sort(unorderedKeyData.begin(),unorderedKeyData.end());
        validateSameness(orderedNetData,orderedKeyData);
        validateSameness(unorderedNetData,unorderedKeyData);
    }
    SstTest():mCount(0),mDisconCount(0),ENDSTRING("T end"),mAbortTest(false),mReadyToConnect(false){
        mPort="9142";
        mThread= new boost::thread(boost::bind(&SstTest::ioThread,this));
        bool doUnorderedTest=false;
        bool doShortTest=true;
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:0");
            mMessagesToSend.push_back("U:1");
            mMessagesToSend.push_back("U:2");
        }
        mMessagesToSend.push_back("T0th");
        mMessagesToSend.push_back("T1st");
        mMessagesToSend.push_back("T2nd");
        mMessagesToSend.push_back("T3rd");
        if (!doShortTest) {
        mMessagesToSend.push_back("T4th");
        mMessagesToSend.push_back("T5th");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:3");
            mMessagesToSend.push_back("U:4");
            mMessagesToSend.push_back("U:5");
        }
        mMessagesToSend.push_back("T6th");
        mMessagesToSend.push_back("T7th");
        mMessagesToSend.push_back("T8th");
        mMessagesToSend.push_back("T9th");
/*
        std::string test("T");
        for (unsigned int i=0;i<16385;++i) {
            test+=(char)((i+5)%128);
        }
        mMessagesToSend.push_back(test);
        if (doUnorderedTest){
        test[0]='U';
        mMessagesToSend.push_back(test);
}
        for (unsigned int i=0;i<4096*4096;++i) {
            test+=(char)((rand())%256);
        }
        if (doUnorderedTest){
        mMessagesToSend.push_back(test);
}
        test[0]='T';
        mMessagesToSend.push_back(test);
*/
        mMessagesToSend.push_back("T_0th");
        mMessagesToSend.push_back("T_1st");
        mMessagesToSend.push_back("T_2nd");
        mMessagesToSend.push_back("T_3rd");
        mMessagesToSend.push_back("T_4th");
        mMessagesToSend.push_back("T_5th");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:6");
            mMessagesToSend.push_back("U:7");
            mMessagesToSend.push_back("U:8");
            mMessagesToSend.push_back("U:9");
            mMessagesToSend.push_back("U:A");
        }
        mMessagesToSend.push_back("T_6th");
        mMessagesToSend.push_back("T_7th");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:B");
        }
        mMessagesToSend.push_back("T_8th");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:C");
        }
        mMessagesToSend.push_back("T_9th");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:D");
        }
        mMessagesToSend.push_back("The green grasshopper fetched.");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:E");
            mMessagesToSend.push_back("U:F");
            mMessagesToSend.push_back("U:G");
        }
        mMessagesToSend.push_back("T A blade of grass.");
        mMessagesToSend.push_back("T From the playground .");
//        mMessagesToSend.push_back("T Grounds test test test this is a test test test this is a test test test this is a test test test test and the test is proceeding until it reaches signific length with a string that long however. this is not quite long enough to trigger the high water mark--well now it is I believe to the best of my abilities");
//        mMessagesToSend.push_back("T Grounds test test test this is a test test test this is a test test test this is a test test test test and the test is proceeding until it reaches signific length with a string that long however. this is not quite");
        if (doUnorderedTest){
            mMessagesToSend.push_back("U:H");
            mMessagesToSend.push_back("U:I");
            mMessagesToSend.push_back("U:J");
            mMessagesToSend.push_back("U:K");
            mMessagesToSend.push_back("U:L");
            mMessagesToSend.push_back("U:M");
            mMessagesToSend.push_back("U:N");
            mMessagesToSend.push_back("U:O");
            mMessagesToSend.push_back("U:P");
            mMessagesToSend.push_back("U:Q");
        }
        }
        mMessagesToSend.push_back(ENDSTRING);

    }
    static SstTest*createSuite() {
        return new SstTest;
    }
    static void destroySuite(SstTest*sst) {
        delete sst;
    }
    
    ~SstTest() {
        for(std::vector<Stream*>::iterator i=mStreams.begin(),ie=mStreams.end();i!=ie;++i) {
            delete *i;
        }
        mStreams.resize(0);
        mIO.stop();
        /*
        while(mDisconCount.read()<6){
            printf("%d\n",mDisconCount.read());
        }
        */
        delete mThread;
    }
    void simpleConnect(Stream*s, const Address&addy) {
        static int id=-1;
        s->connect(addy,
                  std::tr1::bind(&SstTest::connectionCallback,this,id,_1,_2),
                   std::tr1::bind(&SstTest::connectorNewStreamCallback,this,id,_1,_2),
                   std::tr1::bind(&SstTest::connectorDataRecvCallback,this,s,id,_1));
        --id;
    }
    void testConnectSend (void )
    {
        Stream*z=NULL;
        bool doSubstreams=false;
        {
            TCPStream r(mIO);
            while (!mReadyToConnect);
            simpleConnect(&r,Address("127.0.0.1",mPort));
            runRoutine(&r);

                {
                    Stream*zz=r.factory();        
                    zz->cloneFrom(&r,
                                  std::tr1::bind(&SstTest::connectionCallback,this,-100,_1,_2),
                                  &Stream::ignoreSubstreamCallback,
                                  &Stream::ignoreBytesReceived);
                    runRoutine(zz);
                    zz->close();
                    delete zz;
                }
            if (doSubstreams) {
                z=r.factory();
                z->cloneFrom(&r,
                             std::tr1::bind(&SstTest::connectionCallback,this,-2000000000,_1,_2),
                             std::tr1::bind(&SstTest::connectorNewStreamCallback,this,-2000000000,_1,_2),
                             std::tr1::bind(&SstTest::connectorDataRecvCallback,this,z,-2000000000,_1));
                runRoutine(z);
            }
            //wait until done
            time_t last_time=0;
            while(mCount<(int)(mMessagesToSend.size()*(doSubstreams?5:3))&&!mAbortTest) {
                time_t this_time=time(NULL);
                if (this_time>last_time+5) {
                    std::cerr<<"Message Receive Count == "<<mCount.read()<<'\n';
                    last_time=this_time;
                }
            }
            TS_ASSERT(mAbortTest==false);
            for (std::map<unsigned int,std::vector<Sirikata::Network::Chunk> >::iterator datamapiter=mDataMap.begin();
                 datamapiter!=mDataMap.end();
                 ++datamapiter) {
                validateVector(datamapiter->second,mMessagesToSend);
            }
            r.close();
        }
        if( doSubstreams){
            z->close();
            delete z;
        }
    }
};
