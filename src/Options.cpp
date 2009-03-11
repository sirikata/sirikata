/*  cbr
 *  Options.cpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "Options.hpp"
#include "Duration.hpp"
#include "BoundingBox.hpp"

namespace CBR {

void InitOptions() {
    InitializeOptions::module(CBR_MODULE)
        .addOption(new OptionValue("test", "none", Sirikata::OptionValueType<String>(), "Type of test to run"))
        .addOption(new OptionValue("server-port", "8080", Sirikata::OptionValueType<String>(), "Port for server side of test"))
        .addOption(new OptionValue("client-port", "8081", Sirikata::OptionValueType<String>(), "Port for client side of test"))
        .addOption(new OptionValue("host", "127.0.0.1", Sirikata::OptionValueType<String>(), "Host to connect to for test"))

        .addOption(new OptionValue("sim", "false", Sirikata::OptionValueType<bool>(), "Turns simulated clock time on or off"))
        .addOption(new OptionValue("sim-step", "10ms", Sirikata::OptionValueType<Duration>(), "Size of simulation time steps"))

        .addOption(new OptionValue("time-dilation", "1.0", Sirikata::OptionValueType<float>(), "Factor by which times will be scaled (to allow faster processing when CPU and bandwidth is readily available, slower when they are overloaded)"))

        .addOption(new OptionValue("id", "1", Sirikata::OptionValueType<ServerID>(), "Server ID for this server"))
        .addOption(new OptionValue("objects", "100", Sirikata::OptionValueType<uint32>(), "Number of objects to simulate"))
        .addOption(new OptionValue("region", "<<-100,-100,-100>,<100,100,100>>", Sirikata::OptionValueType<BoundingBox3f>(), "Simulation region"))
        .addOption(new OptionValue("layout", "<2,1,1>", Sirikata::OptionValueType<Vector3ui32>(), "Layout of servers in uniform grid - ixjxk servers"))
        .addOption(new OptionValue("duration", "1s", Sirikata::OptionValueType<Duration>(), "Duration of the simulation"))
        .addOption(new OptionValue("serverips", "serverip.txt", Sirikata::OptionValueType<String>(), "The file containing the server ip list"))

        .addOption(new OptionValue("bandwidth", "2000000000", Sirikata::OptionValueType<uint32>(), "Total bandwidth for this server in bytes per second"))

        .addOption(new OptionValue("rand-seed", "0", Sirikata::OptionValueType<uint32>(), "The random seed to synchronize all servers"))

        .addOption(new OptionValue(STATS_BANDWIDTH_FILE, "bandwidth.txt", Sirikata::OptionValueType<String>(), "The filename to save bandwidth stats to"))
        .addOption(new OptionValue(STATS_LOCATION_FILE, "location.txt", Sirikata::OptionValueType<String>(), "The filename to save location stats to"))

        .addOption(new OptionValue(MAX_EXTRAPOLATOR_DIST, "1.0", Sirikata::OptionValueType<float64>(), "The maximum distance an object is permitted to deviate from the predictions by other objects before an update is sent out."))
     ;
}

void ParseOptions(int argc, char** argv) {
    OptionSet* options = OptionSet::getOptions(CBR_MODULE);
    options->parse(argc, argv);
}

OptionValue* GetOption(const char* name) {
    OptionSet* options = OptionSet::getOptions(CBR_MODULE);
    return options->referenceOption(name);
}

String GetPerServerFile(const char* opt_name, const ServerID& sid) {
    String orig = GetOption(opt_name)->as<String>();

    int32 dot_pos = orig.rfind(".");
    String prefix = orig.substr(0, dot_pos);
    String ext = orig.substr(dot_pos+1);

    char buffer[1024];
    sprintf(buffer, "%s-%04d.%s", prefix.c_str(), (uint32)sid, ext.c_str());

    return buffer;
}

} // namespace CBR
