#ifndef _DISTRIBUTION_PING_SCENARIO_HPP_
#define _DISTRIBUTION_PING_SCENARIO_HPP_

#include "Scenario.hpp"
#include "CountResourceMonitor.hpp"
#include "sirikata/util/SizedThreadSafeQueue.hpp"

namespace CBR {
class ScenarioFactory;
class ConnectedObjectTracker;

class DistributionPingScenario : public Scenario {
    double mNumPingsPerSecond;

    ObjectHostContext*mContext;
    ConnectedObjectTracker* mObjectTracker;
    Poller* mPingPoller;

    IOStrand* mGeneratePingsStrand;
    Poller* mGeneratePingPoller;

    struct PingInfo {
        UUID objA;
        UUID objB;
        float dist;
    };
    Sirikata::SizedThreadSafeQueue<PingInfo,CountResourceMonitor>* mPings;
    int64 mNumGeneratedPings;
    TimeProfiler::Stage* mGeneratePingProfiler;

    unsigned int mPingID;
    bool mSameObjectHostPings;
    bool mForceSameObjectHostPings;
    uint32 mPingPayloadSize;
    Time mStartTime;
    int64 mNumTotalPings;
    int64 mMaxPingsPerRound;
    TimeProfiler::Stage* mPingProfiler;

    bool generateOnePing(ServerID minServer, unsigned int distance, const Time& t, PingInfo* result);
    void generatePings();

    void sendPings();

    static DistributionPingScenario*create(const String&options);
public:
    DistributionPingScenario(const String &options);
    ~DistributionPingScenario();
    virtual void initialize(ObjectHostContext*);
    void start();
    void stop();
    static void addConstructorToFactory(ScenarioFactory*);
};
}
#endif
