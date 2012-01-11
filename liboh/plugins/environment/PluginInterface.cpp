// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/SimulationFactory.hpp>
#include "Environment.hpp"

static int oh_environment_plugin_refcount = 0;

using namespace Sirikata;

namespace {

Simulation* createEnvironment(
    Context* ctx,
    ConnectionEventProvider* cevtprovider,
    HostedObjectPtr obj,
    const SpaceObjectReference& presenceid,
    const String& options,
    Network::IOStrandPtr
) {
    return new EnvironmentSimulation(obj, presenceid);
}

}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    if (oh_environment_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("environment", createEnvironment);
    oh_environment_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++oh_environment_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(oh_environment_plugin_refcount>0);
    return --oh_environment_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    if (oh_environment_plugin_refcount==0)
        SimulationFactory::getSingleton().unregisterConstructor("environment");
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "environment";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return oh_environment_plugin_refcount;
}
