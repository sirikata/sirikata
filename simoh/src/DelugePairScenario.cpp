/*  Sirikata
 *  DelugePairScenario.cpp
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

#include "DelugePairScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include "ConnectedObjectTracker.hpp"
#include <sirikata/core/util/RegionWeightCalculator.hpp>

namespace Sirikata {
void DPSInitOptions(DelugePairScenario *thus) {

    Sirikata::InitializeClassOptions ico("DelugePairScenario",thus,
        new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("prob-messages-uniform","1",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("num-objects-per-server","1000",Sirikata::OptionValueType<uint32>(),"The number of objects that should be connected before the pinging begins"),
        new OptionValue("ping-size","1024",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("flood-server","1",Sirikata::OptionValueType<uint32>(),"The index of the server to flood.  Defaults to 1 so it will work with all layouts. To flood all servers, specify 0."),
        new OptionValue("source-flood-server","false",Sirikata::OptionValueType<bool>(),"This makes the flood server the source of all the packets rather than the destination, so that we can validate that egress routing gets proper fairness."),
        new OptionValue("local","false",Sirikata::OptionValueType<bool>(),"If true, generated traffic will all be local, i.e. will all originate at the flood-server.  Otherwise, it will always originate from other servers."),
        NULL);
}

DelugePairScenario::DelugePairScenario(const String &options)
 : mStartTime(Time::epoch())
{
    mNumTotalPings=0;
    mContext=NULL;
    mObjectTracker = NULL;
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("DelugePairScenario",this);
    optionsSet->parse(options);

    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<double>();
    mPingPayloadSize=optionsSet->referenceOption("ping-size")->as<uint32>();
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
DelugePairScenario::~DelugePairScenario(){
    SILOG(deluge,fatal,
        "DelugePair: Generated: " << mNumGeneratedPings <<
        " Sent: " << mNumTotalPings);
    delete mPings;
    delete mPingPoller;
    delete mPingProfiler;
}

DelugePairScenario*DelugePairScenario::create(const String&options){
    return new DelugePairScenario(options);
}
void DelugePairScenario::addConstructorToFactory(ScenarioFactory*thus){
    thus->registerConstructor("delugepair",&DelugePairScenario::create);
}

void DelugePairScenario::initialize(ObjectHostContext*ctx) {
    mGenPhase=GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
    mContext=ctx;
    mObjectTracker = new ConnectedObjectTracker(mContext->objectHost);

    mPingProfiler = mContext->profiler->addStage("Object Host Send Pings");
    mPingPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&DelugePairScenario::sendPings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );

    mGeneratePingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
    mGeneratePingsStrand = mContext->ioService->createStrand();
    mGeneratePingPoller = new Poller(
        mGeneratePingsStrand,
        std::tr1::bind(&DelugePairScenario::generatePings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );
}

void DelugePairScenario::start() {
    Duration connect_phase = GetOptionValue<Duration>(OBJECT_CONNECT_PHASE);
    mContext->mainStrand->post(
        connect_phase,
        std::tr1::bind(&DelugePairScenario::delayedStart, this)
    );
}
void DelugePairScenario::delayedStart() {
    mStartTime = mContext->simTime();
    mGeneratePingPoller->start();
    mPingPoller->start();
}
void DelugePairScenario::stop() {
    mPingPoller->stop();
    mGeneratePingPoller->stop();
}
#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)
void DelugePairScenario::generatePairs() {

    if (mSendCDF.empty()) {
        std::vector<Object*> floodedObjects;
        Time t=mContext->simTime();

        for (int i=0;i<mObjectTracker->numServerIDs();++i) {
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
bool DelugePairScenario::generateOnePing(const Time& t, PingInfo* result) {
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

void DelugePairScenario::generatePings() {
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

void DelugePairScenario::sendPings() {
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
