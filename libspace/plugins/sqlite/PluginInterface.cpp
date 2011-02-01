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
#include <sirikata/space/Authenticator.hpp>
#include "SQLiteAuthenticator.hpp"

static int space_sqlite_plugin_refcount = 0;

#define OPT_DB_FILE "db"
#define OPT_DB_GET_SESSION_SQL "get-session-sql"
#define OPT_DB_DELETE_SESSION_SQL "delete-session-sql"

namespace Sirikata {

static void InitPluginOptions() {
    Sirikata::InitializeClassOptions ico("space_sqlite", NULL,
        new OptionValue(OPT_DB_FILE, "", Sirikata::OptionValueType<String>(), "The path to the database file."),
        new OptionValue(OPT_DB_GET_SESSION_SQL, "select ticket from session_auth where ticket == ?", Sirikata::OptionValueType<String>(), "The SQL statement which, given a ticket identifier, extracts at least one column from the table that matches the ticket. If any matches are found, the user will be authenticated."),
        new OptionValue(OPT_DB_DELETE_SESSION_SQL, "delete from session_auth where ticket == ?", Sirikata::OptionValueType<String>(), "The SQL statement which, given a ticket identifier, deletes that session from the database so it can't be reused."),
        NULL);
}

static Authenticator* createSQLiteAuthenticator(SpaceContext* ctx, const String& args) {
    OptionSet* optionsSet = OptionSet::getOptions("space_sqlite",NULL);
    optionsSet->parse(args);

    return new SQLiteAuthenticator(
        ctx,
        optionsSet->referenceOption(OPT_DB_FILE)->as<String>(),
        optionsSet->referenceOption(OPT_DB_GET_SESSION_SQL)->as<String>(),
        optionsSet->referenceOption(OPT_DB_DELETE_SESSION_SQL)->as<String>()
    );
}

} // namespace Sirikata

SIRIKATA_PLUGIN_EXPORT_C void init() {
    using namespace Sirikata;
    if (space_sqlite_plugin_refcount==0) {
        InitPluginOptions();
        using std::tr1::placeholders::_1;
        using std::tr1::placeholders::_2;
        AuthenticatorFactory::getSingleton()
            .registerConstructor("sqlite",
                std::tr1::bind(&createSQLiteAuthenticator, _1, _2));
    }
    space_sqlite_plugin_refcount++;
}

SIRIKATA_PLUGIN_EXPORT_C int increfcount() {
    return ++space_sqlite_plugin_refcount;
}
SIRIKATA_PLUGIN_EXPORT_C int decrefcount() {
    assert(space_sqlite_plugin_refcount>0);
    return --space_sqlite_plugin_refcount;
}

SIRIKATA_PLUGIN_EXPORT_C void destroy() {
    using namespace Sirikata;
    if (space_sqlite_plugin_refcount==0) {
        AuthenticatorFactory::getSingleton().unregisterConstructor("sqlite");
    }
}

SIRIKATA_PLUGIN_EXPORT_C const char* name() {
    return "sqlite";
}

SIRIKATA_PLUGIN_EXPORT_C int refcount() {
    return space_sqlite_plugin_refcount;
}
