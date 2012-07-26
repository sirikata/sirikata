// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/core/util/Timer.hpp>

#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>

#include <sirikata/core/network/IOService.hpp>

#include "PintoContext.hpp"
#include "PintoManager.hpp"
#include "ManualPintoManager.hpp"

#include <sirikata/core/command/Commander.hpp>

int main(int argc, char** argv) {
    using namespace Sirikata;

    DynamicLibrary::Initialize();

    InitOptions();
    Trace::Trace::InitOptions();
    InitPintoOptions();

    ParseOptions(argc, argv, OPT_CONFIG_FILE, AllowUnregisteredOptions);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS));
    plugins.loadList( GetOptionValue<String>(OPT_EXTRA_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_PINTO_PLUGINS));

    // Fill defaults after plugin loading to ensure plugin-added
    // options get their defaults.
    FillMissingOptionDefaults();
    // Rerun original parse to make sure any newly added options are
    // properly parsed.
    ParseOptions(argc, argv, OPT_CONFIG_FILE);

    DaemonizeAndSetOutputs();
    ReportVersion(); // After options so log goes to the right place

    // Currently not distributed, so we just use any ID
    String trace_file = GetPerServerFile(STATS_TRACE_FILE, (ServerID)0);
    Trace::Trace* trace = new Trace::Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOptionValue<String>("wait-until");
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );

    Duration duration = GetOptionValue<Duration>("duration")
                        + GetOptionValue<Duration>("wait-additional");

    Network::IOService* ios = new Network::IOService("Pinto");
    Network::IOStrand* mainStrand = ios->createStrand("Pinto Main");

    PintoContext* pinto_context = new PintoContext(ios, mainStrand, trace, start_time, duration);

    String commander_type = GetOptionValue<String>(OPT_COMMAND_COMMANDER);
    String commander_options = GetOptionValue<String>(OPT_COMMAND_COMMANDER_OPTIONS);
    Command::Commander* commander = NULL;
    if (!commander_type.empty())
        commander = Command::CommanderFactory::getSingleton().getConstructor(commander_type)(pinto_context, commander_options);

    String pinto_mgr_type = GetOptionValue<String>(OPT_PINTO_TYPE);
    PintoManagerBase* pinto = NULL;
    if (pinto_mgr_type == "solidangle")
        pinto = new PintoManager(pinto_context);
    else if (pinto_mgr_type == "manual")
        pinto = new ManualPintoManager(pinto_context);
    if (pinto == NULL) {
        SILOG(pinto, fatal, "Unknown pinto type: " << pinto_mgr_type);
        return 1;
    }

    srand( GetOptionValue<uint32>("rand-seed") );

    ///////////Go go go!! start of simulation/////////////////////

    srand(time(NULL));

    pinto_context->add(pinto_context);
    pinto_context->add(pinto);

    pinto_context->run(1);

    pinto_context->cleanup();

    if (GetOptionValue<bool>(PROFILE)) {
        pinto_context->profiler->report();
    }

    trace->prepareShutdown();

    delete pinto;

    trace->shutdown();
    delete trace;
    trace = NULL;

    delete commander;
    delete pinto_context;
    pinto_context = NULL;

    delete mainStrand;
    delete ios;

    plugins.gc();

    Sirikata::Logging::finishLog();

    return 0;
}
