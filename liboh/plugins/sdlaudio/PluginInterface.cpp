// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>
#include "SDLAudio.hpp"

static int sdlaudio_plugin_refcount = 0;

using namespace Sirikata;

namespace {

Simulation* createSDLAudio(
    Context* ctx,
    ConnectionEventProvider* cevtprovider,
    VWObjectPtr obj,
    const SpaceObjectReference& presenceid,
    const String& options
) {
    return new SDL::AudioSimulation(ctx);
}

}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    if (sdlaudio_plugin_refcount==0)
        SimulationFactory::getSingleton().registerConstructor("sdlaudio", createSDLAudio);
    sdlaudio_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++sdlaudio_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(sdlaudio_plugin_refcount>0);
    return --sdlaudio_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    if (sdlaudio_plugin_refcount==0)
        SimulationFactory::getSingleton().unregisterConstructor("sdlaudio");
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "sdlaudio";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return sdlaudio_plugin_refcount;
}
