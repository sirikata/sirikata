/*  Sirikata
 *  JSPlugin.cpp
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

#include <oh/Platform.hpp>
#include <oh/ObjectScriptManagerFactory.hpp>
#include "JSObjectScriptManager.hpp"
#include "JSLogging.hpp"

static int js_plugin_refcount = 0;


SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "js";
}


SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++js_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(js_plugin_refcount>0);
    return --js_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return js_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    using std::tr1::placeholders::_1;
    if (js_plugin_refcount == 0) {
        JSLOG(info,"Initializing JS Plugin.");
        ObjectScriptManagerFactory::getSingleton().registerConstructor(
            "js",
            std::tr1::bind(
                &Sirikata::JS::JSObjectScriptManager::createObjectScriptManager,
                _1
            )
        );
    }
    js_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (js_plugin_refcount>0) {
        js_plugin_refcount--;
        assert(js_plugin_refcount==0);
        if (js_plugin_refcount==0) {
            JSLOG(info,"Destroying JS Plugin.");
            ObjectScriptManagerFactory::getSingleton().unregisterConstructor("js");
        }
    }
}
