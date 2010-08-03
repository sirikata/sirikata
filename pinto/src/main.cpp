/*  Sirikata
 *  main.cpp
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

#include <sirikata/core/util/Timer.hpp>

#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>

#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>

#include "PintoContext.hpp"

int main(int argc, char** argv) {
    using namespace Sirikata;

    InitOptions();
    Trace::Trace::InitOptions();
    InitPintoOptions();
    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS));
    plugins.loadList( GetOptionValue<String>(OPT_PINTO_PLUGINS));

    // Currently not distributed, so we just use any ID
    String trace_file = GetPerServerFile(STATS_TRACE_FILE, (ServerID)0);
    Trace::Trace* trace = new Trace::Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOptionValue<String>("wait-until");
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );

    Duration duration = GetOptionValue<Duration>("duration")
                        + GetOptionValue<Duration>("wait-additional");

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* mainStrand = ios->createStrand();

    PintoContext* pinto_context = new PintoContext(ios, mainStrand, trace, start_time, duration);

    srand( GetOptionValue<uint32>("rand-seed") );

    ///////////Go go go!! start of simulation/////////////////////

    srand(time(NULL));

    pinto_context->add(pinto_context);

    pinto_context->run(2);

    pinto_context->cleanup();

    if (GetOptionValue<bool>(PROFILE)) {
        pinto_context->profiler->report();
    }

    trace->prepareShutdown();

    trace->shutdown();
    delete trace;
    trace = NULL;

    delete pinto_context;
    pinto_context = NULL;

    delete mainStrand;
    Network::IOServiceFactory::destroyIOService(ios);

    plugins.gc();

    return 0;
}
