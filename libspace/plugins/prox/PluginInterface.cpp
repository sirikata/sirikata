/*  Sirikata
 *  PluginInterface.cpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/space/ObjectSegmentation.hpp>
#include "LibproxProximity.hpp"
#include "LibproxManualProximity.hpp"
#include "Options.hpp"

static int space_prox_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    // Note that we add directly to the top level set of options here. This is
    // because these options get sufficiently complicated (and contain their own
    // subsets of options!) that it is much easier to specify them at the top
    // level.
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_PINTO,"local",Sirikata::OptionValueType<String>(),"Specifies which type of Pinto to use."))
        .addOption(new OptionValue(OPT_PINTO_OPTIONS,"",Sirikata::OptionValueType<String>(),"Specifies arguments to Pinto."))

        .addOption(new OptionValue(PROX_MAX_PER_RESULT, "5", Sirikata::OptionValueType<uint32>(), "Maximum number of changes to report in each result message."))

        .addOption(new OptionValue(OPT_PROX_SPLIT_DYNAMIC, "true", Sirikata::OptionValueType<bool>(), "If true, separate query handlers will be used for static and dynamic objects."))

        .addOption(new OptionValue(OPT_PROX_QUERY_RANGE, "100", Sirikata::OptionValueType<float32>(), "The range of queries when using range queries instead of solid angle queries."))

        .addOption(new OptionValue(OPT_PROX_SERVER_QUERY_HANDLER_TYPE, "rtreecut", Sirikata::OptionValueType<String>(), "Type of libprox query handler to use for queries from servers."))
        .addOption(new OptionValue(OPT_PROX_SERVER_QUERY_HANDLER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the query handler."))

        .addOption(new OptionValue(OPT_PROX_OBJECT_QUERY_HANDLER_TYPE, "rtreecut", Sirikata::OptionValueType<String>(), "Type of libprox query handler to use for queries from servers."))
        .addOption(new OptionValue(OPT_PROX_OBJECT_QUERY_HANDLER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the query handler."))

        ;
}

static Proximity* createProx(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr, const String& args) {
    // This implementation doesn't currently parse any options -- the command
    // line parsing takes acer of it for us since we insert options into the
    // main option set.
    return new LibproxProximity(ctx, locservice, cseg, net, aggmgr);
}

static Proximity* createManualProx(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr, const String& args) {
    // This implementation doesn't currently parse any options -- the command
    // line parsing takes acer of it for us since we insert options into the
    // main option set.
    return new LibproxManualProximity(ctx, locservice, cseg, net, aggmgr);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_prox_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        using std::tr1::placeholders::_4;
        using std::tr1::placeholders::_5;
        using std::tr1::placeholders::_6;
        ProximityFactory::getSingleton()
            .registerConstructor("libprox",
                std::tr1::bind(&createProx, _1, _2, _3, _4, _5, _6));
        ProximityFactory::getSingleton()
            .registerConstructor("libprox-manual",
                std::tr1::bind(&createManualProx, _1, _2, _3, _4, _5, _6));
    }
    space_prox_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_prox_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_prox_plugin_refcount>0);
    return --space_prox_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_prox_plugin_refcount==0) {
        ProximityFactory::getSingleton().unregisterConstructor("libprox");
        ProximityFactory::getSingleton().unregisterConstructor("libprox-manual");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "prox";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_prox_plugin_refcount;
}
