// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#include <sirikata/space/Proximity.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::ProximityFactory);

namespace Sirikata {

ProximityFactory& ProximityFactory::getSingleton() {
    return AutoSingleton<ProximityFactory>::getSingleton();
}

void ProximityFactory::destroy() {
    AutoSingleton<ProximityFactory>::destroy();
}




Proximity::Proximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr, const Duration& poll_freq)
 : PollingService(ctx->mainStrand, "Proximity Poll", poll_freq),
   mContext(ctx),
   mLocService(locservice),
   mCSeg(cseg),
   mAggregateManager(aggmgr),
   mStatsPoller(
       ctx->mainStrand,
       std::tr1::bind(&Proximity::reportStats, this),
       "Proximity Stats Poller",
       Duration::seconds((int64)1)),
   mTimeSeriesObjectQueryCountName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".prox.object_queries"),
   mTimeSeriesObjectHostQueryCountName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".prox.object_host_queries"),
   mTimeSeriesServerQueryCountName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".prox.server_queries")
{
    net->addListener(this);
    mLocService->addListener(this, false);
    mCSeg->addListener(this);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_PROX, this);
    mContext->objectSessionManager()->addListener(this);
    mContext->ohSessionManager()->addListener(this);
}

Proximity::~Proximity() {
    mContext->ohSessionManager()->removeListener(this);
    mContext->objectSessionManager()->removeListener(this);
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_PROX, this);
    mLocService->removeListener(this);
}

void Proximity::start() {
    PollingService::start();
    mStatsPoller.start();
}

void Proximity::stop() {
    PollingService::stop();
    mStatsPoller.stop();
}

void Proximity::poll() {
}

void Proximity::reportStats() {
    mContext->timeSeries->report(
        mTimeSeriesServerQueryCountName,
        serverQueries()
    );
    mContext->timeSeries->report(
        mTimeSeriesObjectHostQueryCountName,
        objectHostQueries()
    );
    mContext->timeSeries->report(
        mTimeSeriesObjectQueryCountName,
        objectQueries()
    );
}

} // namespace Sirikata
