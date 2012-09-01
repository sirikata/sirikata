/*  Sirikata
 *  CommonOptions.cpp
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

#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/util/Timer.hpp>

// daemon() method, getpid
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#elif SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
#include <sys/types.h>
#include <unistd.h>
#endif


namespace Sirikata {

void ReportVersion() {
    bool do_version = GetOptionValue<bool>("version");
    if (!do_version) return;
    SILOG(core,info,"Sirikata started at " << Timer::nowAsString());
    SILOG(core,info,"Sirikata version " << SIRIKATA_VERSION << " (git #" << SIRIKATA_GIT_REVISION << ")");
}


void InitOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_defaultLevel) )
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_atLeastLevel) )
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_moduleLevel) )

        .addOption(new OptionValue("version","true",Sirikata::OptionValueType<bool>(),"Report version number."))

        .addOption(new OptionValue(OPT_CRASHREPORT_URL,"http://crashes.sirikata.com/report",Sirikata::OptionValueType<String>(),"URL to report crashes to."))

        .addOption(new OptionValue(OPT_PLUGINS,"tcpsst,servermap-tabular,core-local,graphite,core-http",Sirikata::OptionValueType<String>(),"Plugin list to load."))
        .addOption(new OptionValue(OPT_EXTRA_PLUGINS,"",Sirikata::OptionValueType<String>(),"Extra list of plugins to load. Useful for using existing defaults as well as some additional plugins."))

        .addOption(new OptionValue("ohstreamlib","tcpsst",Sirikata::OptionValueType<String>(),"Which library to use to communicate with the object host"))
        .addOption(new OptionValue("ohstreamoptions","--send-buffer-size=16384 --parallel-sockets=1 --no-delay=false",Sirikata::OptionValueType<String>(),"TCPSST stream options such as how many bytes to collect for sending during an ongoing asynchronous send call."))

        .addOption(new OptionValue(OPT_SST_DEFAULT_WINDOW_SIZE,"10000",Sirikata::OptionValueType<uint32>(),"Default window (and buffer) size for SST streams."))

        .addOption(new OptionValue(OPT_REGION_WEIGHT, "sqr", Sirikata::OptionValueType<String>(), "Type of region weight calculator to use, which affects communication falloff."))
        .addOption(new OptionValue(OPT_REGION_WEIGHT_ARGS, "--flatness=8 --const-cutoff=64", Sirikata::OptionValueType<String>(), "Arguments to region weight calculator."))

        .addOption(new OptionValue("region", "<<1,1,1>,<-1,-1,-1>>", Sirikata::OptionValueType<BoundingBox3f>(), "Simulation region"))
        .addOption(new OptionValue("layout", "<1,1,1>", Sirikata::OptionValueType<Vector3ui32>(), "Layout of servers in uniform grid - ixjxk servers"))
        .addOption(new OptionValue("max-servers", "0", Sirikata::OptionValueType<uint32>(), "Maximum number of servers available for the simulation; if set to 0, use the number of servers specified in the layout option"))
        .addOption(new OptionValue("duration", "0s", Sirikata::OptionValueType<Duration>(), "Duration of the simulation"))

        .addOption(new OptionValue("servermap", "local", Sirikata::OptionValueType<String>(), "The type of ServerIDMap to instantiate."))
        .addOption(new OptionValue("servermap-options", "", Sirikata::OptionValueType<String>(), "Options to pass to the ServerIDMap constructor."))

        .addOption(new OptionValue("capexcessbandwidth", "false", Sirikata::OptionValueType<bool>(), "Total bandwidth for this server in bytes per second"))

        .addOption(new OptionValue("rand-seed", "0", Sirikata::OptionValueType<uint32>(), "The random seed to synchronize all servers"))

        .addOption(new OptionValue(OPT_LOG_FILE, "", Sirikata::OptionValueType<String>(), "Filename to log SILOG messages to. If empty or -, uses stderr"))
        .addOption(new OptionValue(OPT_LOG_ALL_TO_FILE, "true", Sirikata::OptionValueType<bool>(), "If true and a log file is specified, redirect all output o it, including stdout and stderr, instead of just SILOG messages."))
        .addOption(new OptionValue(OPT_DAEMON, "false", Sirikata::OptionValueType<bool>(), "If true, daemonize this process"))
        .addOption(new OptionValue(OPT_PID_FILE, "", Sirikata::OptionValueType<String>(), "Filename to write the process ID to. If empty, no pid file will be written."))

        .addOption(new OptionValue(STATS_TRACE_FILE, "trace.txt", Sirikata::OptionValueType<String>(), "The filename to save the trace to"))

        .addOption(new OptionValue("time-server", "", Sirikata::OptionValueType<String>(), "The server to sync with"))
        .addOption(new OptionValue("wait-until","",Sirikata::OptionValueType<String>(),"The date to wait until before starting"))
        .addOption(new OptionValue("wait-additional","0s",Sirikata::OptionValueType<Duration>(),"How much additional time after date has passed to wait until before starting"))

        .addOption(new OptionValue("monitor-load", "false", Sirikata::OptionValueType<bool>(), "Does the LoadMonitor monitor queue sizes?"))


        .addOption(new OptionValue(PROFILE, "false", Sirikata::OptionValueType<bool>(), "Whether to report profiling information."))

        .addOption(new OptionValue(OPT_CDN_HOST,"open3dhub.com",Sirikata::OptionValueType<String>(), "Hostname for CDN server."))

        .addOption(new OptionValue(OPT_CDN_SERVICE, "http", Sirikata::OptionValueType<String>(), "Service to access CDN by."))
        .addOption(new OptionValue(OPT_CDN_DNS_URI_PREFIX, "/dns", Sirikata::OptionValueType<String>(), "URI prefix for CDN HTTP name looksup."))
        .addOption(new OptionValue(OPT_CDN_DOWNLOAD_URI_PREFIX, "/download", Sirikata::OptionValueType<String>(), "URI prefix for CDN HTTP downloads."))
        .addOption(new OptionValue(OPT_CDN_UPLOAD_URI_PREFIX, "/api/upload", Sirikata::OptionValueType<String>(), "URI prefix for CDN HTTP uploads."))
        .addOption(new OptionValue(OPT_CDN_UPLOAD_STATUS_URI_PREFIX, "/upload/processing", Sirikata::OptionValueType<String>(), "URI prefix for CDN HTTP upload status checks."))

        .addOption(new OptionValue(OPT_TRACE_TIMESERIES, "null", Sirikata::OptionValueType<String>(), "Service to report TimeSeries data to."))
        .addOption(new OptionValue(OPT_TRACE_TIMESERIES_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for TimeSeries reporting service."))

        .addOption(new OptionValue(OPT_COMMAND_COMMANDER, "", Sirikata::OptionValueType<String>(), "Commander service to start"))
        .addOption(new OptionValue(OPT_COMMAND_COMMANDER_OPTIONS, "", Sirikata::OptionValueType<String>(), "Options for the Commander service"))
      ;
}



void FakeParseOptions() {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    int argc = 1; const char* argv[2] = { "bogus", NULL };
    options->parse(argc, argv);
}

void ParseOptions(int argc, char** argv, UnregisteredOptionBehavior unreg) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    options->parse(argc, argv, true, false, (unreg == AllowUnregisteredOptions));
}

void ParseOptionsFile(const String& fname, bool required, UnregisteredOptionBehavior unreg) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    options->parseFile(fname, required, true, false, (unreg == AllowUnregisteredOptions));
}

void ParseOptions(int argc, char** argv, const String& config_file_option, UnregisteredOptionBehavior unreg) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);

    // Parse command line once to make sure we have the right config
    // file. On this pass, use defaults so everything gets filled in.
    options->parse(argc, argv, true, false, (unreg == AllowUnregisteredOptions));

    // Get the config file name and parse it. Don't use defaults to
    // avoid overwriting.
    String fname = GetOptionValue<String>(config_file_option.c_str());
    if (!fname.empty())
        options->parseFile(fname, false, false, false, (unreg == AllowUnregisteredOptions));

    // And parse the command line args a second time to overwrite any settings
    // the config file may have overwritten. Don't use defaults to
    // avoid overwriting.
    options->parse(argc, argv, false, false, (unreg == AllowUnregisteredOptions));
}

void FillMissingOptionDefaults() {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);

    // Parse command line once to make sure we have the right config
    // file. On this pass, use defaults so everything gets filled in.
    options->fillMissingDefaults();
}

void DaemonizeAndSetOutputs() {
    // We only have daemon() on Unices
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
    if (GetOptionValue<bool>(OPT_DAEMON)) {
        // We don't chdir to / because we don't have settings for getting out of
        // there. The user has to start the daemon in a good location.
        daemon(1 /* don't chdir to / */, 0 /* do redirect stdin/out/err to
                                            * /dev/null */);
    }
#endif

    // Set log output
    {
        String logfile = GetOptionValue<String>(OPT_LOG_FILE);
        bool log_all_to_file = GetOptionValue<bool>(OPT_LOG_ALL_TO_FILE);
        bool changed = false;
        if (logfile != "" && logfile != "-") {
            if (log_all_to_file) {
                FILE* fp = fopen(logfile.c_str(), "w");
                if (fp != NULL) {
                    Sirikata::Logging::setOutputFP(fp);
                    changed = true;
                }
            }
            else {
                // Try to open the log file
                std::ostream* logfs = new std::ofstream(logfile.c_str(), std::ios_base::out | std::ios_base::app);
                if (*logfs) {
                    Sirikata::Logging::setLogStream(logfs);
                    changed = true;
                }
            }
        }
        // If that failed, go back to cerr
        if (!changed)
            Sirikata::Logging::SirikataLogStream = &std::cerr;
    }

    // Write pid file if requested
    {
        String pidfile = GetOptionValue<String>(OPT_PID_FILE);
        if (pidfile != "") {
            std::ofstream pidfs(pidfile.c_str(), std::ios_base::out);
            if (!pidfs) {
                SILOG(pid, error, "Couldn't write to requested PID file: " << pidfile);
            }
            else {
#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_MAC || SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_LINUX
                pidfs << getpid() << std::endl;
#endif
            }
        }
    }
}

OptionValue* GetOption(const char* name) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    return options->referenceOption(name);
}

OptionValue* GetOption(const char* klass, const char* name) {
    OptionSet* options = OptionSet::getOptions(klass,NULL);
    return options->referenceOption(name);
}

template<typename T>
T GetOptionValueUnsafe(const char* name) {
    OptionValue* opt = GetOption(name);
    return opt->unsafeAs<T>();
}

#define DEFINE_UNSAFE_GETOPTIONVALUE(T) \
    template<>                                                          \
    T GetOptionValue<T>(const char* name) { return GetOptionValueUnsafe<T>(name); }

DEFINE_UNSAFE_GETOPTIONVALUE(String)
DEFINE_UNSAFE_GETOPTIONVALUE(Vector3f)
DEFINE_UNSAFE_GETOPTIONVALUE(Vector3ui32)
DEFINE_UNSAFE_GETOPTIONVALUE(BoundingBox3f)
DEFINE_UNSAFE_GETOPTIONVALUE(ObjectHostID)
DEFINE_UNSAFE_GETOPTIONVALUE(Task::DeltaTime)
DEFINE_UNSAFE_GETOPTIONVALUE(uint32)
DEFINE_UNSAFE_GETOPTIONVALUE(int32)
DEFINE_UNSAFE_GETOPTIONVALUE(uint64)
DEFINE_UNSAFE_GETOPTIONVALUE(int64)
DEFINE_UNSAFE_GETOPTIONVALUE(bool)

// FIXME method naming
String GetPerServerString(const String& orig, const ServerID& sid) {
    int32 dot_pos = orig.rfind(".");
    String prefix = orig.substr(0, dot_pos);
    String ext = orig.substr(dot_pos+1);

    char buffer[1024];
    sprintf(buffer, "%s-%04d.%s", prefix.c_str(), (uint32)sid, ext.c_str());

    return buffer;
}

String GetPerServerFile(const char* opt_name, const ServerID& sid) {
    String orig = GetOptionValue<String>(opt_name);
    return GetPerServerString(orig, sid);
}

String GetPerServerFile(const char* opt_name, const ObjectHostID& ohid) {
    return (GetPerServerFile(opt_name, (ServerID)ohid.id)); // FIXME relies on fact that ServerID and ObjectHostID are both uint64
}

} // namespace Sirikata
