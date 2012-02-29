// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "HttpCommander.hpp"
#include <sirikata/core/options/Options.hpp>

static int http_plugin_refcount = 0;

namespace Sirikata {
namespace Command {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("http",NULL,
        new Sirikata::OptionValue("host", "127.0.0.1", Sirikata::OptionValueType<String>(), "Http host to listen on."),
        new Sirikata::OptionValue("port", "8088", Sirikata::OptionValueType<uint16>(), "Http port to listen on."),
        NULL);
}

static Commander* createHttpCommander(Context* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("http",NULL);
    optionsSet->parse(args);

    String host = optionsSet->referenceOption("host")->as<String>();
    uint16 port = optionsSet->referenceOption("port")->as<uint16>();

    return new HttpCommander(ctx, host, port);
}

} // namespace Command
} // namespace Sirikata

using namespace Sirikata::Command;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (http_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        CommanderFactory::getSingleton()
            .registerConstructor("http",
                std::tr1::bind(&createHttpCommander, _1, _2));
    }
    http_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++http_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(http_plugin_refcount>0);
    return --http_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (http_plugin_refcount==0) {
        CommanderFactory::getSingleton().unregisterConstructor("http");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "http";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return http_plugin_refcount;
}
