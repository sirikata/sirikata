/*  Sirikata
 *  GenPack.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include <sirikata/core/util/Timer.hpp>

#include "Object.hpp"
#include "ObjectFactory.hpp"

#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/oh/Trace.hpp>

#include <sirikata/core/network/IOServiceFactory.hpp>

#include <sirikata/oh/ObjectHostContext.hpp>

void *main_loop(void *);
int main(int argc, char** argv) {
    using namespace Sirikata;

    InitOptions();
    Trace::Trace::InitOptions();
    OHTrace::InitOptions();
    InitSimOHOptions();
    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_OH_PLUGINS) );

    String trace_file = GetPerServerFile(STATS_OH_TRACE_FILE, 1);
    Trace::Trace* gTrace = new Trace::Trace(trace_file);

    BoundingBox3f region = GetOptionValue<BoundingBox3f>("region");
    Duration duration = GetOptionValue<Duration>("duration");

    // Get the starting time
    Time start_time = Timer::now();

    srand( GetOptionValue<uint32>("rand-seed") );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* mainStrand = ios->createStrand();

    ObjectHostContext* ctx = new ObjectHostContext(ObjectHostID(1), ios, mainStrand, gTrace, start_time, duration);
    ObjectFactory* obj_factory = new ObjectFactory(ctx, region, duration);

    // Nothing actually runs here -- we only cared about getting the
    // object factory setup so it could dump the object pack data.

    gTrace->prepareShutdown();

    delete obj_factory;
    delete ctx;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;

    delete mainStrand;
    Network::IOServiceFactory::destroyIOService(ios);

    return 0;
}
