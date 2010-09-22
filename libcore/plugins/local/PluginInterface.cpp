/*  Sirikata
 *  PluginInterface.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/ServerIDMap.hpp>
#include <sirikata/core/options/Options.hpp>
#include "LocalServerIDMap.hpp"

static int local_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("local",NULL,
        new Sirikata::OptionValue("host", "127.0.0.1", Sirikata::OptionValueType<String>(), "Host for the space server."),
        new Sirikata::OptionValue("port", "7777", Sirikata::OptionValueType<uint16>(), "Port for the space server."),
        NULL);
}

static ServerIDMap* createLocalServerIDMap(const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("local",NULL);
    optionsSet->parse(args);

    String server_host = optionsSet->referenceOption("host")->as<String>();
    uint16 server_port = optionsSet->referenceOption("port")->as<uint16>();

    return new LocalServerIDMap(server_host, server_port);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (local_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        ServerIDMapFactory::getSingleton()
            .registerConstructor("local",
                std::tr1::bind(&createLocalServerIDMap, _1));
    }
    local_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++local_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(local_plugin_refcount>0);
    return --local_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (local_plugin_refcount>0) {
        local_plugin_refcount--;
        assert(local_plugin_refcount==0);
        if (local_plugin_refcount==0) {
            ServerIDMapFactory::getSingleton().unregisterConstructor("local");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "local";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return local_plugin_refcount;
}
