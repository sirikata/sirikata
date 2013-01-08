// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::AggregateManagerFactory);

namespace Sirikata {

AggregateManagerFactory& AggregateManagerFactory::getSingleton() {
    return AutoSingleton<AggregateManagerFactory>::getSingleton();
}

void AggregateManagerFactory::destroy() {
    AutoSingleton<AggregateManagerFactory>::destroy();
}

} // namespace Sirikata
