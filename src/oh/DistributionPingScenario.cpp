#include "DistributionPingScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
namespace CBR{
DistributionPingScenario::DistributionPingScenario(const String &options){ 
    mContext=NULL;    
    mPingID=0;
    mPingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
}
DistributionPingScenario::~DistributionPingScenario(){ 
    delete mPingPoller;
    delete mPingProfiler;
}

DistributionPingScenario*DistributionPingScenario::create(const String&options){
    return new DistributionPingScenario(options);
}
void DistributionPingScenario::addConstructorToFactory(ScenarioFactory*thus){
    thus->registerConstructor("ping",&DistributionPingScenario::create);
}

void DistributionPingScenario::initialize(ObjectHostContext*ctx) {
    mContext=ctx;
    mPingPoller = new Poller(ctx->mainStrand, std::tr1::bind(&DistributionPingScenario::generatePings, this), Duration::milliseconds((int64)1));
}

void DistributionPingScenario::start() {
    mPingPoller->start();
}
void DistributionPingScenario::stop() {
    mPingPoller->stop();
}
void DistributionPingScenario::generatePings() {
    mPingProfiler->started();

    for (int i=0;i<1000;++i)
        if (!mContext->objectHost->randomPing(mContext->simTime()))
            break;

    mPingProfiler->finished();
}
}
