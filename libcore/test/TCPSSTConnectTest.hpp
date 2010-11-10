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

#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
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
#include <time.h>
#undef SILOG
#define SILOG(...)

using namespace Sirikata::Network;
class SstTest : public CxxTest::TestSuite
{
    std::vector<Stream *> dedStreams;
    typedef boost::unique_lock<boost::mutex> unique_mutex_lock;
public:
    void runRoutine(Stream* s) {
        for (unsigned int i=0;i<mMessagesToSend.size();++i) {
            s->send(Chunk(mMessagesToSend[i].begin(),mMessagesToSend[i].end()),
                    mMessagesToSend[i].size()?(mMessagesToSend[i][0]=='U'?ReliableUnordered:(mMessagesToSend[i][0]=='X'?Unreliable:ReliableOrdered)):ReliableOrdered);
        }
    }
    ///this will only be calledback if main connection fails--which means that secondary stream rather than answerer to secondary stream will fail
    void mainStreamConnectionCallback(int id, Stream::ConnectionStatus stat, const std::string&reason) {
        connectionCallback(id,stat,reason);
        if (stat!=Stream::Connected) {
            ++mDisconCount;
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

        mDataMap[id].push_back(connectionReason);
        if (stat!=Stream::Connected){
            ++mDisconCount;
        }

        if (stat == Stream::Disconnected) {
            ++mEndCount;
        }
    }
    void dataRecvCallback(Stream *s,int id, const Chunk&data, const Stream::PauseReceiveCallback& pauseReceive) {
        static bool dopause=false;
        dopause=!dopause;
        if (dopause) {//rand()>RAND_MAX/10) {
            mDataMap[id].push_back(data);
            bool found=false;
            if (data.size()) {
                for (unsigned int i=0;i<mMessagesToSend.size();++i) {

                    if (data.size()==mMessagesToSend[i].size()
                        && memcmp(&*data.begin(),&*mMessagesToSend[i].begin(),mMessagesToSend[i].size())==0) {
                        found=true;
                    }
                }
                TS_ASSERT(found);
                if (!found) {
                    found=false;
                    SILOG(tcpsst,error,"Test failed to find string "/*<<std::string((const char*)&*data.begin(),data.size())*/<<" size "<<data.size());

                }
            }else {
                SILOG(tcpsst,error,"GHOST PACKET (size0)");
            }
            ++mCount;
            return;
        }
        mServicePool->service()->post(Duration::microseconds(100),
            std::tr1::bind(&SstTest::lockReadyRead,this,s));
        pauseReceive();
    }
    void lockReadyRead(Network::Stream*s) {
        unique_mutex_lock lck(mMutex);
        if(std::find(dedStreams.begin(),dedStreams.end(),s)==dedStreams.end())
            s->readyRead();
    }
    void connectorDataRecvCallback(Stream *s,int id, const Chunk&data, const Stream::PauseReceiveCallback& pauseReceive) {
        dataRecvCallback(s,id,data,pauseReceive);
    }
    void listenerDataRecvCallback(Stream *s,int id, const Chunk&data, const Stream::PauseReceiveCallback& pauseReceive) {
        dataRecvCallback(s,id,data,pauseReceive);
    }
    void connectorNewStreamCallback (int id,Stream * newStream, Stream::SetCallbacks& setCallbacks) {
        assert(newStream);

        static int newid=0;
        {
            unique_mutex_lock lck(mMutex);
            mStreams.push_back(newStream);
        }
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        setCallbacks(std::tr1::bind(&SstTest::connectionCallback,this,newid,_1,_2),
            std::tr1::bind(&SstTest::connectorDataRecvCallback,this,newStream,newid,_1,_2),
            &Stream::ignoreReadySendCallback);
        ++newid;
        runRoutine(newStream);
    }
    void listenerNewStreamCallback (int id,Stream * newStream, Stream::SetCallbacks& setCallbacks) {
        if (newStream) {
            static int newid=0;
            {
                unique_mutex_lock lck(mMutex);
                mStreams.push_back(newStream);
            }
            using std::tr1::placeholders::_1;
            using std::tr1::placeholders::_2;
            setCallbacks(std::tr1::bind(&SstTest::connectionCallback,this,newid,_1,_2),
                std::tr1::bind(&SstTest::listenerDataRecvCallback,this,newStream,newid,_1,_2),
                &Stream::ignoreReadySendCallback);
            ++newid;
            runRoutine(newStream);
        }
    }
    std::string mPort;
    IOServicePool* mServicePool;
    IOStrand* mServiceStrand;
    StreamListener* mListener;
    std::vector<Stream*> mStreams;
    std::vector<Chunk> mMessagesToSend;
    Sirikata::AtomicValue<int> mCount;
    Sirikata::AtomicValue<int> mDisconCount;
    Sirikata::AtomicValue<int> mEndCount;
    typedef Sirikata::uint8 uint8;
    std::map<unsigned int, std::vector<Sirikata::Network::Chunk> > mDataMap;
    const char * ENDSTRING;
    volatile bool mAbortTest;
    boost::mutex mMutex;
    void validateSameness(
        int id,
        const std::vector<const Sirikata::Network::Chunk* >&netData,
        const std::vector<const Sirikata::Network::Chunk* >&keyData) {
        size_t i,j;
        for (i=0,j=0;i<netData.size()&&j<keyData.size();) {
            if (netData[i]->size()&&(*netData[i])[0]=='C') {
                ++i;
                continue;
            }
            TS_ASSERT_EQUALS(netData[i]->size(),keyData[j]->size());
            TS_ASSERT_EQUALS(*netData[i],*keyData[j]);
/*
            if (!netData[i]->empty()) {
                TS_ASSERT_SAME_DATA(&*netData[i]->begin(),&*keyData[j]->begin(),netData[i]->size()*sizeof((*netData[i])[0]));
            }
*/
            ++i;
            ++j;
        }
        for (;i<netData.size();++i) {
            if (netData[i]->size()) {
                TS_ASSERT_EQUALS((*netData[i])[0],'C');
            }else {
                TS_FAIL("Stray Empty packets");
            }
        }
        if (id!=-1999999999) {//this is the socket that ignored returning bytes
            TS_ASSERT_EQUALS(j,keyData.size());
        }
    }
    class Comparator {
    public:
        bool operator() (const Sirikata::Network::Chunk*a, const Sirikata::Network::Chunk*b) {
            size_t i=0;
            size_t ilim=a->size();
            if( b->size()<ilim)
                ilim=b->size();
            Sirikata::Network::Chunk::const_iterator aiter=a->begin();
            Sirikata::Network::Chunk::const_iterator biter=b->begin();
            for(;i<ilim;++i,++aiter,++biter) {

                if ((*aiter)<(*biter)) {
                    return true;
                }
                if ((*aiter)>(*biter)) {
                    return false;
                }
            }
            if (a->size()<b->size())
                return true;
            return false;
        }
        bool operator() (const std::string*a, const std::string*b) {
            return *a<*b;
        }
        bool ascmp (const Sirikata::Network::Chunk*a, const Sirikata::Network::Chunk*b) {
            return (*this)(a,b);
        }
        bool acmp (const Sirikata::Network::Chunk&a, const Sirikata::Network::Chunk&b) {
            return (*this)(&a,&b);
        }
    };
    void validateVector(int id, const std::vector<Sirikata::Network::Chunk>&netData,
                        const std::vector<Sirikata::Network::Chunk>&keyData) {
        std::vector<const Sirikata::Network::Chunk* > orderedNetData,orderedKeyData;
        std::vector<const Sirikata::Network::Chunk* > unorderedNetData,unorderedKeyData;
        for (size_t i=0;i<netData.size();++i) {
            if (netData[i].size()==0||netData[i][0]=='T') {
                orderedNetData.push_back(&netData[i]);
            }else {
                unorderedNetData.push_back(&netData[i]);
            }
        }
        for (size_t i=0;i<keyData.size();++i) {
            if (keyData[i].size()==0||keyData[i][0]=='T') {
                orderedKeyData.push_back(&keyData[i]);
            }else {
                unorderedKeyData.push_back(&keyData[i]);
            }
        }
        Comparator c;
        std::sort(unorderedNetData.begin(),unorderedNetData.end(),c);
        std::sort(unorderedKeyData.begin(),unorderedKeyData.end(),c);
        validateSameness(id,unorderedNetData,unorderedKeyData);
        validateSameness(id,orderedNetData,orderedKeyData);
        validateSameness(id,unorderedNetData,unorderedKeyData);
    }
    Chunk ToChunk(const std::string &input) {
        return Chunk(input.begin(),input.end());
    }
    SstTest():mCount(0),mDisconCount(0),mEndCount(0),ENDSTRING("T end"),mAbortTest(false) {
        Sirikata::PluginManager plugins;
        plugins.load( "tcpsst" );

        uint32 randport = 3000 + (uint32)(Sirikata::Task::LocalTime::now().raw() % 20000);
        mPort = boost::lexical_cast<std::string>(randport);

        mServicePool = new IOServicePool(4);
        mServiceStrand = mServicePool->service()->createStrand();

        mListener = StreamListenerFactory::getSingleton().getDefaultConstructor()(mServiceStrand,StreamListenerFactory::getSingleton().getDefaultOptionParser()(String()));
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        mListener->listen(Address("127.0.0.1",mPort),std::tr1::bind(&SstTest::listenerNewStreamCallback,this,0,_1,_2));

        mServicePool->run();

        bool doUnorderedTest=true;
        bool doShortTest=false;

        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:0"));
            mMessagesToSend.push_back(ToChunk("U:1"));
            mMessagesToSend.push_back(ToChunk("U:2"));
        }
        mMessagesToSend.push_back(ToChunk("T0th"));
        mMessagesToSend.push_back(ToChunk("T1st"));
        mMessagesToSend.push_back(ToChunk("T2nd"));
        mMessagesToSend.push_back(ToChunk("T3rd"));
        mMessagesToSend.push_back(ToChunk("T4th"));
        mMessagesToSend.push_back(ToChunk("T5th"));
        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:3"));
            mMessagesToSend.push_back(ToChunk("U:4"));
            mMessagesToSend.push_back(ToChunk("U:5"));
        }
        mMessagesToSend.push_back(ToChunk("T6th"));
        mMessagesToSend.push_back(ToChunk("T7th"));
        mMessagesToSend.push_back(ToChunk("T8th"));
        mMessagesToSend.push_back(ToChunk("T9th"));
        if (!doShortTest) {

        Chunk test;
        test.push_back('T');
        for (unsigned int i=0;i<16385;++i) {
            test.push_back((char)(rand()%256));
        }
        mMessagesToSend.push_back(test);
        if (doUnorderedTest){
        test[0]='U';
        mMessagesToSend.push_back(test);
}
        for (unsigned int i=0;i<256*1024;++i) {
            test.push_back('b');//(char)((rand())%256);
        }
        for (unsigned int i=1;i<test.size();++i) {
            test[i]='b';
        }
        if (doUnorderedTest){
        mMessagesToSend.push_back(test);
}

        test[0]='T';
        mMessagesToSend.push_back(test);

        int pattern[]={32768};
        for (size_t i=0;i<sizeof(pattern)/sizeof(pattern[0]);++i) {
            Chunk pat;
            pat.push_back('T');
            for (int j=0;j<pattern[i];++j) {
                pat.push_back('\0');//'~'-(i%((pattern[i]%90)+3));
            }
            mMessagesToSend.push_back(pat);
            pat[0]='U';
            mMessagesToSend.push_back(pat);
        }

        mMessagesToSend.push_back(ToChunk("T_0th"));
        mMessagesToSend.push_back(ToChunk("T_1st"));
        mMessagesToSend.push_back(ToChunk("T_2nd"));
        mMessagesToSend.push_back(ToChunk("T_3rd"));
        mMessagesToSend.push_back(ToChunk("T_4th"));
        mMessagesToSend.push_back(ToChunk("T_5th"));

        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:6"));
            mMessagesToSend.push_back(ToChunk("U:7"));
            mMessagesToSend.push_back(ToChunk("U:8"));
            mMessagesToSend.push_back(ToChunk("U:9"));
            mMessagesToSend.push_back(ToChunk("U:A"));
        }
        mMessagesToSend.push_back(ToChunk("T_6th"));
        mMessagesToSend.push_back(ToChunk("T_7th"));

        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:B"));
        }
            mMessagesToSend.push_back(ToChunk("T_8th"));
        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:C"));
        }
        mMessagesToSend.push_back(ToChunk("T_9th"));
        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:D"));
        }
        mMessagesToSend.push_back(ToChunk("The green grasshopper fetched."));
        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:E"));
            mMessagesToSend.push_back(ToChunk("U:F"));
            mMessagesToSend.push_back(ToChunk("U:G"));
        }

        mMessagesToSend.push_back(ToChunk("T A blade of grass."));
        mMessagesToSend.push_back(ToChunk("T From the playground ."));
        mMessagesToSend.push_back(ToChunk("T Grounds test test test this is a test test test this is a test test test this is a test test test test and the test is proceeding until it reaches signific length with a string that long however. this is not quite long enough to trigger the high water mark--well now it is I believe to the best of my abilities"));
        mMessagesToSend.push_back(ToChunk("T Grounds test test test this is a test test test this is a test test test this is a test test test test and the test is proceeding until it reaches signific length with a string that long however. this is not quite"));

        if (doUnorderedTest){
            mMessagesToSend.push_back(ToChunk("U:H"));
            mMessagesToSend.push_back(ToChunk("U:I"));
            mMessagesToSend.push_back(ToChunk("U:J"));
            mMessagesToSend.push_back(ToChunk("U:K"));
            mMessagesToSend.push_back(ToChunk("U:L"));
            mMessagesToSend.push_back(ToChunk("U:M"));
            mMessagesToSend.push_back(ToChunk("U:N"));
            mMessagesToSend.push_back(ToChunk("U:O"));
            mMessagesToSend.push_back(ToChunk("U:P"));
            mMessagesToSend.push_back(ToChunk("U:Q"));
        }
        }

        mMessagesToSend.push_back(ToChunk(ENDSTRING));

    }
    static SstTest*createSuite() {
        return new SstTest;
    }
    static void destroySuite(SstTest*sst) {
        delete sst;
    }

    ~SstTest() {
        {
            unique_mutex_lock lck(mMutex);
            for(std::vector<Stream*>::iterator i=mStreams.begin(),ie=mStreams.end();i!=ie;++i) {
                dedStreams.push_back(*i);
                delete *i;
            }
            mStreams.resize(0);
        }
        delete mListener;

        // The other thread should finish up any outstanding handlers and stop
        mServicePool->join();
        delete mServiceStrand;
        delete mServicePool;
    }
    void simpleConnect(Stream*s, const Address&addy) {
        static int id=-1;
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        s->connect(addy,
            std::tr1::bind(&SstTest::connectorNewStreamCallback,this,id,_1,_2),
            std::tr1::bind(&SstTest::mainStreamConnectionCallback,this,id,_1,_2),
            std::tr1::bind(&SstTest::connectorDataRecvCallback,this,s,id,_1,_2),
            &Stream::ignoreReadySendCallback);
        --id;
    }
    void testInt30Serialization(void) {
        Sirikata::vuint32 a(10),b(129),c(257),d(384),e(16300),f(16385),g(32767),h(32768),i(65535),j(131073),guinea;
        uint8 buffer[Sirikata::vuint32::MAX_SERIALIZED_LENGTH];
        unsigned int maxlen=Sirikata::vuint32::MAX_SERIALIZED_LENGTH;
        unsigned int curlen;
        curlen=a.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(a,guinea);

        curlen=b.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(b,guinea);

        curlen=c.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(c,guinea);

        curlen=d.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(d,guinea);

        curlen=e.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(e,guinea);

        curlen=f.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(f,guinea);

        curlen=g.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(g,guinea);

        curlen=h.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(h,guinea);

        curlen=i.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(i,guinea);

        curlen=j.serialize(buffer,maxlen);
        TS_ASSERT(guinea.unserialize(buffer,curlen));
        TS_ASSERT_EQUALS(j,guinea);

        for (unsigned int iter=1;iter<(1<<30);iter*=2) {
            Sirikata::vuint32 vartest(iter);
            curlen=vartest.serialize(buffer,maxlen);
            TS_ASSERT(guinea.unserialize(buffer,curlen));
            TS_ASSERT_EQUALS(vartest,guinea);
            {
                Sirikata::vuint32 vartest(iter+iter-1);
                curlen=vartest.serialize(buffer,maxlen);
                TS_ASSERT(guinea.unserialize(buffer,curlen));
                TS_ASSERT_EQUALS(vartest,guinea);

            }
            {
                Sirikata::vuint32 vartest(iter+iter/2);
                curlen=vartest.serialize(buffer,maxlen);
                TS_ASSERT(guinea.unserialize(buffer,curlen));
                TS_ASSERT_EQUALS(vartest,guinea);

            }
            for (int i=0;i<10;++i) {
                Sirikata::vuint32 vartest(iter+i);
                curlen=vartest.serialize(buffer,maxlen);
                TS_ASSERT(guinea.unserialize(buffer,curlen));
                TS_ASSERT_EQUALS(vartest,guinea);

            }
        }

    }
    static void noopSubstream(Stream *stream, Stream::SetCallbacks&scb) {
        scb(&Stream::ignoreConnectionCallback,&Stream::ignoreReceivedCallback,&Stream::ignoreReadySendCallback);
    }
    void testSubstream(Stream* stream, Stream::SetCallbacks&scb) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        scb(std::tr1::bind(&SstTest::connectionCallback,this,-2000000000,_1,_2),
            std::tr1::bind(&SstTest::connectorDataRecvCallback,this,stream,-2000000000,_1,_2),
            &Stream::ignoreReadySendCallback);
    }
    void testConnectSend (void )
    {
        dedStreams.clear();
        Stream*z=NULL;
        bool doSubstreams=true;
        {
            Stream *r=StreamFactory::getSingleton().getDefaultConstructor()(mServiceStrand,StreamFactory::getSingleton().getDefaultOptionParser()(String()));
            simpleConnect(r,Address("127.0.0.1",mPort));
            runRoutine(r);
            if (doSubstreams) {

                if (true){
                    Stream*zz=r->factory();
                    zz = r->clone(&SstTest::noopSubstream);
                    if (zz) {
                        runRoutine(zz);
                        zz->close();
                    }
                    delete zz;
                }
                using std::tr1::placeholders::_1;
                using std::tr1::placeholders::_2;
                z=r->clone(std::tr1::bind(&SstTest::testSubstream,this,_1,_2));
                if (z) {
                    runRoutine(z);
                }else {
                    ++mDisconCount;
                }
            }
            //wait until done
            time_t last_time=time(NULL);
            int retry_count=200;
            while(mCount<(int)(mMessagesToSend.size()*(doSubstreams?5:2))&&!mAbortTest) {
                if (0&&rand()<RAND_MAX/10) {
                    if (r)
                        r->readyRead();
                    if(z)
                        z->readyRead();
                    {
                        unique_mutex_lock lck(mMutex);
                        for(std::vector<Stream*>::iterator i=mStreams.begin(),ie=mStreams.end();i!=ie;++i) {
                            (*i)->readyRead();
                        }
                    }
                }

                time_t this_time=time(NULL);
                if (this_time>last_time+50) {
                    std::cerr<<"Message Receive Count == "<<mCount.read()<<'\n';
                    last_time=this_time;
                    if (--retry_count<=0) {
                        TS_FAIL("Timeout  in receiving messages");
                        TS_ASSERT_EQUALS(mCount.read(),(int)(mMessagesToSend.size()*(doSubstreams?5:2)));
                        break;
                    }
                }
            }
            TS_ASSERT(mAbortTest==false);
            for (std::map<unsigned int,std::vector<Sirikata::Network::Chunk> >::iterator datamapiter=mDataMap.begin();
                 datamapiter!=mDataMap.end();
                 ++datamapiter) {
                validateVector(datamapiter->first,datamapiter->second,mMessagesToSend);
            }
            r->close();
            {
                unique_mutex_lock lck(mMutex);
                dedStreams.push_back(r);
                delete r;
            }
        }
        if( doSubstreams){
            z->close();
            time_t last_time=time(NULL);
            int retry_count=8;
            while(mDisconCount.read()<2){
                if (0&&rand()<RAND_MAX/10) {
                    z->readyRead();
                    {
                        unique_mutex_lock lck(mMutex);
                        for (ptrdiff_t i=((ptrdiff_t)mStreams.size())-1;i>=0;--i) {
                            mStreams[i]->readyRead();
                        }
                    }
                }

                time_t this_time=time(NULL);
                if (this_time>last_time+500) {
                    std::cerr<<"Close Receive Count == "<<mDisconCount.read()<<'\n';
                    last_time=this_time;
                    if (--retry_count<=0) {
                        TS_FAIL("Timeout  in receiving close signals");
                        TS_ASSERT_EQUALS(mDisconCount.read(),2);
                        break;
                    }
                }
            }
            unique_mutex_lock lck(mMutex);
            {
                dedStreams.push_back(z);
                delete z;
            }
        }
        time_t last_time=time(NULL);
        int retry_count=3;
        while(mEndCount.read()<1){//checking for that final call to newSubstream
            time_t this_time=time(NULL);
            if (0&&rand()<RAND_MAX/10) {
                {
                    unique_mutex_lock lck(mMutex);
                    for (ptrdiff_t i=((ptrdiff_t)mStreams.size())-1;i>=0;--i) {
                        mStreams[i]->readyRead();
                    }
                }
            }
            if (this_time>last_time+500) {
                std::cerr<<"SubStream End Receive Count == "<<mEndCount.read()<<'\n';
                last_time=this_time;
                if (--retry_count<=0) {
                    TS_FAIL("Timeout  in receiving newSubstream functor deallocation request");
                    TS_ASSERT_EQUALS(mEndCount.read(),1);
                    break;
                }
            }
        }
    }
};
