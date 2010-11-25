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

namespace Sirikata {

void InitOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_defaultLevel) )
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_atLeastLevel) )
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_moduleLevel) )

        .addOption(new OptionValue(OPT_PLUGINS,"tcpsst,servermap-tabular,core-local",Sirikata::OptionValueType<String>(),"Plugin list to load."))

        .addOption(new OptionValue("ohstreamlib","tcpsst",Sirikata::OptionValueType<String>(),"Which library to use to communicate with the object host"))
        .addOption(new OptionValue("ohstreamoptions","--send-buffer-size=16384 --parallel-sockets=1 --no-delay=false",Sirikata::OptionValueType<String>(),"TCPSST stream options such as how many bytes to collect for sending during an ongoing asynchronous send call."))

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

        .addOption(new OptionValue(STATS_TRACE_FILE, "trace.txt", Sirikata::OptionValueType<String>(), "The filename to save the trace to"))

        .addOption(new OptionValue("time-server", "", Sirikata::OptionValueType<String>(), "The server to sync with"))
        .addOption(new OptionValue("wait-until","",Sirikata::OptionValueType<String>(),"The date to wait until before starting"))
        .addOption(new OptionValue("wait-additional","0s",Sirikata::OptionValueType<Duration>(),"How much additional time after date has passed to wait until before starting"))

        .addOption(new OptionValue("monitor-load", "false", Sirikata::OptionValueType<bool>(), "Does the LoadMonitor monitor queue sizes?"))


        .addOption(new OptionValue(PROFILE, "false", Sirikata::OptionValueType<bool>(), "Whether to report profiling information."))

        .addOption(new OptionValue(OPT_CDN_HOST, "cdn.sirikata.com", Sirikata::OptionValueType<String>(), "Hostname for CDN server."))
        .addOption(new OptionValue(OPT_CDN_SERVICE, "http", Sirikata::OptionValueType<String>(), "Service to access CDN by."))
      ;
}

void FakeParseOptions() {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    int argc = 1; char* argv[2] = { "bogus", NULL };
    options->parse(argc, argv);
}

void ParseOptions(int argc, char** argv) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    options->parse(argc, argv);
}

void ParseOptionsFile(const String& fname, bool required) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);
    options->parseFile(fname, required);
}

void ParseOptions(int argc, char** argv, const String& config_file_option) {
    OptionSet* options = OptionSet::getOptions(SIRIKATA_OPTIONS_MODULE,NULL);

    // Parse command line once to make sure we have the right config
    // file. On this pass, use defaults so everything gets filled in.
    options->parse(argc, argv, true);

    // Get the config file name and parse it. Don't use defaults to
    // avoid overwriting.
    String fname = GetOptionValue<String>(config_file_option.c_str());
    if (!fname.empty())
        options->parseFile(fname, true, false);

    // And parse the command line args a second time to overwrite any settings
    // the config file may have overwritten. Don't use defaults to
    // avoid overwriting.
    options->parse(argc, argv, false);
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
