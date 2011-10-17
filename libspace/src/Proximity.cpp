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




Proximity::Proximity(SpaceContext* ctx, LocationService* locservice, SpaceNetwork* net, AggregateManager* aggmgr, const Duration& poll_freq)
 : PollingService(ctx->mainStrand, poll_freq),
   mContext(ctx),
   mLocService(locservice),
   mCSeg(NULL),
   mAggregateManager(aggmgr),
   mStatsPoller(
       ctx->mainStrand,
       std::tr1::bind(&Proximity::reportStats, this),
       Duration::seconds((int64)1)),
   mTimeSeriesObjectQueryCountName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".prox.object_queries"),
   mTimeSeriesServerQueryCountName(String("space.server") + boost::lexical_cast<String>(ctx->id()) + ".prox.server_queries")
{
    net->addListener(this);
    mLocService->addListener(this, false);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_PROX, this);
}

Proximity::~Proximity() {
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_PROX, this);
    mLocService->removeListener(this);
}

void Proximity::initialize(CoordinateSegmentation* cseg) {
    mCSeg = cseg;
    mCSeg->addListener(this);
}

void Proximity::shutdown() {
}

void Proximity::poll() {
}

void Proximity::reportStats() {
    mContext->timeSeries->report(
        mTimeSeriesServerQueryCountName,
        serverQueries()
    );
    mContext->timeSeries->report(
        mTimeSeriesObjectQueryCountName,
        objectQueries()
    );
}

} // namespace Sirikata
