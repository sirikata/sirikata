/*  Sirikata
 *  PluginInterface.cpp
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/LocationService.hpp>

#include "StandardLocationService.hpp"
#include "AlwaysLocationUpdatePolicy.hpp"

static int space_standard_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    InitAlwaysLocationUpdatePolicyOptions();
}

static LocationService* createStandardLoc(SpaceContext* ctx, LocationUpdatePolicy* update_policy, const String& args) {
    return new StandardLocationService(ctx, update_policy);
}

static LocationUpdatePolicy* createAlwaysPolicy(const String& args) {
    return new AlwaysLocationUpdatePolicy(args);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_standard_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        LocationServiceFactory::getSingleton()
            .registerConstructor("standard",
                std::tr1::bind(&createStandardLoc, _1, _2, _3));
        LocationUpdatePolicyFactory::getSingleton()
            .registerConstructor("always",
                std::tr1::bind(&createAlwaysPolicy, _1));
    }
    space_standard_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_standard_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_standard_plugin_refcount>0);
    return --space_standard_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_standard_plugin_refcount>0) {
        space_standard_plugin_refcount--;
        assert(space_standard_plugin_refcount==0);
        if (space_standard_plugin_refcount==0) {
            LocationServiceFactory::getSingleton().unregisterConstructor("standard");
            LocationUpdatePolicyFactory::getSingleton().unregisterConstructor("always");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "local";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_standard_plugin_refcount;
}
