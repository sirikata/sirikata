/*  Sirikata
 *  PluginInterface.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PluginInterface.hpp"

#include <sirikata/mesh/Filter.hpp>
#include "LoadFilter.hpp"
#include "SaveFilter.hpp"
#include "PrintFilter.hpp"
#include "ComputeBoundsFilter.hpp"

static int common_filters_plugin_refcount = 0;

SIRIKATA_PLUGIN_EXPORT_C void init ()
{
    using namespace Sirikata;
    using namespace Sirikata::Mesh;
    if ( common_filters_plugin_refcount == 0 ) {
        FilterFactory::getSingleton().registerConstructor("load", LoadFilter::create);
        FilterFactory::getSingleton().registerConstructor("save", SaveFilter::create);
        FilterFactory::getSingleton().registerConstructor("print", PrintFilter::create);
        FilterFactory::getSingleton().registerConstructor("compute-bounds", ComputeBoundsFilter::create);
    }

    ++common_filters_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount ()
{
    return ++common_filters_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int decrefcount ()
{
    assert ( common_filters_plugin_refcount > 0 );
    return --common_filters_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy ()
{
    using namespace Sirikata;
    using namespace Sirikata::Mesh;

    if ( common_filters_plugin_refcount > 0 )
    {
        --common_filters_plugin_refcount;

        assert ( common_filters_plugin_refcount == 0 );

        if ( common_filters_plugin_refcount == 0 ) {
            FilterFactory::getSingleton().unregisterConstructor("load");
            FilterFactory::getSingleton().unregisterConstructor("save");
            FilterFactory::getSingleton().unregisterConstructor("print");
            FilterFactory::getSingleton().unregisterConstructor("compute-bounds");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C char const* name ()
{
    return "common-filters";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount ()
{
    return common_filters_plugin_refcount;
}
