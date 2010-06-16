/*  Sirikata
 *  PingDelugeScenario.hpp
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

#ifndef _PING_DELUGE_SCENARIO_HPP_
#define _PING_DELUGE_SCENARIO_HPP_

#include "Scenario.hpp"
#include <sirikata/cbrcore/CountResourceMonitor.hpp>
#include <sirikata/core/util/SizedThreadSafeQueue.hpp>

namespace Sirikata {

class ScenarioFactory;
class ConnectedObjectTracker;

class PingDelugeScenario : public Scenario {
    double mNumPingsPerSecond;

    ObjectHostContext*mContext;
    ConnectedObjectTracker* mObjectTracker;
    Poller* mPingPoller;

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
    Time mStartTime;
    int64 mNumTotalPings;
    int64 mMaxPingsPerRound;
    TimeProfiler::Stage* mPingProfiler;

    uint32 mFloodServer;
    bool mLocalTraffic;

    void delayedStart();

    bool generateOnePing(const Time& t, PingInfo* result);
    void generatePings();

    void sendPings();

    static PingDelugeScenario*create(const String&options);
public:
    PingDelugeScenario(const String &options);
    ~PingDelugeScenario();
    virtual void initialize(ObjectHostContext*);
    void start();
    void stop();
    static void addConstructorToFactory(ScenarioFactory*);
};

} // namespace Sirikata

#endif //_PING_DELUGE_SCENARIO_HPP_
