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
#include "SQLiteStorage.hpp"
#include "SQLitePersistedObjectSet.hpp"
#include "SQLiteObjectFactory.hpp"

static int sqliteoh_plugin_refcount = 0;

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("sqlitestorage",NULL,
        new Sirikata::OptionValue("db", "storage.db", Sirikata::OptionValueType<String>(), "Database file to store data to."),
        NULL);

    Sirikata::InitializeClassOptions icop("sqlitepersistedset",NULL,
        new Sirikata::OptionValue("db", "storage.db", Sirikata::OptionValueType<String>(), "Database file to store data to."),
        NULL);

    Sirikata::InitializeClassOptions icof("sqlitefactory",NULL,
        new Sirikata::OptionValue("db", "storage.db", Sirikata::OptionValueType<String>(), "File to read objects from."),
        NULL);
}

static OH::Storage* createSQLiteStorage(ObjectHostContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("sqlitestorage",NULL);
    optionsSet->parse(args);

    String db = optionsSet->referenceOption("db")->as<String>();

    return new OH::SQLiteStorage(ctx, db);
}

static OH::PersistedObjectSet* createSQLitePersistedObjectSet(ObjectHostContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("sqlitepersistedset",NULL);
    optionsSet->parse(args);

    String db = optionsSet->referenceOption("db")->as<String>();

    return new OH::SQLitePersistedObjectSet(ctx, db);
}

static ObjectFactory* createSQLiteObjectFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("sqlitefactory",NULL);
    optionsSet->parse(args);

    String dbfile = optionsSet->referenceOption("db")->as<String>();

    return new SQLiteObjectFactory(ctx, oh, space, dbfile);
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (sqliteoh_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        OH::StorageFactory::getSingleton()
            .registerConstructor("sqlite",
                                 std::tr1::bind(&createSQLiteStorage, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
        OH::PersistedObjectSetFactory::getSingleton()
            .registerConstructor("sqlite",
                                 std::tr1::bind(&createSQLitePersistedObjectSet, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
        ObjectFactoryFactory::getSingleton()
            .registerConstructor("sqlite",
                                 std::tr1::bind(&createSQLiteObjectFactory, std::tr1::placeholders::_1, std::tr1::placeholders::_2, std::tr1::placeholders::_3, std::tr1::placeholders::_4));
    }
    sqliteoh_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++sqliteoh_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(sqliteoh_plugin_refcount>0);
    return --sqliteoh_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (sqliteoh_plugin_refcount==0) {
        OH::StorageFactory::getSingleton().unregisterConstructor("sqlite");
        OH::PersistedObjectSetFactory::getSingleton().unregisterConstructor("sqlite");
        ObjectFactoryFactory::getSingleton().unregisterConstructor("sqlite");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "oh-sqlite";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return sqliteoh_plugin_refcount;
}
