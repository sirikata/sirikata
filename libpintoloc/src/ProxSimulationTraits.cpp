// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/pintoloc/ProxSimulationTraits.hpp>

#include <float.h>

namespace Sirikata {

const ProxSimulationTraits::realType ProxSimulationTraits::InfiniteRadius = FLT_MAX;

const ProxSimulationTraits::intType ProxSimulationTraits::InfiniteResults = INT_MAX;

SIRIKATA_LIBPINTOLOC_EXPORT GeomQueryHandlerFactory<ObjectProxSimulationTraits> ObjectProxGeomQueryHandlerFactory;
SIRIKATA_LIBPINTOLOC_EXPORT ManualQueryHandlerFactory<ObjectProxSimulationTraits> ObjectProxManualQueryHandlerFactory;

} // namespace Sirikata
