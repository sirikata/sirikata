#include "DistributionPingScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include "Options.hpp"
namespace CBR{
void DPSInitOptions(DistributionPingScenario *thus) {
   
    Sirikata::InitializeClassOptions ico("DistributedPingScenario",thus,
                                         new OptionValue("num-pings-per-tick","1000",Sirikata::OptionValueType<size_t>(),"Number of pings launched per tick event"),
        NULL);
}
DistributionPingScenario::DistributionPingScenario(const String &options){ 

    mContext=NULL;    
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("DistributedPingScenario",this);
    optionsSet->parse(options);
    mNumPingsPerTick=optionsSet->referenceOption("num-pings-per-tick")->as<size_t>();
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
    
    for (unsigned int i=0;i<mNumPingsPerTick;++i) {        
        Object * objA=mContext->objectHost->getObjectConnections()->randomObject((ServerID)minServer,
                                                         false);
        Object * objB=mContext->objectHost->getObjectConnections()->randomObject((ServerID)(minServer+distance),
                                                         false);
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
                                            distance*GetOption("region")->as<BoundingBox3f>().diag().x/maxDistance)) {
                
                break;
            }
        }
    }
    mPingProfiler->finished();
}
}
