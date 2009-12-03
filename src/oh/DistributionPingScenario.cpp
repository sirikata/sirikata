#include "DistributionPingScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include "Options.hpp"
namespace CBR{
void DPSInitOptions(DistributionPingScenario *thus) {
   
    Sirikata::InitializeClassOptions ico("DistributedPingScenario",thus,
                                         new OptionValue("num-pings-per-tick","1000",Sirikata::OptionValueType<size_t>(),"Number of pings launched per tick event"),
                                         new OptionValue("allow-same-object-host","false",Sirikata::OptionValueType<bool>(),"allow pings to occasionally hit the same object host you are on"),
                                         new OptionValue("force-same-object-host","false",Sirikata::OptionValueType<bool>(),"force pings to only go through 1 spaec server hop"),
        NULL);
}
DistributionPingScenario::DistributionPingScenario(const String &options){ 

    mContext=NULL;    
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("DistributedPingScenario",this);
    optionsSet->parse(options);
    mNumPingsPerTick=optionsSet->referenceOption("num-pings-per-tick")->as<size_t>();
    mSameObjectHostPings=optionsSet->referenceOption("allow-same-object-host")->as<bool>();
    mForceSameObjectHostPings=optionsSet->referenceOption("force-same-object-host")->as<bool>();
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
    if (maxDistance&&((!mSameObjectHostPings)&&(!mForceSameObjectHostPings)))
        maxDistance-=1;
    if (maxDistance>1&&!mForceSameObjectHostPings) {
        distance=(rand()%maxDistance);//uniform distance at random
        if (!mSameObjectHostPings)
            distance+=1;
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
