/*  Sirikata
 *  SQLiteObjectFactory.cpp
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

#include "SQLiteObjectFactory.hpp"
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/sqlite/SQLite.hpp>

#define TABLE_NAME "objects"

namespace Sirikata {

SQLiteObjectFactory::SQLiteObjectFactory(ObjectHostContext* ctx, ObjectHost* oh, const SpaceID& space, const String& filename)
 : mContext(ctx),
   mOH(oh),
   mSpace(space),
   mDBFilename(filename),
   mConnectRate(100)
{
}

void SQLiteObjectFactory::generate() {
    SQLiteDBPtr db = SQLite::getSingleton().open(mDBFilename);
    sqlite3_busy_timeout(db->db(), 1000);

    String value_query = "SELECT object, script_type, script_args, script_contents FROM ";
    value_query += "\"" TABLE_NAME "\"";
    int rc;
    char* remain;
    sqlite3_stmt* value_query_stmt;
    rc = sqlite3_prepare_v2(db->db(), value_query.c_str(), -1, &value_query_stmt, (const char**)&remain);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error preparing value query statement");
    if (rc==SQLITE_OK) {
        int step_rc = sqlite3_step(value_query_stmt);
        while(step_rc == SQLITE_ROW) {
            String object_str(
                (const char*)sqlite3_column_text(value_query_stmt, 0),
                sqlite3_column_bytes(value_query_stmt, 0)
            );
            String script_type(
                (const char*)sqlite3_column_text(value_query_stmt, 1),
                sqlite3_column_bytes(value_query_stmt, 1)
            );
            String script_args(
                (const char*)sqlite3_column_text(value_query_stmt, 2),
                sqlite3_column_bytes(value_query_stmt, 2)
            );
            String script_contents(
                (const char*)sqlite3_column_text(value_query_stmt, 3),
                sqlite3_column_bytes(value_query_stmt, 3)
            );

            HostedObjectPtr obj = mOH->createObject(
                UUID(object_str, UUID::HexString()),
                script_type, script_args, script_contents
            );

            step_rc = sqlite3_step(value_query_stmt);
        }
        if (step_rc != SQLITE_DONE) {
            // reset the statement so it'll clean up properly
            rc = sqlite3_reset(value_query_stmt);
            SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
        }
    }
    rc = sqlite3_finalize(value_query_stmt);
    SQLite::check_sql_error(db->db(), rc, NULL, "Error finalizing value query statement");
}

}
