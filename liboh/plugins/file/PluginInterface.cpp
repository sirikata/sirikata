/*  Sirikata
 *  PluginInterface.cpp
 *
 *  Copyright (c) 2011, Stanford University
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
#include <sirikata/core/options/Options.hpp>
#include "FileStorage.hpp"

static int filestorage_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("filestorage",NULL,
        new Sirikata::OptionValue("dir", "", Sirikata::OptionValueType<String>(), "Directory to store data to."),
        NULL);
}

static OH::Storage* createFileStorage(ObjectHostContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("filestorage",NULL);
    optionsSet->parse(args);

    String dir = optionsSet->referenceOption("dir")->as<String>();

    return new OH::FileStorage(ctx, dir);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (filestorage_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        OH::StorageFactory::getSingleton()
            .registerConstructor("file",
                                 std::tr1::bind(&createFileStorage, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
    }
    filestorage_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++filestorage_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(filestorage_plugin_refcount>0);
    return --filestorage_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (filestorage_plugin_refcount==0) {
        OH::StorageFactory::getSingleton().unregisterConstructor("file");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-file";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return filestorage_plugin_refcount;
}
