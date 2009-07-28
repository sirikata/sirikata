/*  Sirikata SQLite Plugin
 *  SQLitePlugin.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH package.
 */

#include <util/Platform.hpp>
#include <boost/thread.hpp>
#include "persistence/ObjectStorage.hpp"
#include "persistence/MinitransactionHandlerFactory.hpp"
#include "persistence/ReadWriteHandlerFactory.hpp"
#include "SQLite_Persistence.pbj.hpp"
#include "SQLiteObjectStorage.hpp"
static int core_plugin_refcount = 0;

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (core_plugin_refcount==0) {
        using std::tr1::placeholders::_1;
        Persistence::MinitransactionHandlerFactory::getSingleton()
            .registerConstructor("sqlite",
                                 std::tr1::bind(&Persistence::SQLiteObjectStorage::create,true,_1),
                                 true);
        Persistence::ReadWriteHandlerFactory::getSingleton()
            .registerConstructor("sqlite",
                                 std::tr1::bind(&Persistence::SQLiteObjectStorage::create,false,_1),
                                 true);
    }
    core_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++core_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(core_plugin_refcount>0);
    return --core_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (core_plugin_refcount>0) {
        core_plugin_refcount--;
        assert(core_plugin_refcount==0);
        if (core_plugin_refcount==0) {
            Persistence::MinitransactionHandlerFactory::getSingleton().unregisterConstructor("sqlite",true);
            Persistence::ReadWriteHandlerFactory::getSingleton().unregisterConstructor("sqlite",true);
        }
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "sqlite";
}
SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return core_plugin_refcount;
}
