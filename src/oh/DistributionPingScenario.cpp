#include "DistributionPingScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
namespace CBR{
DistributionPingScenario::DistributionPingScenario(const String &options){ 
    mContext=NULL;    
    mPingID=0;
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
    mPingProfiler = mContext->profiler->addStage("Object Host Generate Pings");
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
    unsigned int maxDistance=mContext->objectHost->getObjectConnections()->numServerIDs();
    unsigned int distance=0;
    if (maxDistance>1) {
        maxDistance-=1;
        distance=(rand()%maxDistance)+1;//uniform distance at random
    }    
    unsigned int minServer=(rand()%(maxDistance-distance+1))+1;
    
    for (int i=0;i<1000;++i) {        
        Object * objA=mContext->objectHost->getObjectConnections()->randomObject((ServerID)minServer,
                                                         false);
        Object * objB=mContext->objectHost->getObjectConnections()->randomObject((ServerID)(minServer+distance),
                                                         false);
        //FIXME: below I am selecting the objects badly using a bad mechanism from Bertrand's paradox suggesting I wil get a shorter length on average. But I don't want to change
        //the benchmarks too much mid-stream
        objA=mContext->objectHost->getObjectConnections()->randomObject();
        objB=mContext->objectHost->getObjectConnections()->randomObject();
        if (rand()<RAND_MAX/2) {
            Object * tmp=objA;
            objA=objB;
            objB=tmp;
        }
        Time t=mContext->simTime();
        if (objA&&objB) {
            if (!mContext->objectHost->ping(t,
                                            objA,
                                            objB->uuid(),
                                            (objA->location().extrapolate(t).position()-objB->location().extrapolate(t).position()).length())) {
                
                break;
            }
        }
    }
    mPingProfiler->finished();
}
}
