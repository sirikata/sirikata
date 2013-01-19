// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include "ManualObjectQueryProcessor.hpp"
#include "Options.hpp"
#include <sirikata/core/options/Options.hpp>

static int oh_manual_query_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    // Note that we add directly to the top level set of options here. This is
    // because these options get sufficiently complicated (and contain their own
    // subsets of options!) that it is much easier to specify them at the top
    // level.
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_MANUAL_QUERY_SPLIT_DYNAMIC, "false", Sirikata::OptionValueType<bool>(), "If true, separate query handlers will be used for static and dynamic objects."))

        .addOption(new OptionValue(OPT_MANUAL_QUERY_HANDLER_TYPE, "rtreecutagg", Sirikata::OptionValueType<String>(), "Type of libprox query handler to use for object queries."))
        .addOption(new OptionValue(OPT_MANUAL_QUERY_HANDLER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the query handler."))
        .addOption(new OptionValue(OPT_MANUAL_QUERY_HANDLER_NODE_DATA, "maxsize", Sirikata::OptionValueType<String>(), "Per-node data in query handler, e.g. bounds, maxsize, similarmaxsize."))

        ;
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-manual-query";
}


SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++oh_manual_query_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(oh_manual_query_plugin_refcount>0);
    return --oh_manual_query_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return oh_manual_query_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    if (oh_manual_query_plugin_refcount == 0) {
        InitPluginOptions();
        Sirikata::OH::ObjectQueryProcessorFactory::getSingleton().registerConstructor(
            "manual",
            std::tr1::bind(
                &Sirikata::OH::Manual::ManualObjectQueryProcessor::create,
                _1, _2
            )
        );
    }
    oh_manual_query_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    if (oh_manual_query_plugin_refcount==0) {
        Sirikata::OH::ObjectQueryProcessorFactory::getSingleton().unregisterConstructor("manual");
    }
}
