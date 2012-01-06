// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/SpaceModule.hpp>
#include "Environment.hpp"
#include <sirikata/core/options/Options.hpp>

static int space_environment_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("space_environment", NULL,
        NULL);
}

static SpaceModule* createEnvironment(SpaceContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("space_environment",NULL);
    optionsSet->parse(args);

    return new Environment(ctx);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_environment_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        using std::tr1::placeholders::_4;
        using std::tr1::placeholders::_5;
        SpaceModuleFactory::getSingleton()
            .registerConstructor("environment",
                std::tr1::bind(&createEnvironment, _1, _2));
    }
    space_environment_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_environment_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_environment_plugin_refcount>0);
    return --space_environment_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_environment_plugin_refcount==0) {
        SpaceModuleFactory::getSingleton().unregisterConstructor("environment");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "environment";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_environment_plugin_refcount;
}
