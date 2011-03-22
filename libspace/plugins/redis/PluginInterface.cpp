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
#include "RedisObjectSegmentation.hpp"

static int space_redis_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("space_redis", NULL,
        new OptionValue("host","127.0.0.1",Sirikata::OptionValueType<String>(),"Redis host to connect to."),
        new OptionValue("port","6379",Sirikata::OptionValueType<uint32>(),"Redis port to connect to."),
        NULL
    );
}

static ObjectSegmentation* createRedisOSeg(SpaceContext* ctx, Network::IOStrand* oseg_strand, CoordinateSegmentation* cseg, OSegCache* cache, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("space_redis", NULL);
    optionsSet->parse(args);

    String redis_host = optionsSet->referenceOption("host")->as<String>();
    uint32 redis_port = optionsSet->referenceOption("port")->as<uint32>();

    return new RedisObjectSegmentation(ctx, oseg_strand, cseg, cache, redis_host, redis_port);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_redis_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        using std::tr1::placeholders::_4;
        using std::tr1::placeholders::_5;
        OSegFactory::getSingleton()
            .registerConstructor("redis",
                std::tr1::bind(&createRedisOSeg, _1, _2, _3, _4, _5));
    }
    space_redis_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_redis_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_redis_plugin_refcount>0);
    return --space_redis_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_redis_plugin_refcount==0) {
        OSegFactory::getSingleton().unregisterConstructor("redis");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "redis";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_redis_plugin_refcount;
}
