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
#include "GraphiteTimeSeries.hpp"

static int graphite_plugin_refcount = 0;

namespace Sirikata {
namespace Trace {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("graphite",NULL,
        new Sirikata::OptionValue("host", "127.0.0.1", Sirikata::OptionValueType<String>(), "Graphite host to report to."),
        new Sirikata::OptionValue("port", "2003", Sirikata::OptionValueType<uint16>(), "Graphite port to report to."),
        NULL);
}

static TimeSeries* createGraphiteTimeSeries(Context* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("graphite",NULL);
    optionsSet->parse(args);

    String host = optionsSet->referenceOption("host")->as<String>();
    uint16 port = optionsSet->referenceOption("port")->as<uint16>();

    return new GraphiteTimeSeries(ctx, host, port);
}

} // namespace Trace
} // namespace Sirikata

using namespace Sirikata::Trace;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (graphite_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        TimeSeriesFactory::getSingleton()
            .registerConstructor("graphite",
                std::tr1::bind(&createGraphiteTimeSeries, _1, _2));
    }
    graphite_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++graphite_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(graphite_plugin_refcount>0);
    return --graphite_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (graphite_plugin_refcount==0) {
        TimeSeriesFactory::getSingleton().unregisterConstructor("graphite");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "graphite";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return graphite_plugin_refcount;
}
