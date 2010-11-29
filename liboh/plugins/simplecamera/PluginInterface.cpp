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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include "SimpleCameraObjectScriptManager.hpp"

static int simplecamera_plugin_refcount = 0;


SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "simplecamera";
}


SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++simplecamera_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(simplecamera_plugin_refcount>0);
    return --simplecamera_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return simplecamera_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    using std::tr1::placeholders::_1;
    if (simplecamera_plugin_refcount == 0) {
        ObjectScriptManagerFactory::getSingleton().registerConstructor(
            "simplecamera",
            std::tr1::bind(
                &Sirikata::SimpleCamera::SimpleCameraObjectScriptManager::createObjectScriptManager,
                _1
            )
        );
    }
    simplecamera_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (simplecamera_plugin_refcount>0) {
        simplecamera_plugin_refcount--;
        assert(simplecamera_plugin_refcount==0);
        if (simplecamera_plugin_refcount==0) {
            ObjectScriptManagerFactory::getSingleton().unregisterConstructor("simplecamera");
        }
    }
}
