/*  Sirikata
 *  LoadPacketTrace.cpp
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

#include "LoadPacketTrace.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/cbrcore/Options.hpp>
#include "ConnectedObjectTracker.hpp"

#include <sirikata/cbrcore/ServerWeightCalculator.hpp>
namespace Sirikata {
void DPSInitOptions(LoadPacketTrace *thus) {

    Sirikata::InitializeClassOptions ico("LoadPacketTrace",thus,
        new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("prob-messages-uniform","1",Sirikata::OptionValueType<double>(),"Number of pings launched per simulation second"),
        new OptionValue("num-objects-per-server","1000",Sirikata::OptionValueType<uint32>(),"The number of objects that should be connected before the pinging begins"),
        new OptionValue("ping-size","30",Sirikata::OptionValueType<uint32>(),"Size of ping payloads.  Doesn't include any other fields in the ping or the object message headers."),
        new OptionValue("flood-server","4",Sirikata::OptionValueType<uint32>(),"The index of the server to flood.  Defaults to 1 so it will work with all layouts. To flood all servers, specify 0."),
        new OptionValue("source-flood-server","false",Sirikata::OptionValueType<bool>(),"This makes the flood server the source of all the packets rather than the destination, so that we can validate that egress routing gets proper fairness."),
        new OptionValue("local","false",Sirikata::OptionValueType<bool>(),"If true, generated traffic will all be local, i.e. will all originate at the flood-server.  Otherwise, it will always originate from other servers."),
        new OptionValue("tracefile","messagetrace",Sirikata::OptionValueType<String>(),"File that will store the traces in ascii with the source then destination object UUIDs separated by one character and ending with a newline of some sort"),
        NULL);
}
enum {UUIDLEN =36};
void LoadPacketTrace::loadPacketTrace(const std::string &tracefile, std::vector<std::pair<UUID,UUID> >&retval) {
    FILE *fp=fopen(tracefile.c_str(),"rb");
    if (fp) {
        char line[1025];
        line[1024]='\0';
        int msgcount=0;
        while ((!feof(fp))&&fgets(line,1024,fp)) {
            if (strlen(line)<UUIDLEN*2+1)continue;
            char source[UUIDLEN+1];
            char dest[UUIDLEN+1];
            source[UUIDLEN]='\0';
            dest[UUIDLEN]='\0';
            memcpy(source,line,UUIDLEN);
            memcpy(dest,line+UUIDLEN+1,UUIDLEN);
            //fprintf (stderr,"sd %s %s\n",source,dest);
            //SILOG(loadpackettrace,fatal,"source  "<<source << " dest "<<dest<<'\n');


            UUID s(source,UUID::HumanReadable());
            UUID d(dest,UUID::HumanReadable());
            Object *ss=mObjectTracker->getObject(s);

            Object *dd=mObjectTracker->getObject(d);
            std::string sstr(s.toString()),dstr(d.toString());
            //SILOG(loadpackettrace,fatal,"source  "<<sstr<< " dest "<<dstr<<'\n');

            //fprintf (stderr,"zd %s %s\n",sstr.c_str(),dstr.c_str());
            if (ss&&dd&&ss->connected()&&dd->connected()) {
                //fprintf (stderr,"xd %s %s\n",sstr.c_str(),dstr.c_str());
                //SILOG(loadpackettrace,fatal,"source  "<<s.toString() << " dest "<<d.toString()<<'\n');
                retval.push_back(std::pair<UUID,UUID>(s,d));
                ++msgcount;
            }
            line[0]='\0';//clear the entry

        }

        fclose(fp);

        if (retval.empty()){
            Object* first=mObjectTracker->roundRobinObject(mFloodServer);
            Object* second=mObjectTracker->roundRobinObject();
            retval.push_back(std::pair<UUID,UUID>(first->uuid(),second->uuid()));
        }
    }else{
        SILOG(loadpackettrace,fatal,"Unable to open file "<<tracefile);
    }

}
LoadPacketTrace::LoadPacketTrace(const String &options)
 : mStartTime(Time::epoch())
{
    mNumTotalPings=0;
    mContext=NULL;
    mObjectTracker = NULL;
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("LoadPacketTrace",this);
    optionsSet->parse(options);
    mPacketTraceFileName=optionsSet->referenceOption("tracefile")->as<String>();

    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<double>();
    mPingPayloadSize=optionsSet->referenceOption("ping-size")->as<uint32>();
    mFloodServer = optionsSet->referenceOption("flood-server")->as<uint32>();
    mSourceFloodServer = optionsSet->referenceOption("source-flood-server")->as<bool>();
    mNumObjectsPerServer=optionsSet->referenceOption("num-objects-per-server")->as<uint32>();
    mLocalTraffic = optionsSet->referenceOption("local")->as<bool>();
    mNumGeneratedPings = 0;
    mGeneratePingsStrand = NULL;
    mGeneratePingPoller = NULL;
    mPings = // We allocate space for 1/4 seconds worth of pings
        new Sirikata::SizedThreadSafeQueue<PingInfo,CountResourceMonitor>(
            CountResourceMonitor(std::max((uint32)(mNumPingsPerSecond / 4), (uint32)2))
        );
    mWeightCalculator=WeightCalculatorFactory(NULL);
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

LoadPacketTrace::~LoadPacketTrace(){
    SILOG(loadpackettrace,fatal,
        "LoadPacketTrace: Generated: " << mNumGeneratedPings <<
        " Sent: " << mNumTotalPings);
    delete mPings;
    delete mPingPoller;
    delete mPingProfiler;

}


LoadPacketTrace*LoadPacketTrace::create(const String&options){
    return new LoadPacketTrace(options);
}
void LoadPacketTrace::addConstructorToFactory(ScenarioFactory*thus){
    thus->registerConstructor("loadpackettrace",&LoadPacketTrace::create);
}

void LoadPacketTrace::initialize(ObjectHostContext*ctx) {
    mGenPhase=GetOption(OBJECT_CONNECT_PHASE)->as<Duration>();
    mContext=ctx;
    mObjectTracker = new ConnectedObjectTracker(mContext->objectHost);

    mPingProfiler = mContext->profiler->addStage("Object Host Send Pings");
    mPingPoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&LoadPacketTrace::sendPings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );

    mGeneratePingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
    mGeneratePingsStrand = mContext->ioService->createStrand();
    mGeneratePingPoller = new Poller(
        mGeneratePingsStrand,
        std::tr1::bind(&LoadPacketTrace::generatePings, this),
        mNumPingsPerSecond > 1000 ? // Try to amortize out some of the
                                    // scheduling cost
        Duration::seconds(10.0/mNumPingsPerSecond) :
        Duration::seconds(1.0/mNumPingsPerSecond)
    );
}

void LoadPacketTrace::start() {
    Duration connect_phase = GetOption(OBJECT_CONNECT_PHASE)->as<Duration>();
    mContext->mainStrand->post(
        connect_phase,
        std::tr1::bind(&LoadPacketTrace::delayedStart, this)
    );
}
void LoadPacketTrace::delayedStart() {
    mStartTime = mContext->simTime();
    mGeneratePingPoller->start();
    mPingPoller->start();
}
void LoadPacketTrace::stop() {
    mPingPoller->stop();
    mGeneratePingPoller->stop();
}
#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)
void LoadPacketTrace::generatePairs() {

    if (mPacketTrace.empty()) {
        std::vector<Object*> floodedObjects;
        Time t=mContext->simTime();
        int nobjects=0;
        for (int i=0;i<(int)mObjectTracker->numServerIDs();++i) {
            nobjects+=mObjectTracker->numObjectsConnected(mObjectTracker->getServerID(i));
        }
        loadPacketTrace(mPacketTraceFileName,mPacketTrace);
        OH_LOG(warning, "Beginning object seed phase at " << (t-mStartTime)<<" with "<<nobjects<<" connected objects and "<<mPacketTrace.size()<<"messages\n");
    }

}
static unsigned int even=0;
bool LoadPacketTrace::generateOnePing(const Time& t, PingInfo* result) {
    generatePairs();
    result->objA=mPacketTrace[mPacketTraceIndex%mPacketTrace.size()].first;
    result->objB=mPacketTrace[mPacketTraceIndex++%mPacketTrace.size()].second;
    result->ping = new Sirikata::Protocol::Object::Ping();
    result->dist=(double)mPacketTraceIndex;
    mContext->objectHost->fillPing(result->dist, mPingPayloadSize, result->ping);
    return true;
}

void LoadPacketTrace::generatePings() {
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

void LoadPacketTrace::sendPings() {
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
