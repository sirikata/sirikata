// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include "TermBloomFilterQueryDataLookup.hpp"
#include <sirikata/core/options/Options.hpp>

static int oh_twitter_plugin_refcount = 0;

using namespace Sirikata;

namespace {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("twitter-term-bloom-filter", NULL,
        new Sirikata::OptionValue("buckets", "1024", Sirikata::OptionValueType<uint32>(), "Number of bloom filter buckets"),
        new Sirikata::OptionValue("hashes", "1", Sirikata::OptionValueType<uint16>(), "Number of hashes to compute for each item in the bloom filter."),
        NULL);
}

QueryDataLookup* createTermBloomFilterQueryDataLookup(const String& opts) {
    OptionSet* optionsSet = OptionSet::getOptions("twitter-term-bloom-filter",NULL);
    optionsSet->parse(opts);

    uint32 buckets = optionsSet->referenceOption("buckets")->as<uint32>();
    uint16 hashes = optionsSet->referenceOption("hashes")->as<uint16>();

    return new TermBloomFilterQueryDataLookup(buckets, hashes);
}

}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    if (oh_twitter_plugin_refcount==0) {
        InitPluginOptions();
        QueryDataLookupFactory::getSingleton().registerConstructor("twitter-term-bloom", createTermBloomFilterQueryDataLookup);
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
