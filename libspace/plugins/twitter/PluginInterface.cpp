// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>

#include "TwitterAggregateManager.hpp"
#include "Options.hpp"

static int space_twitter_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_TWAGGMGR_LOCAL_PATH, "", Sirikata::OptionValueType<String>(), "Path to generate local, file:// URL meshes to for helpful testing"))
        .addOption(new OptionValue(OPT_TWAGGMGR_LOCAL_URL_PREFIX, "", Sirikata::OptionValueType<String>(), "Prefix to append to locally saved meshes"))
        .addOption(new OptionValue(OPT_TWAGGMGR_GEN_THREADS, "4", Sirikata::OptionValueType<uint16>(), "Number of AggregateManager mesh generation threads"))
        .addOption(new OptionValue(OPT_TWAGGMGR_UPLOAD_THREADS, "8", Sirikata::OptionValueType<uint16>(), "Number of AggregateManager mesh upload threads"))
        ;
}

static AggregateManager* createAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username) {
    return new TwitterAggregateManager(loc, oauth, username);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_twitter_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        AggregateManagerFactory::getSingleton()
            .registerConstructor("twitter",
                std::tr1::bind(&createAggregateManager, _1, _2, _3));
    }
    space_twitter_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_twitter_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_twitter_plugin_refcount>0);
    return --space_twitter_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_twitter_plugin_refcount>0) {
        space_twitter_plugin_refcount--;
        if (space_twitter_plugin_refcount==0) {
            AggregateManagerFactory::getSingleton().unregisterConstructor("twitter");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "space-twitter";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_twitter_plugin_refcount;
}
