#include "DistributionPingScenario.hpp"
#include "ScenarioFactory.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"
#include "Options.hpp"
namespace CBR{
void DPSInitOptions(DistributionPingScenario *thus) {

    Sirikata::InitializeClassOptions ico("DistributedPingScenario",thus,
                                         new OptionValue("num-pings-per-second","1000",Sirikata::OptionValueType<size_t>(),"Number of pings launched per simulation second"),
                                         new OptionValue("allow-same-object-host","false",Sirikata::OptionValueType<bool>(),"allow pings to occasionally hit the same object host you are on"),
                                         new OptionValue("force-same-object-host","false",Sirikata::OptionValueType<bool>(),"force pings to only go through 1 spaec server hop"),
        NULL);
}
DistributionPingScenario::DistributionPingScenario(const String &options):mStartTime(Time::epoch()){
    mNumTotalPings=0;
    mContext=NULL;
    mPingID=0;
    DPSInitOptions(this);
    OptionSet* optionsSet = OptionSet::getOptions("DistributedPingScenario",this);
    optionsSet->parse(options);
    mNumPingsPerSecond=optionsSet->referenceOption("num-pings-per-second")->as<size_t>();
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
    mPingPoller = new Poller(ctx->mainStrand, std::tr1::bind(&DistributionPingScenario::generatePings, this), Duration::seconds(1.0/mNumPingsPerSecond));
}

void DistributionPingScenario::start() {
    mPingPoller->start();
}
void DistributionPingScenario::stop() {
    mPingPoller->stop();
}
bool DistributionPingScenario::pingOne(ServerID minServer, unsigned int distance) {
    unsigned int maxDistance=mContext->objectHost->getObjectConnections()->numServerIDs();
    Object * objA=mContext->objectHost->getObjectConnections()->randomObject((ServerID)minServer,
                                                                             false);
    Object * objB=mContext->objectHost->getObjectConnections()->randomObject((ServerID)(minServer+distance),
                                                                             false);

    std::cout<<"\n\nbftm: This is distance:  "<<distance<<"\n\n";
    

    if (rand()<RAND_MAX/2) {
        Object * tmp=objA;
        objA=objB;
        objB=tmp;
    }
    Time t(mContext->simTime());
    if (objA&&objB) {
        if (!mContext->objectHost->ping(t,
                                        objA,
                                        objB->uuid(),
                                        distance*GetOption("region")->as<BoundingBox3f>().diag().x/maxDistance)) {

            return false;
        }
    }
    return true;
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
    Time newTime=mContext->simTime();
    int64 howManyPings=(newTime-mStartTime).toSeconds()*mNumPingsPerSecond;
    

    bool broke=false;
    int64 limit=howManyPings-mNumTotalPings;
    int64 i;
    for (i=0;i<limit;++i) {
        if (!pingOne(minServer,distance)) {
            break;
        }
    }
    mNumTotalPings+=i;
    mPingProfiler->finished();
    static bool printed=false;
    if (i-limit>10*mNumPingsPerSecond&&!printed) {
        SILOG(oh,debug,"[OH] " << i-limit<<" pending ");
        printed=true;
    }
}
}
