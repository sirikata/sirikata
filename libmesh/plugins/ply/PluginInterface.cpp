// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/mesh/Platform.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>
#include "PlyModelSystem.hpp"

static int ply_plugin_refcount = 0;

namespace {
Sirikata::ModelsSystem* createPlyModelSystem(const Sirikata::String & options) {
    return new Sirikata::PlyModelSystem();
}
}

SIRIKATA_PLUGIN_EXPORT_C void init ()
{
    using namespace Sirikata;
    if ( ply_plugin_refcount == 0 )
        ModelsSystemFactory::getSingleton ().registerConstructor
            ( "mesh-ply" , &createPlyModelSystem, true );

    ++ply_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount ()
{
    return ++ply_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int decrefcount ()
{
    assert ( ply_plugin_refcount > 0 );
    return --ply_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy ()
{
    using namespace Sirikata;

    if ( ply_plugin_refcount > 0 )
    {
        --ply_plugin_refcount;

        assert ( ply_plugin_refcount == 0 );

        if ( ply_plugin_refcount == 0 )
            ModelsSystemFactory::getSingleton ().unregisterConstructor ( "mesh-ply" );
    }
}

SIRIKATA_PLUGIN_EXPORT_C char const* name ()
{
    return "mesh-ply";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount ()
{
    return ply_plugin_refcount;
}
