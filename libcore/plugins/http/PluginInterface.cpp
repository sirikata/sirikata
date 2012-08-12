// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "HttpCommander.hpp"
#include "HttpServerIDMap.hpp"
#include <sirikata/core/options/Options.hpp>

static int http_plugin_refcount = 0;

namespace Sirikata {
namespace Command {

static void InitPluginCommanderOptions() {
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

static void InitPluginServermapOptions() {
    Sirikata::InitializeClassOptions ico("servermap_http",NULL,
        // These are defaults so you only have to specify them once if they are
        // the same for internal/external. You *must* specify host if you don't
        // specify internal/external-specific versions
        new Sirikata::OptionValue("host", "", Sirikata::OptionValueType<String>(), "Host to connect to, used as default if internal/external-specific hosts are not specified"),
        new Sirikata::OptionValue("service", "http", Sirikata::OptionValueType<String>(), "Port or service name (e.g. http) to connect on, used as default if internal/external-specific ports are not specified"),

        // Internal
        new Sirikata::OptionValue("internal-host", "", Sirikata::OptionValueType<String>(), "Host to connect to for internal requests (space-to-space)"),
        new Sirikata::OptionValue("internal-service", "", Sirikata::OptionValueType<String>(), "Port or service name (e.g. http) to connect on for internal requests (space-to-space)"),
        // External
        new Sirikata::OptionValue("external-host", "", Sirikata::OptionValueType<String>(), "Host to connect to for internal requests (space-to-space)"),
        new Sirikata::OptionValue("external-service", "", Sirikata::OptionValueType<String>(), "Port or service name (e.g. http) to connect on for internal requests (space-to-space)"),


        // You *must* specify these.
        new Sirikata::OptionValue("internal-path", "/internal", Sirikata::OptionValueType<String>(), "Path to send internal lookup requests to"),
        new Sirikata::OptionValue("external-path", "/external", Sirikata::OptionValueType<String>(), "Path to send external lookup requests to"),

        new Sirikata::OptionValue("retries", "5", Sirikata::OptionValueType<uint32>(), "Number of retries to perform on a lookup before giving up and assuming the request is invalid."),

        NULL);
}

static ServerIDMap* createHttpServerIDMap(Context* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("servermap_http",NULL);
    optionsSet->parse(args);

    String internal_host = optionsSet->referenceOption("internal-host")->as<String>();
    if (internal_host.empty()) internal_host = optionsSet->referenceOption("host")->as<String>();
    String internal_service = optionsSet->referenceOption("internal-service")->as<String>();
    if (internal_service.empty()) internal_service = optionsSet->referenceOption("service")->as<String>();

    String external_host = optionsSet->referenceOption("external-host")->as<String>();
    if (external_host.empty()) external_host = optionsSet->referenceOption("host")->as<String>();
    String external_service = optionsSet->referenceOption("external-service")->as<String>();
    if (external_service.empty()) external_service = optionsSet->referenceOption("service")->as<String>();

    String internal_path = optionsSet->referenceOption("internal-path")->as<String>();
    String external_path = optionsSet->referenceOption("external-path")->as<String>();

    uint32 retries = optionsSet->referenceOption("retries")->as<uint32>();

    return new HttpServerIDMap(
        ctx,
        internal_host, internal_service, internal_path,
        external_host, external_service, external_path,
        retries
    );
}


} // namespace Sirikata

using namespace Sirikata::Command;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (http_plugin_refcount==0) {
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;

        InitPluginCommanderOptions();
        CommanderFactory::getSingleton()
            .registerConstructor("http",
                std::tr1::bind(&createHttpCommander, _1, _2));

        InitPluginServermapOptions();
        ServerIDMapFactory::getSingleton()
            .registerConstructor("http",
                std::tr1::bind(&createHttpServerIDMap, _1, _2));
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
        ServerIDMapFactory::getSingleton().unregisterConstructor("http");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "http";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return http_plugin_refcount;
}
