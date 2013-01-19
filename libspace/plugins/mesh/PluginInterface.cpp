// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>

#include "MeshAggregateManager.hpp"
#include "Options.hpp"
#include <sirikata/core/options/Options.hpp>

static int space_mesh_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_AGGMGR_LOCAL_PATH, "", Sirikata::OptionValueType<String>(), "Path to generate local, file:// URL meshes to for helpful testing"))
        .addOption(new OptionValue(OPT_AGGMGR_LOCAL_URL_PREFIX, "", Sirikata::OptionValueType<String>(), "Prefix to append to locally saved meshes"))
        .addOption(new OptionValue(OPT_AGGMGR_GEN_THREADS, "4", Sirikata::OptionValueType<uint16>(), "Number of AggregateManager mesh generation threads"))
        .addOption(new OptionValue(OPT_AGGMGR_UPLOAD_THREADS, "8", Sirikata::OptionValueType<uint16>(), "Number of AggregateManager mesh upload threads"))
        .addOption(new OptionValue(OPT_AGGMGR_SKIP_GENERATE, "false", Sirikata::OptionValueType<bool>(), "If true, skips generating but pretends it was always successful. Useful for testing without the overhead of generating aggregates."))
        .addOption(new OptionValue(OPT_AGGMGR_SKIP_UPLOAD, "false", Sirikata::OptionValueType<bool>(), "If true, skips uploading but pretends it was always successful. Useful for testing without pushing data to the CDN."))
        ;
}

static AggregateManager* createAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username) {
    return new MeshAggregateManager(loc, oauth, username);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_mesh_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        AggregateManagerFactory::getSingleton()
            .registerConstructor("mesh",
                std::tr1::bind(&createAggregateManager, _1, _2, _3));
    }
    space_mesh_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_mesh_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_mesh_plugin_refcount>0);
    return --space_mesh_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_mesh_plugin_refcount>0) {
        space_mesh_plugin_refcount--;
        if (space_mesh_plugin_refcount==0) {
            AggregateManagerFactory::getSingleton().unregisterConstructor("mesh");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "mesh";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_mesh_plugin_refcount;
}
