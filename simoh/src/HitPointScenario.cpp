/*  Sirikata
 *  HitPointScenario.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include "HitPointScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include "ConnectedObjectTracker.hpp"
#include <sirikata/core/util/RegionWeightCalculator.hpp>

namespace Sirikata {


char tohexdig(uint8 inp) {
    if (inp<=9) return inp+'0';
    return (inp-10)+'A';
}
String makeHexString(uint8*buffer, int length) {
    String retval;
    for (int i=0;i<length;++i) {
        uint8 dat = buffer[i];
        uint8 nibble0 = buffer[i]%16;
        uint8 nibble1=  buffer[i]/16%16;
        retval.push_back(tohexdig(nibble0));
        retval.push_back(tohexdig(nibble1));
        retval.push_back(' ');
    }
    return retval;
}

class DamagableObject {
public:
    Object *object;
    class HPTYPE {
        double hp;
        unsigned char magicNumber[8];
    public:
        bool operator == (const HPTYPE & other) const {
            return hp==other.hp&&memcmp(magicNumber,other.magicNumber,8)==0;
        }
        double read()const {
            return hp;
        }
        HPTYPE(double hp) {
            static char mag[8]={10,251,229,199,24,50,200,178};
            memcpy(magicNumber,mag,8);
            this->hp=hp;
        }
    };
    HPTYPE mHP;
    unsigned int mListenPort;
    DamagableObject (Object *obj, HPTYPE hp, unsigned int listenPort):mHP(hp){
        object=obj;
        mListenPort = listenPort;
    }
    void update() {
        for (size_t i=0;i<mDamageReceivers.size();++i) {
            mDamageReceivers[i]->sendUpdate();
        }
    }
    class ReceiveDamage {
        UUID mID;
        DamagableObject *mParent;
        boost::shared_ptr<Stream<UUID> > mSendStream;
        boost::shared_ptr<Stream<UUID> > mReceiveStream;

        uint8 mPartialUpdate[sizeof(HPTYPE)];
        int mPartialCount;

        uint8 mPartialSend[sizeof(HPTYPE)*2];
        int mPartialSendCount;
        Object * mObj;
        HitPointScenario *mContext;
    public:
        ReceiveDamage(DamagableObject*parent, HitPointScenario *ctx, Object * obj, UUID id) {
            mContext = ctx;
            mObj=obj;
            mID=id;
            mParent=parent;
            mPartialCount=0;
            mPartialSendCount=0;
            memset(mPartialSend,0,sizeof(DamagableObject::HPTYPE)*2);
            memset(mPartialUpdate,0,sizeof(DamagableObject::HPTYPE));
            static int offset=0;
            ++offset;
            int port = parent->mListenPort+offset;
            SILOG(hitpoint,error,"Listen/Connecting "<<mID.toString()<<" to "<<mParent->object->uuid().toString());
            Stream<UUID>::listen(std::tr1::bind(&DamagableObject::ReceiveDamage::login,this,_1,_2),
                                 EndPoint<UUID>(mID,parent->mListenPort));
            Stream<UUID>::connectStream(mParent->object,
                                        EndPoint<UUID>(mParent->object->uuid(),port),
                                        EndPoint<UUID>(mID,parent->mListenPort),
                                        std::tr1::bind(&DamagableObject::ReceiveDamage::connectionCallback,this,port,_1,_2));

        }
        void connectionCallback(int port, int err, boost::shared_ptr<Stream<UUID> > s) {
            if (err != 0 ) {
                SILOG(hitpoint,error,"Failed to connect two objects...Retry"<<mParent->object->uuid().toString()<<" - "<<mID.toString());
                Stream<UUID>::connectStream(mParent->object,
                                            EndPoint<UUID>(mParent->object->uuid(),port),
                                            EndPoint<UUID>(mID,mParent->mListenPort),
                                            std::tr1::bind(&DamagableObject::ReceiveDamage::connectionCallback,this,port,_1,_2));

            }else {
                SILOG(hitpoint,error,"Got connection "<<mParent->object->uuid().toString()<<" - "<<mID.toString());
                this->mSendStream = s;
                s->registerReadCallback( std::tr1::bind( &DamagableObject::ReceiveDamage::senderShouldNotGetUpdate, this, _1, _2)  ) ;
            }
        }
        void login(int err, boost::shared_ptr< Stream<UUID> > s) {
            if (err != 0) {
                SILOG(hitpoint,error,"Failed to listen on port for two objects\n");
            }else {
                s->registerReadCallback( std::tr1::bind( &DamagableObject::ReceiveDamage::getUpdate, this, _1, _2)  ) ;
                mReceiveStream=s;
            }
        }
        void sendUpdate() {
            if (mSendStream) {
                int tosend=sizeof(DamagableObject::HPTYPE);
                if (mPartialSendCount) {
                    memcpy(mPartialSend+sizeof(DamagableObject::HPTYPE),&mParent->mHP,sizeof(DamagableObject::HPTYPE));
                    tosend*=2;
                    tosend-=mPartialSendCount;
                }else {
                    memcpy(mPartialSend,&mParent->mHP,tosend);
                }
                //SILOG(hitpoint,error,"Sent buffer "<<makeHexString(mPartialSend+mPartialSendCount,tosend)<<" AT TIME ");
                mPartialSendCount+=mSendStream->write(mPartialSend+mPartialSendCount,tosend);
                if(mPartialSendCount>=(int)sizeof(DamagableObject::HPTYPE)) {
                    mPartialSendCount-=sizeof(DamagableObject::HPTYPE);
                    memcpy(mPartialSend,mPartialSend+sizeof(DamagableObject::HPTYPE),sizeof(DamagableObject::HPTYPE));
                }
                if(mPartialSendCount>=(int)sizeof(DamagableObject::HPTYPE)) {
                    mPartialSendCount=0;
                }
                assert(mPartialSendCount<(int)sizeof(DamagableObject::HPTYPE));
            }
        }
        void senderShouldNotGetUpdate(uint8*buffer, int length) {
            SILOG(hitpoint,error,"Should not receive updates from receiver");
        }
        void getUpdate(uint8*buffer, int length) {
            //SILOG(hitpoint,error,"Received buffer "<<makeHexString(buffer,length));
            while (length>0) {

                int datacopied = (length>(int)sizeof(DamagableObject::HPTYPE)-mPartialCount?sizeof(DamagableObject::HPTYPE)-mPartialCount:length);
                memcpy(mPartialUpdate+mPartialCount, buffer,datacopied);
                mPartialCount+=datacopied;
                if (mPartialCount==sizeof(DamagableObject::HPTYPE)) {
                    DamagableObject::HPTYPE hp(0.0);
                    memcpy(&hp,mPartialUpdate,mPartialCount);
                    DamagableObject::HPTYPE test(hp.read());
                    if (rand()%10==0)
                        SILOG(hitpoint,error,"Got hitpoint update for "<<mID.toString()<<" as "<<hp.read()<<" Delay "<<(hp.read()-mContext->calcHp())<<" distance "<<(mObj->location().position()-mParent->object->location().position()).length());
                    assert(hp==test);

                    mPartialCount=0;
                }
                buffer+=datacopied;
                length-=datacopied;
            }
        }
    };
    std::vector<ReceiveDamage*> mDamageReceivers;
    friend class ReceiveDamage;
};

void DPSInitOptions(HitPointScenario *thus) {

    Sirikata::InitializeClassOptions ico("HitPointScenario",thus,
        new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("num-hp-per-second","100",Sirikata::OptionValueType<double>(),"Number of hp updaets performed per simulation second"),
        new OptionValue("prob-messages-uniform","1",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("num-objects-per-server","1000",Sirikata::OptionValueType<uint32>(),"The number of objects that should be connected before the pinging begins"),
        new OptionValue("ping-size","8",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("ping-big-chance",".5",Sirikata::OptionValueType<float>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("flood-server","1",Sirikata::OptionValueType<uint32>(),"The index of the server to flood.  Defaults to 1 so it will work with all layouts. To flood all servers, specify 0."),
        new OptionValue("source-flood-server","false",Sirikata::OptionValueType<bool>(),"This makes the flood server the source of all the packets rather than the destination, so that we can validate that egress routing gets proper fairness."),
        new OptionValue("local","false",Sirikata::OptionValueType<bool>(),"If true, generated traffic will all be local, i.e. will all originate at the flood-server.  Otherwise, it will always originate from other servers."),
        new OptionValue("receivers-per-server","3",Sirikata::OptionValueType<int>(),"The number of folks listening for HP updates at each server"),
        NULL);
}

HitPointScenario::HitPointScenario(const String &options)
 : mStartTime(Time::epoch())
{
    mNumTotalPings=0;
    mContext=NULL;
    mObjectTracker = NULL;
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("HitPointScenario",this);
    optionsSet->parse(options);

    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<double>();
    mPingPayloadSize=optionsSet->referenceOption("ping-size")->as<uint32>();
    mPingBigChance=optionsSet->referenceOption("ping-big-chance")->as<float>();
    mNumHitPointsPerSecond=optionsSet->referenceOption("num-hp-per-second")->as<double>();
    mFloodServer = optionsSet->referenceOption("flood-server")->as<uint32>();
    mSourceFloodServer = optionsSet->referenceOption("source-flood-server")->as<bool>();
    mNumObjectsPerServer=optionsSet->referenceOption("num-objects-per-server")->as<uint32>();
    mLocalTraffic = optionsSet->referenceOption("local")->as<bool>();
    mFractionMessagesUniform= optionsSet->referenceOption("prob-messages-uniform")->as<double>();
    mNumGeneratedPings = 0;
    mGeneratePingsStrand = NULL;
    mGeneratePingPoller = NULL;
    mPings = // We allocate space for 1/4 seconds worth of pings
        new Sirikata::SizedThreadSafeQueue<PingInfo,CountResourceMonitor>(
            CountResourceMonitor(std::max((uint32)(mNumPingsPerSecond / 4), (uint32)2))
        );
    mWeightCalculator =
        RegionWeightCalculatorFactory::getSingleton().getConstructor(GetOptionValue<String>(OPT_REGION_WEIGHT))(GetOptionValue<String>(OPT_REGION_WEIGHT_ARGS))
        ;
    mPingPoller = NULL;
    // NOTE: We have this limit because we can get in lock-step with the
    // generator, causing this to run for excessively long when we fall behind
    // on pings.  This allows us to burst to catch up when there is time (and
    // amortize the constant overhead of doing 1 round), but if
    // there is other work to be done we won't make our target rate.
    mMaxPingsPerRound = 40;
    // Do we want to vary this based on mNumPingsPerSecond? 20 is a decent
    // burst, but at 10k/s can take 2ms (we're seeing about 100us/ping
    // currently), which is potentially a long delay for other things in this
    // strand.  If we try to get to 100k, that can be up to 20ms which is very
    // long to block other things on the main strand.
}
HitPointScenario::~HitPointScenario(){
    SILOG(deluge,fatal,
        "HitPoint: Generated: " << mNumGeneratedPings <<
        " Sent: " << mNumTotalPings);
    delete mPings;
    delete mPingPoller;
    delete mPingProfiler;
}

HitPointScenario*HitPointScenario::create(const String&options){
    return new HitPointScenario(options);
}
void HitPointScenario::addConstructorToFactory(ScenarioFactory*thus){
    thus->registerConstructor("hitpoint",&HitPointScenario::create);
}

void HitPointScenario::initialize(ObjectHostContext*ctx) {
    mGenPhase=GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
    mContext=ctx;
    mObjectTracker = new ConnectedObjectTracker(mContext->objectHost);

    mPingProfiler = mContext->profiler->addStage("Object Host Send Pings");
    mPingPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&HitPointScenario::sendPings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );

    mHPPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&HitPointScenario::sendHPs, this),
        Duration::seconds(1./mNumHitPointsPerSecond));

    mGeneratePingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
    mGeneratePingsStrand = mContext->ioService->createStrand();
    mGeneratePingPoller = new Poller(
        mGeneratePingsStrand,
        std::tr1::bind(&HitPointScenario::generatePings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );
}

void HitPointScenario::start() {
    Duration connect_phase = GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
    mContext->mainStrand->post(
        connect_phase,
        std::tr1::bind(&HitPointScenario::delayedStart, this)
    );
}
void HitPointScenario::delayedStart() {
    mStartTime = mContext->simTime();
    OptionSet* optionsSet = OptionSet::getOptions("HitPointScenario",this);
    int receiversPerServer = optionsSet->referenceOption("receivers-per-server")->as<int>();
    int ss=mFloodServer;
    if (mFloodServer ==0) {
        ss=(rand() % mObjectTracker->numServerIDs())+1;
    }
    Object * objA = mObjectTracker->randomObjectFromServer(ss);
    if (objA) {
        static int a =14150;
        mDamagableObjects.push_back(new DamagableObject(objA,1000, a++));
        for (int i=1;i<=(int)mObjectTracker->numServerIDs();++i) {
            std::set<Object* > receivers;
            for (int j=0;j<receiversPerServer;++j) {
                Object * objB = mObjectTracker->randomObjectFromServer(i);
                if (objB&&receivers.find(objB)==receivers.end()) {
                    receivers.insert(objB);
                    mDamagableObjects.back()->mDamageReceivers.push_back(new DamagableObject::ReceiveDamage(mDamagableObjects.back(),
                                                                                                            this,
                                                                                                            objB,
                                                                                                            objB->uuid()));
                }
            }
        }
        mGeneratePingPoller->start();
        mHPPoller->start();
        mPingPoller->start();
    }else {
        Duration connect_phase = GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
        connect_phase=connect_phase/16.0;
        SILOG(oh, debug, "error during connect phase, retrying "<<connect_phase<<" later");
        mContext->mainStrand->post(
            connect_phase,
            std::tr1::bind(&HitPointScenario::delayedStart, this)
            );

    }

}
void HitPointScenario::stop() {
    mPingPoller->stop();
    mGeneratePingPoller->stop();
}
#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)
void HitPointScenario::generatePairs() {

    if (mSendCDF.empty()) {
        std::vector<Object*> floodedObjects;
        Time t=mContext->simTime();

        for (int i=0;i<(int)mObjectTracker->numServerIDs();++i) {
            if (mObjectTracker->numObjectsConnected(mObjectTracker->getServerID(i))<mNumObjectsPerServer) {
                return;
            }
        }
        OH_LOG(debug, "Beginning object seed phase at " << (t-mStartTime)<<"\n");
        Object* first=mObjectTracker->roundRobinObject(mFloodServer);
        if (!first) {
            assert(0);
            return;
        }
        Object* cur=first;
        do {
            floodedObjects.push_back(cur);
            cur=mObjectTracker->roundRobinObject(mFloodServer);
        }while(cur!=first);
        for (size_t i=0;i<mObjectTracker->numServerIDs();++i) {
            ServerID sid=mObjectTracker->getServerID(i);
            if (sid==mFloodServer) continue;
            Object* first=mObjectTracker->roundRobinObject(sid);
            if (!first) {
                assert(0);
                return;
            }
            Object* cur=first;
            do {
                //generate message from/to cur to a floodedObject
                Object* dest=floodedObjects[rand()%floodedObjects.size()];
                assert(dest);
                Object* src=cur=mObjectTracker->roundRobinObject(sid);
                assert(src);
                if (mSourceFloodServer) {
                    std::swap(src,dest);
                }
                MessageFlow pi;
                pi.dest=dest->uuid();
                pi.source=src->uuid();
                pi.dist=(src->location().position(t)-dest->location().position(t)).length();
/*
                printf ("Sending data from object size %f to object size %f, %f meters away\n",
                        src->bounds().radius(),
                        dest->bounds().radius(),
                        pi.dist);
*/
                BoundingBox3f srcbb(src->location().position(t),src->bounds().radius());
                BoundingBox3f dstbb(dest->location().position(t),dest->bounds().radius());
                pi.cumulativeProbability= this->mWeightCalculator->weight(srcbb,dstbb);
                mSendCDF.push_back(pi);

            }while(cur!=first);
        }
        std::sort(mSendCDF.begin(),mSendCDF.end());
        double cumulative=0;
        for (size_t i=0;i<mSendCDF.size();++i) {
            cumulative+=mSendCDF[i].cumulativeProbability;
            mSendCDF[i].cumulativeProbability=cumulative;
        }

        for (size_t i=0;i<mSendCDF.size();++i) {
            mSendCDF[i].cumulativeProbability/=cumulative;
        }
    }

}
static unsigned int even=0;
bool HitPointScenario::generateOnePing(const Time& t, PingInfo* result) {
    generatePairs();
    double which =(rand()/(double)RAND_MAX);
    if (!mSendCDF.empty()) {
        FlowCDF::iterator where;
        {
            MessageFlow comparator;
            comparator.cumulativeProbability=which;
            if (rand()<mFractionMessagesUniform*RAND_MAX)
                where=mSendCDF.begin()+which*(mSendCDF.size()-1);
            else
                where=std::lower_bound(mSendCDF.begin(),mSendCDF.end(),comparator);
        }
        if (where==mSendCDF.end()) {
            --where;
        }
        result->objB = where->source;
        result->objA = where->dest;
        result->dist = where->dist;
        result->ping = new Sirikata::Protocol::Object::Ping();
        mContext->objectHost->fillPing(result->dist, (rand()<mPingBigChance*RAND_MAX?1024:mPingPayloadSize), result->ping);
        return true;
    }

    return false;
}

void HitPointScenario::generatePings() {
    mGeneratePingProfiler->started();

    Time t=mContext->simTime();
    int64 howManyPings=((t-mStartTime).toSeconds()+0.25)*mNumPingsPerSecond;

    bool broke=false;
    int64 limit=howManyPings-mNumGeneratedPings;
    int64 i;
    for (i=0;i<limit;++i) {
        PingInfo result;
        if (!mPings->probablyCanPush(result))
            break;
        if (!generateOnePing(t, &result))
            break;
        if (!mPings->push(result, false))
            break;
    }
    mNumGeneratedPings += i;

    mGeneratePingProfiler->finished();
}

double HitPointScenario::calcHp(const Time*t) {
    if (t==NULL) {
        return 1000-((mContext->simTime()-mStartTime).toSeconds());
    }
    return 1000-((*t-mStartTime).toSeconds());
}

void HitPointScenario::sendHPs() {
    {
        Time t(mContext->simTime());
        DamagableObject::HPTYPE hp = calcHp(&t);
        for (size_t index=0;index<mDamagableObjects.size();++index) {
            mDamagableObjects[index]->mHP=hp;
            mDamagableObjects[index]->update();
        }
    }
}

void HitPointScenario::sendPings() {
    mPingProfiler->started();

    Time newTime=mContext->simTime();
    int64 howManyPings = (newTime-mStartTime).toSeconds()*mNumPingsPerSecond;

    bool broke=false;
    // NOTE: We limit here because we can get in lock-step with the generator,
    // causing this to run for excessively long when we fall behind on pings.
    int64 limit = std::min((int64)howManyPings-mNumTotalPings, mMaxPingsPerRound);
    int64 i;
    for (i=0;i<limit;++i) {
        Time t(mContext->simTime());
        PingInfo result;
        if (!mPings->pop(result)) {
            SILOG(oh,insane,"[OH] " << "Ping queue underflowed.");
            break;
        }
        bool sent_success = mContext->objectHost->sendPing(t, result.objA, result.objB, result.ping);
        delete result.ping;
        if (!sent_success)
            break;
    }
    mNumTotalPings += i;

    mPingProfiler->finished();

    static bool printed=false;
    if ( (i-limit) > (10*(int64)mNumPingsPerSecond) && !printed) {
        SILOG(oh,debug,"[OH] " << i-limit<<" pending ");
        printed=true;
    }

}

}
