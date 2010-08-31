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
#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include "ConnectedObjectTracker.hpp"

namespace Sirikata {

class DamagableObject {
public:
    Object *object;
    typedef float HPTYPE;
    HPTYPE mHP;
    unsigned int mListenPort;
    DamagableObject (Object *obj, float hp, unsigned int listenPort){
        mHP=hp;
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
    public:
        ReceiveDamage(DamagableObject*parent, UUID id) {
            mID=id;
            mParent=parent;
            mPartialCount=0;
            mPartialSendCount=0;
            memset(mPartialSend,0,sizeof(DamagableObject::HPTYPE)*2);
            memset(mPartialUpdate,0,sizeof(DamagableObject::HPTYPE));
            Stream<UUID>::listen(std::tr1::bind(&DamagableObject::ReceiveDamage::login,this,_1,_2),
                                 EndPoint<UUID>(mID,parent->mListenPort));
            Stream<UUID>::connectStream(mParent->object,
                                        EndPoint<UUID>(mParent->object->uuid(),parent->mListenPort),
                                        EndPoint<UUID>(mID,parent->mListenPort),
                                        std::tr1::bind(&DamagableObject::ReceiveDamage::connectionCallback,this,_1,_2));

        }
        void connectionCallback(int err, boost::shared_ptr<Stream<UUID> > s) {
            if (err != 0 ) {
                SILOG(hitpoint,error,"Failed to connect two objects\n");
            }else {
                this->mSendStream = s;
                s->registerReadCallback( std::tr1::bind( &DamagableObject::ReceiveDamage::senderShouldNotGetUpdate, this, _1, _2)  ) ;
                sendUpdate();
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
            int tosend=sizeof(DamagableObject::HPTYPE);
            if (mPartialSendCount) {
                memcpy(mPartialSend+sizeof(DamagableObject::HPTYPE),&mParent->mHP,sizeof(DamagableObject::HPTYPE));
                tosend*=2;
                tosend-=mPartialSendCount;
            }else {
                memcpy(mPartialSend,&mParent->mHP,tosend);
            }
            mPartialSendCount+=mSendStream->write(mPartialSend+mPartialSendCount,tosend);
            if(mPartialSendCount>=sizeof(DamagableObject::HPTYPE)) {
                mPartialSendCount-=sizeof(DamagableObject::HPTYPE);
                memcpy(mPartialSend,mPartialSend+sizeof(DamagableObject::HPTYPE),sizeof(DamagableObject::HPTYPE));
            }
            if(mPartialSendCount>=sizeof(DamagableObject::HPTYPE)) {
                mPartialSendCount=0;
            }
            assert(mPartialSendCount<sizeof(DamagableObject::HPTYPE));
        }
        void senderShouldNotGetUpdate(uint8*buffer, int length) {
            SILOG(hitpoint,error,"Should not receive updates from receiver");
        }
        void getUpdate(uint8*buffer, int length) {
            while (length>0) {

                int datacopied = (length>sizeof(DamagableObject::HPTYPE)-mPartialCount?sizeof(DamagableObject::HPTYPE)-mPartialCount:length);
                memcpy(mPartialUpdate+mPartialCount, buffer,datacopied);
                mPartialCount+=datacopied;
                if (mPartialCount==sizeof(DamagableObject::HPTYPE)) {
                    DamagableObject::HPTYPE hp;
                    memcpy(&hp,mPartialUpdate,mPartialCount);
                    SILOG(hitpoint,error,"Got hitpoint update for "<<mID.toString()<<" as "<<hp);
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

void PDSInitOptions(HitPointScenario *thus) {

    Sirikata::InitializeClassOptions ico("HitPointScenario",thus,
        new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("ping-size","1024",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("flood-server","1",Sirikata::OptionValueType<uint32>(),"The index of the server to flood.  Defaults to 1 so it will work with all layouts. To flood all servers, specify 0."),
        new OptionValue("local","false",Sirikata::OptionValueType<bool>(),"If true, generated traffic will all be local, i.e. will all originate at the flood-server.  Otherwise, it will always originate from other servers."),
        NULL);
}

HitPointScenario::HitPointScenario(const String &options)
 : mStartTime(Time::epoch())
{
    mNumTotalPings=0;
    mContext=NULL;
    mObjectTracker = NULL;
    mPingID=0;
    PDSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("HitPointScenario",this);
    optionsSet->parse(options);

    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<double>();
    mPingPayloadSize=optionsSet->referenceOption("ping-size")->as<uint32>();
    mFloodServer = optionsSet->referenceOption("flood-server")->as<uint32>();
    mLocalTraffic = optionsSet->referenceOption("local")->as<bool>();

    mNumGeneratedPings = 0;
    mGeneratePingsStrand = NULL;
    mGeneratePingPoller = NULL;
    mPings = // We allocate space for 1/4 seconds worth of pings
        new Sirikata::SizedThreadSafeQueue<PingInfo,CountResourceMonitor>(
            CountResourceMonitor(std::max((uint32)(mNumPingsPerSecond / 4), (uint32)2))
        );
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
    mGeneratePingPoller->start();
    mPingPoller->start();
}
void HitPointScenario::stop() {
    mPingPoller->stop();
    mGeneratePingPoller->stop();
}
bool HitPointScenario::generateOnePing(const Time& t, PingInfo* result) {
    Object* objA = NULL;
    Object* objB = NULL;
    if (mFloodServer == 0) {
        // When we don't have a flood server, we have a slightly different process.
        if (mLocalTraffic) {
            // If we need local traffic, pick a server and pin both of
            // them to that server.
            ServerID ss = rand() % mObjectTracker->numServerIDs();
            objA = mObjectTracker->randomObjectFromServer(ss);
            objB = mObjectTracker->randomObjectFromServer(ss);
        }
        else {
            // Otherwise, we don't care about the origin of either
            objA = mObjectTracker->randomObject();
            objB = mObjectTracker->randomObject();
        }
    }
    else {
        // When we do have a flood server, we always pick the dest
        // from that server.
        objB = mObjectTracker->randomObjectFromServer(mFloodServer);

        if (mLocalTraffic) {
            // If forcing local traffic, pick source from same server
            objA = mObjectTracker->randomObjectFromServer(mFloodServer);
        }
        else {
            // Otherwise, pick from any *other* server.
            objA = mObjectTracker->randomObjectExcludingServer(mFloodServer);
        }
    }

    if (!objA || !objB) {
        return false;
    }

    result->objA = objA->uuid();
    result->objB = objB->uuid();
    result->dist = (objA->location().position(t) - objB->location().position(t)).length();
    result->ping = new Sirikata::Protocol::Object::Ping();
    mContext->objectHost->fillPing(result->dist, mPingPayloadSize, result->ping);
    return true;
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
