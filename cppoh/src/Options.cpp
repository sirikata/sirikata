/*  Sirikata
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

#include "Options.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/core/util/UUID.hpp>

namespace Sirikata {

void InitCPPOHOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        // Note: we load *many* more plugins for cppoh by default since it is
        // also the client.

        .addOption(new OptionValue(OPT_CONFIG_FILE,"",Sirikata::OptionValueType<String>(),"Configuration file to load."))

        .addOption(new OptionValue(OPT_OH_PLUGINS,"weight-exp,weight-sqr,tcpsst,sqlite,weight-const,ogregraphics,colladamodels,csvfactory,scripting-js,simplecamera",Sirikata::OptionValueType<String>(),"Plugin list to load."))

        .addOption(new OptionValue("ohid", "1", Sirikata::OptionValueType<ObjectHostID>(), "Object host ID for this server"))

        .addOption(new OptionValue(STATS_OH_TRACE_FILE, "trace.txt", Sirikata::OptionValueType<String>(), "The filename to save the trace to"))
        .addOption(new OptionValue(STATS_SAMPLE_RATE, "250ms", Sirikata::OptionValueType<Duration>(), "Frequency to sample non-event statistics such as queue information."))

        .addOption(new OptionValue("object-host-receive-buffer", "32768", Sirikata::OptionValueType<int32>(), "size of the object host space node connection receive queue"))
        .addOption(new OptionValue("object-host-send-buffer", "32768", Sirikata::OptionValueType<int32>(), "size of the object host space node cnonection send queue"))

        .addOption(new OptionValue(OPT_OH_OPTIONS,"",OptionValueType<String>(),"Options passed to the object host"))
        .addOption(new OptionValue(OPT_MAIN_SPACE,"12345678-1111-1111-1111-DEFA01759ACE",OptionValueType<UUID>(),"space which to connect default objects to"))
        .addOption(new OptionValue(OPT_SPACEID_MAP,
                "12345678-1111-1111-1111-DEFA01759ACE:{127.0.0.1:5943}",
                OptionValueType<std::map<std::string,std::string> >(),
                "Map between space ID's and TCPSST IPs."))

        .addOption(new OptionValue(OPT_SIGFPE,"false",OptionValueType<bool>(),"Enable floating point exceptions"))

        .addOption(new OptionValue(OPT_OBJECT_FACTORY,"csv",OptionValueType<String>(),"Type of object factory to instantiate"))
        .addOption(new OptionValue(OPT_OBJECT_FACTORY_OPTS,"--db=scene.db",OptionValueType<String>(),"Options to pass to object factory constructor"))
        .addOption(new OptionValue(OPT_CAMERASCRIPT,"",OptionValueType<String>(),"The scripts the camera should startup with"))
        .addOption(new OptionValue(OPT_CAMERASCRIPTTYPE,"js",OptionValueType<String>(),"The language the script is written in."))
        ;
}

} // namespace Sirikata
