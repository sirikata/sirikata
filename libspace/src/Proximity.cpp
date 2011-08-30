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




Proximity::Proximity(SpaceContext* ctx, LocationService* locservice, SpaceNetwork* net, const Duration& poll_freq)
 : PollingService(ctx->mainStrand, poll_freq),
   mContext(ctx),
   mLocService(locservice),
   mCSeg(NULL)
{
    net->addListener(this);
}

Proximity::~Proximity() {
}

void Proximity::initialize(CoordinateSegmentation* cseg) {
    mCSeg = cseg;
    mCSeg->addListener(this);
}

void Proximity::shutdown() {
}

void Proximity::poll() {
}

} // namespace Sirikata
