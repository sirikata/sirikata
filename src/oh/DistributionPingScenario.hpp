#ifndef _DISTRIBUTION_PING_SCENARIO_HPP_
#define _DISTRIBUTION_PING_SCENARIO_HPP_

#include "Scenario.hpp"
namespace CBR {
class ScenarioFactory;
class ConnectedObjectTracker;
class DistributionPingScenario : public Scenario {
    double mNumPingsPerSecond;

    ObjectHostContext*mContext;
    ConnectedObjectTracker* mObjectTracker;
    Poller* mPingPoller;
    unsigned int mPingID;
    bool mSameObjectHostPings;
    bool mForceSameObjectHostPings;
    Time mStartTime;
    int64 mNumTotalPings;
    TimeProfiler::Stage* mPingProfiler;

    bool pingOne(ServerID minServer, unsigned int distance);
    void generatePings();
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
