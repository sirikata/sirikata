// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/SpaceModule.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::SpaceModuleFactory);

namespace Sirikata {

SpaceModule::~SpaceModule() {
}

SpaceModuleFactory& SpaceModuleFactory::getSingleton() {
    return AutoSingleton<SpaceModuleFactory>::getSingleton();
}

void SpaceModuleFactory::destroy() {
    AutoSingleton<SpaceModuleFactory>::destroy();
}

} // namespace Sirikata
