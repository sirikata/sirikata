// Copyright (c) 2013 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/oh/Platform.hpp>
#include "ZernikeQueryDataLookup.hpp"

static int oh_mesh_plugin_refcount = 0;

using namespace Sirikata;

namespace {

QueryDataLookup* createZernikeQueryDataLookup(const String& opts) {
    // Ignore options

    return new ZernikeQueryDataLookup();
}

}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    if (oh_mesh_plugin_refcount==0)
        QueryDataLookupFactory::getSingleton().registerConstructor("zernike", createZernikeQueryDataLookup);
    oh_mesh_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++oh_mesh_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(oh_mesh_plugin_refcount>0);
    return --oh_mesh_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    if (oh_mesh_plugin_refcount==0)
        QueryDataLookupFactory::getSingleton().unregisterConstructor("zernike");
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-mesh";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return oh_mesh_plugin_refcount;
}
