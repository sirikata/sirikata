// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include "ManualObjectQueryProcessor.hpp"

static int oh_manual_query_plugin_refcount = 0;


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
