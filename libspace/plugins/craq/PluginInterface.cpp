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
#include <sirikata/core/options/Options.hpp>
#include <sirikata/space/ObjectSegmentation.hpp>
#include "CraqObjectSegmentation.hpp"

static int space_craq_plugin_refcount = 0;

#define OSEG_UNIQUE_CRAQ_PREFIX    "oseg_unique_craq_prefix"

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("space_craq", NULL,
        new OptionValue(OSEG_UNIQUE_CRAQ_PREFIX,"G",Sirikata::OptionValueType<String>(),"Specifies a unique character prepended to Craq lookup calls.  Note: takes in type string.  Will only select the first character in the string.  Also note that it acceptable values range from g to z, and are case sensitive."),
        NULL);
}

static ObjectSegmentation* createCraqOSeg(SpaceContext* ctx, Network::IOStrand* oseg_strand, CoordinateSegmentation* cseg, OSegCache* cache, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("space_craq",NULL);
    optionsSet->parse(args);

    String oseg_craq_prefix = optionsSet->referenceOption(OSEG_UNIQUE_CRAQ_PREFIX)->as<String>();
    if (oseg_craq_prefix.size() ==0)
    {
        std::cout<<"\n\nERROR: Incorrect craq prefix for oseg.  String must be at least one letter long.  (And be between G and Z.)  Please try again.\n\n";
        assert(false);
    }

    return new CraqObjectSegmentation(ctx, oseg_strand, cseg, cache, oseg_craq_prefix[0]);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_craq_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        using std::tr1::placeholders::_3;
        using std::tr1::placeholders::_4;
        using std::tr1::placeholders::_5;
        OSegFactory::getSingleton()
            .registerConstructor("craq",
                std::tr1::bind(&createCraqOSeg, _1, _2, _3, _4, _5));
    }
    space_craq_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_craq_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_craq_plugin_refcount>0);
    return --space_craq_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_craq_plugin_refcount>0) {
        space_craq_plugin_refcount--;
        assert(space_craq_plugin_refcount==0);
        if (space_craq_plugin_refcount==0) {
            OSegFactory::getSingleton().unregisterConstructor("craq");
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "craq";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_craq_plugin_refcount;
}
