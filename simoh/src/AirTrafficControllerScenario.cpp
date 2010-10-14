/*  Sirikata
 *  AirTrafficControllerScenario.cpp
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

#include "AirTrafficControllerScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include "ConnectedObjectTracker.hpp"
#include <sirikata/core/util/RegionWeightCalculator.hpp>
#include "ObjectFactory.hpp"
namespace Sirikata {
namespace AIRTRAFFIC {


static char tohexdig(uint8 inp) {
    if (inp<=9) return inp+'0';
    return (inp-10)+'A';
}
static String makeHexString(uint8*buffer, int length) {
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
        unsigned char otherData[8];
        bool operator == (const HPTYPE & other) const {
            return hp==other.hp&&memcmp(magicNumber,other.magicNumber,8)==0;
        }
        double read()const {
            return hp;
        }
        HPTYPE(double hp) {
            static char mag[8]={10,251,229,199,24,50,200,178};
            memcpy(magicNumber,mag,8);
            memset(otherData,0,sizeof(otherData));
            this->hp=hp;
        }
    };
    HPTYPE mHP;
    unsigned int mHPSize;
    unsigned int mListenPort;
    class ReceiveDamage;
    typedef std::tr1::unordered_map<UUID,ReceiveDamage*,UUID::Hasher> DamageReceiverMap;
    DamageReceiverMap::iterator curIter;
    bool invalidIterator;
    DamagableObject (Object *obj, HPTYPE hp, unsigned int listenPort, unsigned int hpsize):mHP(hp){
        mHPSize=hpsize;
        invalidIterator=true;
        object=obj;
        mListenPort = listenPort;
        curIter = mDamageReceivers.end();
    }
    void update() {
        if (invalidIterator||curIter==mDamageReceivers.end()) {
            curIter=mDamageReceivers.begin();
        }else {
            ++curIter;
            if (curIter==mDamageReceivers.end()) {
                curIter=mDamageReceivers.begin();
            }
        }
        if (curIter!=mDamageReceivers.end()) {
            assert(curIter->second);
            if (curIter->second)
                curIter->second->sendUpdate();
        }
        invalidIterator=false;
    }
    class ReceiveDamage {
        UUID mID;
        DamagableObject *mParent;


        std::string mPartialSend;
        Object * mObj;
        AirTrafficControllerScenario *mContext;
    public:
        double distance;
        std::vector<std::pair <Time,HPTYPE> > samples;
        ReceiveDamage(DamagableObject*parent, AirTrafficControllerScenario *ctx, Object * obj, UUID id) {
            mContext = ctx;
            mObj=obj;
            mID=id;
            mParent=parent;
            distance=0;




            SILOG(hitpoint,error,"Listen/Connecting "<<mID.toString()<<" to "<<mParent->object->uuid().toString());

        }
        void sendUpdate() {
            int tosend=sizeof(DamagableObject::HPTYPE);
            if (tosend<(int)mParent->mHPSize) {
                tosend=mParent->mHPSize;
            }
            mPartialSend.resize(tosend);
            char temp[sizeof(DamagableObject::HPTYPE)+1];
            memcpy(temp,&mParent->mHP,sizeof(DamagableObject::HPTYPE));
            for (int i=0;i<(int)sizeof(DamagableObject::HPTYPE);++i) {
                mPartialSend[i]=temp[i];
            }
            bool ok = mParent->object->send(mParent->mListenPort,mObj->uuid(),mParent->mListenPort,mPartialSend);
            static bool first=true;
            if (first) {
                if (ok) {
                    first=false;
                    SILOG(oh,error,"SENDING to "<<mParent->mListenPort);
                }else {
                    SILOG(oh,error,"not sending to "<<mParent->mListenPort);
                }
            }
        }
        void getUpdate(const uint8*buffer, int length);
    };
    DamageReceiverMap mDamageReceivers;
    friend class ReceiveDamage;
};
DamagableObject::HPTYPE calcHp(AirTrafficControllerScenario* thus, const Time*t) {
    Time tim((t==NULL)?thus->mContext->simTime():*t);
    DamagableObject::HPTYPE retval(1000-((tim-thus->mStartTime).toSeconds()));
    uint64 timTime = tim.raw();
    memcpy(retval.otherData,&timTime,sizeof(uint64));
    return retval;
}
void DamagableObject::ReceiveDamage::getUpdate(const uint8*buffer, int length) {
    if (length>=(int)sizeof(DamagableObject::HPTYPE)) {
        HPTYPE hp(0.0);
        memcpy(&hp,buffer,sizeof(DamagableObject::HPTYPE));
        Time sampTime (mContext->mContext->simTime());
        samples.push_back(std::pair<Time,HPTYPE>(sampTime,hp));
        if (rand()%1000==0) {
            printf("Update: delay %f\n",hp.read()-calcHp(mContext,NULL).read());
        }
        distance=(mObj->location().position()-mParent->object->location().position()).length();
        int64 raw;
        memcpy(&raw,hp.otherData,sizeof(int64));
        Time sentTime(Time::microseconds(raw));
        ObjectHostContext *mContext=this->mContext->mContext;
        CONTEXT_OHTRACE_NO_TIME(hitpoint,
                                sentTime,
                                mObj->uuid(),
                                sampTime,
                                mParent->object->uuid(),
                                hp.read(),
                                calcHp(this->mContext,&sampTime).read(),
                                distance,
                                mObj->bounds().radius(),
                                mParent->object->bounds().radius(),
                                length);

    }
}

void DPSInitOptions(AirTrafficControllerScenario *thus) {

    Sirikata::InitializeClassOptions ico("AirTrafficControllerScenario",thus,
        new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("num-hp-per-second","100",Sirikata::OptionValueType<double>(),"Number of hp updaets performed per simulation second"),
        new OptionValue("prob-messages-uniform","1",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("num-objects-per-server","1000",Sirikata::OptionValueType<uint32>(),"The number of objects that should be connected before the pinging begins"),
        new OptionValue("num-flows-per-pair","1",Sirikata::OptionValueType<uint32>(),"The number of objects flows per source/dest objects (one obj may talk to more than one)"),
        new OptionValue("ping-size","1024",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("hp-size","32",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("flood-server","1",Sirikata::OptionValueType<uint32>(),"The index of the server to flood.  Defaults to 1 so it will work with all layouts. To flood all servers, specify 0."),
        new OptionValue("source-flood-server","true",Sirikata::OptionValueType<bool>(),"This makes the flood server the source of all the packets rather than the destination, so that we can validate that egress routing gets proper fairness."),
        new OptionValue("local","false",Sirikata::OptionValueType<bool>(),"If true, generated traffic will all be local, i.e. will all originate at the flood-server.  Otherwise, it will always originate from other servers."),
        new OptionValue("num-receivers","24",Sirikata::OptionValueType<int>(),"The number of folks listening for HP updates at each server"),
        NULL);
}
}
using namespace AIRTRAFFIC;
AirTrafficControllerScenario::AirTrafficControllerScenario(const String &options)
 : mStartTime(Time::epoch())
{
    mPort=14050;
    mNumTotalPings=0;
    mNumTotalHPs=0;
    mContext=NULL;
    mObjectTracker = NULL;
    mPingID=0;
    DPSInitOptions(this);

    OptionSet* optionsSet = OptionSet::getOptions("AirTrafficControllerScenario",this);
    optionsSet->parse(options);
    mNumFlowsPerPair=optionsSet->referenceOption("num-flows-per-pair")->as<uint32>();
    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<double>();
    mPingPayloadSize=optionsSet->referenceOption("ping-size")->as<uint32>();
    mHPSize=optionsSet->referenceOption("hp-size")->as<uint32>();
    mNumHitPointsPerSecond=optionsSet->referenceOption("num-hp-per-second")->as<double>();
    if (GetOptionValue<ObjectHostID>("ohid").id!=ObjectHostID(11).id) {
        SILOG(oh,error,"Not #11, not providing any hp pings");
        mNumHitPointsPerSecond=0;
    }else {
        SILOG(oh,error,"Am #11, providing hp pings!!! no other pings");
        mNumPingsPerSecond=0;
    }
    mFloodServer = optionsSet->referenceOption("flood-server")->as<uint32>();
    mSourceFloodServer = optionsSet->referenceOption("source-flood-server")->as<bool>();
    mNumObjectsPerServer=optionsSet->referenceOption("num-objects-per-server")->as<uint32>();
    mLocalTraffic = optionsSet->referenceOption("local")->as<bool>();
    mFractionMessagesUniform= optionsSet->referenceOption("prob-messages-uniform")->as<double>();
    Vector3ui32 layout = GetOptionValue<Vector3ui32>("layout");


    Duration duration = GetOptionValue<Duration>("duration");

    
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
AirTrafficControllerScenario::~AirTrafficControllerScenario(){
    SILOG(deluge,fatal,
        "HitPoint: Generated: " << mNumGeneratedPings <<
        " Sent: " << mNumTotalPings);
    std::stringstream filenamess;
    filenamess<<"hitpointlog."<<mNumPingsPerSecond<<'.'<<mNumHitPointsPerSecond;

    std::string filename= filenamess.str();
    FILE *fp=fopen(filename.c_str(),"wb");
    if(fp) {
        for (DamagableObjectMap::iterator i=mDamagableObjects.begin(),ie=mDamagableObjects.end();i!=ie;++i) {
            double minReceiveTime=1.0e38;
            Time mMinRecvTimeRecord(mStartTime);
            Time mMaxRecvTimeRecord(mStartTime);
            double maxReceiveTime=0;
            for (AIRTRAFFIC::DamagableObject::DamageReceiverMap::iterator j=i->second->mDamageReceivers.begin(),je=i->second->mDamageReceivers.end();j!=je;++j) {
                fprintf(fp,"%f",j->second->distance);
                for(size_t k=0;k<j->second->samples.size();++k) {
                    std::pair<Time, DamagableObject::HPTYPE >*sample = &j->second->samples[k];
                    double hp = sample->second.read();
                    Time sampTime(sample->first);
                    double recvtime=(sampTime-mStartTime).toSeconds();
                    fprintf(fp," %f %f",recvtime,hp);
                    if (recvtime>maxReceiveTime) {
                        maxReceiveTime=recvtime;
                        mMaxRecvTimeRecord=sampTime;
                    }
                    if (recvtime<minReceiveTime) {
                        minReceiveTime=recvtime;
                        mMinRecvTimeRecord=sampTime;
                    }
                }
                fprintf(fp,"\n");
            }
            fprintf (fp,"%f, %f, %f, %f\n",minReceiveTime,calcHp(this,&mMinRecvTimeRecord).read(),maxReceiveTime,calcHp(this,&mMaxRecvTimeRecord).read());
        }
        fclose(fp);
    }

    delete mPings;
    delete mPingPoller;
    delete mPingProfiler;
}

AirTrafficControllerScenario*AirTrafficControllerScenario::create(const String&options){
    return new AirTrafficControllerScenario(options);
}
void AirTrafficControllerScenario::addConstructorToFactory(ScenarioFactory*thus){
    thus->registerConstructor("airtraffic",&AirTrafficControllerScenario::create);
}
void AirTrafficControllerScenario::hpReturn(const Sirikata::Protocol::Object::ObjectMessage&msg){
    static bool first=true;
    if (first) {
        first=false;
        SILOG(oh,error,"RECVING");
    }
    DamagableObjectMap::iterator where =mDamagableObjects.find(msg.source_object());
    if (where!=mDamagableObjects.end()) {
        DamagableObject::DamageReceiverMap::iterator where2=where->second->mDamageReceivers.find(msg.dest_object());
        if (where2!=where->second->mDamageReceivers.end()) {
            where2->second->getUpdate((const uint8*)msg.payload().data(),msg.payload().size());
        }else {
            SILOG(oh,error,"FAILED to find object "<<msg.dest_object().toString() <<" in map");
        }
    }else {
        SILOG(oh,error,"FAILED to find sender object "<<msg.source_object().toString() <<" in map");
    }
}
void AirTrafficControllerScenario::initialize(ObjectHostContext*ctx) {
    mGenPhase=GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
    mContext=ctx;
    BoundingBox3f region = GetOptionValue<BoundingBox3f>("region");


    Duration duration = GetOptionValue<Duration>("duration");


    mObjectTracker = new ConnectedObjectTracker(mContext->objectHost);
    if (mNumHitPointsPerSecond) {
        OptionSet* optionsSet = OptionSet::getOptions("AirTrafficControllerScenario",this);
        mAirplanes = new ObjectFactory(ctx, region, duration, 200, optionsSet->referenceOption("num-receivers")->as<int>());
        
        mControlTower= new ObjectFactory(ctx, BoundingBox3f(Vector3f(0,0,0),Vector3f(0,0,0))/*make sure airport is at the edge of the scene*/, duration,550,2);
        ctx->add(mAirplanes);
        ctx->add(mControlTower);
    }else {
        mAirplanes=mControlTower=NULL;
    }
    mContext->objectHost->registerService(mPort,std::tr1::bind(&AirTrafficControllerScenario::hpReturn,this,_1));

    mPingProfiler = mContext->profiler->addStage("Object Host Send Pings");
    double bothpersecond=(mNumHitPointsPerSecond+mNumPingsPerSecond);
    mPingPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&AirTrafficControllerScenario::sendPings, this),
        (bothpersecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/bothpersecond) :
         Duration::seconds(1.0/bothpersecond))
    );
/*
    mHPPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&AirTrafficControllerScenario::sendHPs, this),
        Duration::seconds(1./mNumHitPointsPerSecond));
*/
    mGeneratePingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
    mGeneratePingsStrand = mContext->ioService->createStrand();
    mGeneratePingPoller = new Poller(
        mGeneratePingsStrand,
        std::tr1::bind(&AirTrafficControllerScenario::generatePings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );
}

void AirTrafficControllerScenario::start() {
    Duration connect_phase = GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
    mContext->mainStrand->post(
        connect_phase,
        std::tr1::bind(&AirTrafficControllerScenario::delayedStart, this)
    );
}
void AirTrafficControllerScenario::delayedStart() {
    mStartTime = mContext->simTime();
    OptionSet* optionsSet = OptionSet::getOptions("AirTrafficControllerScenario",this);

    int ss=mFloodServer;
    if (mFloodServer ==0) {
        ss=(rand() % mObjectTracker->numServerIDs())+1;
    }
    if (mNumHitPointsPerSecond) {
        Object * objA = mControlTower->getNextObject(UUID::null());
        if (!objA->connected()) {
            objA = mControlTower->getNextObject(objA->uuid());
        }
        if (!objA->connected()) {
            SILOG(oh,error,"Air traffic controller: error, tower not connected\n");
        }
        if (objA) {
            DamagableObject * d=(mDamagableObjects[objA->uuid()]=new DamagableObject(objA,1000, mPort, mHPSize));
            std::vector<Object* >allReceivers;
            Object  * objB=NULL;
            while((objB=mAirplanes->getNextObject(objB?objB->uuid():UUID::null()))) {
                if (objB->connected()) {
                    allReceivers.push_back(objB);
                    d->mDamageReceivers[objB->uuid()]=NULL;
                }
            }
            
            SILOG(oh,error,"Pinging "<<allReceivers.size()<<" objects ");
            
            for (size_t i=0;i<allReceivers.size();++i) {
                d->mDamageReceivers[allReceivers[i]->uuid()]=(new DamagableObject::ReceiveDamage(d,
                                                                                                 this,
                                                                                                 allReceivers[i],
                                                                                                 allReceivers[i]->uuid()));
            }
            
            
        }else {
            Duration connect_phase = GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
            connect_phase=connect_phase/16.0;
            SILOG(oh, debug, "error during connect phase, retrying "<<connect_phase<<" later");
            mContext->mainStrand->post(
                connect_phase,
                std::tr1::bind(&AirTrafficControllerScenario::delayedStart, this)
                );
            
        }
    }
    mGeneratePingPoller->start();
    //mHPPoller->start();
    mPingPoller->start();


}
void AirTrafficControllerScenario::stop() {
    mPingPoller->stop();
    mGeneratePingPoller->stop();
}
#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)
void AirTrafficControllerScenario::generatePairs() {

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
            for (int nf=0;nf<(int)(this->mNumFlowsPerPair*(i+1));++nf)
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
bool AirTrafficControllerScenario::generateOnePing(const Time& t, PingInfo* result) {
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
        result->objA = where->source;
        result->objB = where->dest;
        result->dist = where->dist;
        result->ping = new Sirikata::Protocol::Object::Ping();
        mContext->objectHost->fillPing(result->dist, mPingPayloadSize, result->ping);
        return true;
    }

    return false;
}

void AirTrafficControllerScenario::generatePings() {
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


void AirTrafficControllerScenario::sendHPs() {
    {
        bool first=true;
        Time t(mContext->simTime());
        DamagableObject::HPTYPE hp = calcHp(this,&t);

        for (        DamagableObjectMap::iterator i=mDamagableObjects.begin(),ie=mDamagableObjects.end();i!=ie;++i) {
            i->second->mHP=hp;
            i->second->update();
            assert(first);
            first=false;
        }

    }
}

void AirTrafficControllerScenario::sendPings() {
    mPingProfiler->started();

    Time newTime=mContext->simTime();
    double both=(mNumPingsPerSecond+mNumHitPointsPerSecond);
    int64 howManyPings = (newTime-mStartTime).toSeconds()*both;
    double chanceHp = mNumHitPointsPerSecond/both;
    bool broke=false;
    // NOTE: We limit here because we can get in lock-step with the generator,
    // causing this to run for excessively long when we fall behind on pings.
    int64 limit = std::min((int64)howManyPings-mNumTotalPings, mMaxPingsPerRound);
    int64 i;
    for (i=0;i<limit;++i) {
        Time t(mContext->simTime());
        bool sent_success=true;
        if (rand()<RAND_MAX*chanceHp) {
            this->sendHPs();
        }else {
            PingInfo result;
            if (!mPings->pop(result)) {
                SILOG(oh,insane,"[OH] " << "Ping queue underflowed.");
                break;
            }
            sent_success = mContext->objectHost->sendPing(t, result.objA, result.objB, result.ping);
            delete result.ping;
        }
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
