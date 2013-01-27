// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/SimulationFactory.hpp>
#include "ClutterRenderer.hpp"

static int oh_clutter_plugin_refcount = 0;

using namespace Sirikata;

namespace {

Simulation* createClutterRenderer(
    Context* ctx,
    ConnectionEventProvider* cevtprovider,
    HostedObjectPtr obj,
    const SpaceObjectReference& presenceid,
    const String& options,
    Network::IOStrandPtr strand
) {
    return new ClutterRenderer(strand);
}

}

SIRIKATA_PLUGIN_EXPORT_C void init() {

    // Clutter w/ threads requires some careful setup. Just get this
    // done before anything else happens that could screw it up.
    ClutterRenderer::preinit();

    if (oh_clutter_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("clutter", createClutterRenderer);
    oh_clutter_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++oh_clutter_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(oh_clutter_plugin_refcount>0);
    return --oh_clutter_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    if (oh_clutter_plugin_refcount==0)
        SimulationFactory::getSingleton().unregisterConstructor("clutter");
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-clutter";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return oh_clutter_plugin_refcount;
}
