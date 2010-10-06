/*  Sirikata
 *  HitPointScenario.hpp
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

#ifndef _AIR_TRAFFIC_CONTROLLER_SCENARIO_HPP_
#define _AIR_TRAFFIC_CONTROLLER_SCENARIO_HPP_

#include "Scenario.hpp"
#include <sirikata/core/queue/CountResourceMonitor.hpp>
#include <sirikata/core/queue/SizedThreadSafeQueue.hpp>
#include <sirikata/core/service/Poller.hpp>

namespace Sirikata {

class ScenarioFactory;
class ConnectedObjectTracker;
class RegionWeightCalculator;
class ObjectFactory;
namespace AIRTRAFFIC {
class DamagableObject;
}
class AirTrafficControllerScenario : public Scenario {
    double mNumPingsPerSecond;
    int32 mNumFlowsPerPair;
    double mNumHitPointsPerSecond;
    int mPort;
    ConnectedObjectTracker* mObjectTracker;
    Poller* mPingPoller;
    //Poller* mHPPoller;

    Network::IOStrand* mGeneratePingsStrand;
    Poller* mGeneratePingPoller;

    struct PingInfo {
        UUID objA;
        UUID objB;
        float dist;
        Sirikata::Protocol::Object::Ping* ping;
    };
    Sirikata::SizedThreadSafeQueue<PingInfo,CountResourceMonitor>* mPings;
    int64 mNumGeneratedPings;
    TimeProfiler::Stage* mGeneratePingProfiler;

    unsigned int mPingID;
    uint32 mPingPayloadSize;
    int64 mNumTotalPings;
    int64 mNumTotalHPs;
    int64 mMaxPingsPerRound;
    TimeProfiler::Stage* mPingProfiler;

    ServerID mFloodServer;
    uint32 mNumObjectsPerServer;
    bool mLocalTraffic;
    bool mSourceFloodServer;
    double mFractionMessagesUniform;
    void hpReturn(const Sirikata::Protocol::Object::ObjectMessage&msg);
    struct MessageFlow {
        float cumulativeProbability;
        float dist;
        UUID source;
        UUID dest;
        bool operator< (const MessageFlow&flow) const{
            return cumulativeProbability<flow.cumulativeProbability;
        }
    };
    Duration mGenPhase;
    class MessageFlowLess{public:
            bool operator()(const MessageFlow&a,const MessageFlow&b) const{return a<b;}
    };
    typedef std::vector<MessageFlow> FlowCDF;
    FlowCDF mSendCDF;
    void delayedStart();

    bool generateOnePing(const Time& t, PingInfo* result);
    void generatePings();

    void sendPings();
    void sendHPs();
    RegionWeightCalculator* mWeightCalculator;
    static AirTrafficControllerScenario*create(const String&options);
    void generatePairs();
    ObjectFactory *mAirplanes;
    ObjectFactory *mControlTower;
public:
    Time mStartTime;
    uint32 mHPSize;
    ObjectHostContext*mContext;

    AirTrafficControllerScenario(const String &options);
    ~AirTrafficControllerScenario();
    virtual void initialize(ObjectHostContext*);
    void start();
    void stop();
    static void addConstructorToFactory(ScenarioFactory*);
    typedef std::tr1::unordered_map<UUID,AIRTRAFFIC::DamagableObject*,UUID::Hasher> DamagableObjectMap;
    DamagableObjectMap mDamagableObjects;
    
};

} // namespace Sirikata

#endif //_PING_DELUGE_SCENARIO_HPP_
