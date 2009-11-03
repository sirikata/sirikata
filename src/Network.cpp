#include "Network.hpp"
#include "Options.hpp"

#include <netdb.h>
namespace CBR{

Network::Network(SpaceContext* ctx)
 : PollingService(ctx->mainStrand),
   mContext(ctx)
{
    mProfiler = mContext->profiler->addStage("Network");

    mStatsPoller = new Poller(
       ctx->mainStrand,
       std::tr1::bind(&Network::reportQueueInfo, this),
       GetOption(STATS_SAMPLE_RATE)->as<Duration>()
    );
    mStatsPoller->start();
}

Network::~Network() {
    delete mStatsPoller;
}

void Network::poll() {
    mProfiler->started();
    service();
    mProfiler->finished();
}

void Network::shutdown() {
    mStatsPoller->stop();
}

}
