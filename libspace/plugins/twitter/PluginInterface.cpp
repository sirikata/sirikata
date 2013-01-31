// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/Platform.hpp>
#include <sirikata/space/AggregateManager.hpp>

#include "TwitterAggregateManager.hpp"

#include "TermBloomFilterNodeData.hpp"
#include <sirikata/pintoloc/ProxSimulationTraits.hpp>

#include "Options.hpp"

static int space_twitter_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_TWAGGMGR_LOCAL_PATH, "", Sirikata::OptionValueType<String>(), "Path to generate local, file:// URL meshes to for helpful testing"))
        .addOption(new OptionValue(OPT_TWAGGMGR_LOCAL_URL_PREFIX, "", Sirikata::OptionValueType<String>(), "Prefix to append to locally saved meshes"))
        .addOption(new OptionValue(OPT_TWAGGMGR_GEN_THREADS, "4", Sirikata::OptionValueType<uint16>(), "Number of AggregateManager mesh generation threads"))
        .addOption(new OptionValue(OPT_TWAGGMGR_UPLOAD_THREADS, "8", Sirikata::OptionValueType<uint16>(), "Number of AggregateManager mesh upload threads"))
        .addOption(new OptionValue(OPT_TWAGGMGR_BASELINE_DATA, "", Sirikata::OptionValueType<String>(), "File to load baseline term frequency data from to use to reduce weight of common terms"))
        ;

    Sirikata::InitializeClassOptions ico(SIRIKATA_OPTIONS_MODULE, NULL,
        new Sirikata::OptionValue(OPT_TWITTER_BLOOM_BUCKETS, "1024", Sirikata::OptionValueType<uint32>(), "Number of bloom filter buckets"),
        new Sirikata::OptionValue(OPT_TWITTER_BLOOM_HASHES, "1", Sirikata::OptionValueType<uint16>(), "Number of hashes to compute for each item in the bloom filter."),
        NULL);
}

static AggregateManager* createAggregateManager(LocationService* loc, Transfer::OAuthParamsPtr oauth, const String& username) {
    return new TwitterAggregateManager(loc, oauth, username);
}


static ObjectProxManualQueryHandlerFactoryType::QueryHandler* createManualRTreeTwitterQueryHandler() {
    // FIXME custom branching size
    return new Prox::RTreeManualQueryHandler<ObjectProxSimulationTraits, TermBloomFilterNodeData<ObjectProxSimulationTraits> >(10);
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

        ObjectProxManualQueryHandlerFactory.registerConstructor(
            "rtreecut", "twitter-term-bloom",
            std::tr1::bind(&createManualRTreeTwitterQueryHandler),
            false
        );

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

            ObjectProxManualQueryHandlerFactory.unregisterConstructor("rtreecut", "twitter-bloom");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "space-twitter";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_twitter_plugin_refcount;
}
