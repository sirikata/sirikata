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

#include "util/Standard.hh"

#include "network/TCPDefinitions.hpp"
#include "network/TCPStream.hpp"
#include "network/TCPStreamListener.hpp"
#include <cxxtest/TestSuite.h>
#include <boost/thread.hpp>
using namespace Sirikata::Network;
class SstTest : public CxxTest::TestSuite
{
public:
    void runRoutine(Stream* s) {
        for (unsigned int i=0;i<mMessagesToSend.size();++i) {
            s->send(Chunk(mMessagesToSend[i].begin(),mMessagesToSend[i].end()),
                    mMessagesToSend[i].size()?(mMessagesToSend[i][0]=='U'?Stream::ReliableUnordered:(mMessagesToSend[i][0]=='X'?Stream::Unreliable:Stream::Reliable)):Stream::Reliable);
        }
    }
    void connectionCallback(int id, Stream::ConnectionStatus stat, const std::string&reason) {
        mDataMap[id].push_back(Chunk(reason.begin(),reason.end()));
    }
    void dataRecvCallback(Stream *s,int id, const Chunk&data) {
        mDataMap[id].push_back(data);
        ++mCount;
    }
    void newStreamCallback (int id,Stream * newStream, Stream::SetCallbacks& setCallbacks) {
        static int newid=0;
        mStreams.push_back(newStream);
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        setCallbacks(std::tr1::bind(&SstTest::connectionCallback,this,newid,_1,_2),
                     std::tr1::bind(&SstTest::newStreamCallback,this,newid,_1,_2),
                     std::tr1::bind(&SstTest::dataRecvCallback,this,newStream,newid,_1));
        ++newid;
        runRoutine(newStream);
    }
    void awesomeThread(){
        TCPStreamListener s(mIO);
        s.listen(Address("127.0.0.1",mPort),boost::bind(&SstTest::newStreamCallback,this,0,_1,_2));
        mIO.run();
    }
    std::string mPort;
    boost::asio::io_service mIO;
    boost::thread *mThread;
    std::vector<Stream*> mStreams;
    std::vector<std::string> mMessagesToSend;
    Sirikata::AtomicValue<int> mCount;
    std::map<unsigned int, std::vector<Sirikata::Network::Chunk> > mDataMap;
    SstTest():mCount(0){
        mPort="9142";
        mThread= new boost::thread(boost::bind(&SstTest::awesomeThread,this));
        mMessagesToSend.push_back("U:0");
        mMessagesToSend.push_back("U:1");
        mMessagesToSend.push_back("U:2");
        mMessagesToSend.push_back("U:3");
        mMessagesToSend.push_back("U:4");
        mMessagesToSend.push_back("U:5");
        mMessagesToSend.push_back("U:6");
        mMessagesToSend.push_back("U:7");
        mMessagesToSend.push_back("U:8");
        mMessagesToSend.push_back("U:9");
        mMessagesToSend.push_back("T0th");
        mMessagesToSend.push_back("T1st");
        mMessagesToSend.push_back("T2nd");
        mMessagesToSend.push_back("T3rd");
        mMessagesToSend.push_back("T4th");
        mMessagesToSend.push_back("T5th");
        mMessagesToSend.push_back("T6th");
        mMessagesToSend.push_back("T7th");
        mMessagesToSend.push_back("T8th");
        mMessagesToSend.push_back("T9th");
        std::string test("T");
        for (unsigned int i=0;i<16385;++i) {
            test+=(char)((i+5)%128);
        }
        mMessagesToSend.push_back(test);
        for (unsigned int i=0;i<4096*4096;++i) {
            test+=(char)((rand())%256);
        }
        mMessagesToSend.push_back(test);
        mMessagesToSend.push_back("T_0th");
        mMessagesToSend.push_back("T_1st");
        mMessagesToSend.push_back("T_2nd");
        mMessagesToSend.push_back("T_3rd");
        mMessagesToSend.push_back("T_4th");
        mMessagesToSend.push_back("T_5th");
        mMessagesToSend.push_back("T_6th");
        mMessagesToSend.push_back("T_7th");
        mMessagesToSend.push_back("T_8th");
        mMessagesToSend.push_back("T_9th");
        mMessagesToSend.push_back("The green grasshopper fetched.");
        mMessagesToSend.push_back("T A blade of grass.");
        mMessagesToSend.push_back("T From the playground .");
        mMessagesToSend.push_back("T Grounds test test test this is a test test test this is a test test test this is a test test test test and the test is proceeding until it reaches signific length with a string that long however. this is not quite long enough to trigger the high water mark--well now it is I believe to the best of my abilities");
        mMessagesToSend.push_back("T Grounds test test test this is a test test test this is a test test test this is a test test test test and the test is proceeding until it reaches signific length with a string that long however. this is not quite");

    }
    ~SstTest() {
        delete mThread;
        for(std::vector<Stream*>::iterator i=mStreams.begin(),ie=mStreams.end();i!=ie;++i) {
            delete *i;
        }
    }
    void simpleConnect(Stream*s, const Address&addy) {
        static int id=-1;
        s->connect(addy,
                  std::tr1::bind(&SstTest::connectionCallback,this,id,_1,_2),
                   std::tr1::bind(&SstTest::newStreamCallback,this,id,_1,_2),
                   std::tr1::bind(&SstTest::dataRecvCallback,this,s,id,_1));
        --id;
    }
    void testConnectSend (void )
    {
        TCPStream r(mIO);
        simpleConnect(&r,Address("127.0.0.1",mPort));
        runRoutine(&r);
        Stream*z=r.factory();
        z->cloneFrom(&r,
                     std::tr1::bind(&SstTest::connectionCallback,this,-100,_1,_2),
                     std::tr1::bind(&SstTest::newStreamCallback,this,-100,_1,_2),
                     std::tr1::bind(&SstTest::dataRecvCallback,this,z,-100,_1));
        runRoutine(z);
        delete z;

    }
};
