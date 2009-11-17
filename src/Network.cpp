#include "Network.hpp"
#include "Options.hpp"

#include <netdb.h>
namespace CBR{

Network::Network(SpaceContext* ctx)
 : mContext(ctx),
   mServerIDMap(NULL)
{
    mStatsPoller = new Poller(
       ctx->mainStrand,
       std::tr1::bind(&Network::reportQueueInfo, this),
       GetOption(STATS_SAMPLE_RATE)->as<Duration>()
    );
}

Network::~Network() {
    delete mStatsPoller;
}

void Network::start() {
    mStatsPoller->start();
}

void Network::stop() {
    mStatsPoller->stop();
}

void Network::setServerIDMap(ServerIDMap* sidmap) {
    mServerIDMap = sidmap;
}

}
