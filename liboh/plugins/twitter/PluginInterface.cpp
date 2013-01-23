// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"

#include "TermBloomFilterQueryDataLookup.hpp"

#include "TermBloomFilterNodeData.hpp"
#include <sirikata/pintoloc/ProxSimulationTraits.hpp>
#include <sirikata/twitter/TermRegionQueryHandler.hpp>

static int oh_twitter_plugin_refcount = 0;

using namespace Sirikata;

namespace {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico(SIRIKATA_OPTIONS_MODULE, NULL,
        new Sirikata::OptionValue(OPT_TWITTER_BLOOM_BUCKETS, "1024", Sirikata::OptionValueType<uint32>(), "Number of bloom filter buckets"),
        new Sirikata::OptionValue(OPT_TWITTER_BLOOM_HASHES, "1", Sirikata::OptionValueType<uint16>(), "Number of hashes to compute for each item in the bloom filter."),
        NULL);
}

QueryDataLookup* createTermBloomFilterQueryDataLookup(const String& opts) {
    uint32 buckets = GetOptionValue<uint32>(OPT_TWITTER_BLOOM_BUCKETS);
    uint16 hashes = GetOptionValue<uint16>(OPT_TWITTER_BLOOM_HASHES);

    return new TermBloomFilterQueryDataLookup(buckets, hashes);
}


// These ones aren't really that useful since they only do geometric queries,
// but are useful for testing
static ObjectProxGeomQueryHandlerFactoryType::QueryHandler* createGeomRTreeCutTwitterQueryHandler() {
    // FIXME custom branching size
    return new Prox::RTreeCutQueryHandler<ObjectProxSimulationTraits, TermBloomFilterNodeData<ObjectProxSimulationTraits> >(10, false);
}
static ObjectProxGeomQueryHandlerFactoryType::QueryHandler* createGeomRTreeCutAggTwitterQueryHandler() {
    // FIXME custom branching size
    return new Prox::RTreeCutQueryHandler<ObjectProxSimulationTraits, TermBloomFilterNodeData<ObjectProxSimulationTraits> >(10, true);
}
// This one's the real useful one
static ObjectProxGeomQueryHandlerFactoryType::QueryHandler* createGeomTermRegionTwitterQueryHandler() {
    // FIXME custom branching size
    return new TermRegionQueryHandler<ObjectProxSimulationTraits, TermBloomFilterNodeData<ObjectProxSimulationTraits> >(10);
}
}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    if (oh_twitter_plugin_refcount==0) {
        InitPluginOptions();
        QueryDataLookupFactory::getSingleton().registerConstructor("twitter-term-bloom", createTermBloomFilterQueryDataLookup);

        ObjectProxGeomQueryHandlerFactory.registerConstructor(
            "rtreecut", "twitter-term-bloom",
            std::tr1::bind(&createGeomRTreeCutTwitterQueryHandler),
            false
        );
        ObjectProxGeomQueryHandlerFactory.registerConstructor(
            "rtreecutagg", "twitter-term-bloom",
            std::tr1::bind(&createGeomRTreeCutAggTwitterQueryHandler),
            false
        );
        ObjectProxGeomQueryHandlerFactory.registerConstructor(
            "term-region", "twitter-term-bloom",
            std::tr1::bind(&createGeomTermRegionTwitterQueryHandler),
            false
        );

    }
    oh_twitter_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++oh_twitter_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(oh_twitter_plugin_refcount>0);
    return --oh_twitter_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    if (oh_twitter_plugin_refcount==0)
        QueryDataLookupFactory::getSingleton().unregisterConstructor("twitter-term-bloom");
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-twitter";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return oh_twitter_plugin_refcount;
}
