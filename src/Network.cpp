#include "Network.hpp"
#include "Options.hpp"

#include <netdb.h>
namespace CBR{

Network::Network(SpaceContext* ctx)
 : PollingService(ctx->mainStrand),
   mContext(ctx),
   mLastStatsSample(Time::null())
{
    mStatsSampleRate = GetOption(STATS_SAMPLE_RATE)->as<Duration>();

    mProfiler = mContext->profiler->addStage("Network");
}

void Network::poll() {
    mProfiler->started();
    service();

    Time curt = mContext->time;
    Duration since_last_sample = curt - mLastStatsSample;
    if (since_last_sample > mStatsSampleRate) {
        this->reportQueueInfo(curt);
        mLastStatsSample = mContext->time;
    }

    mProfiler->finished();
}

}
