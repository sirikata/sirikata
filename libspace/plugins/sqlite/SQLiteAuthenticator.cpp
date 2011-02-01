/*  Sirikata
 *  SQLiteAuthenticator.cpp
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

#include "SQLiteAuthenticator.hpp"

namespace Sirikata {

SQLiteAuthenticator::SQLiteAuthenticator(SpaceContext* ctx, const String& dbfile, const String& select_stmt, const String& delete_stmt)
 : mContext(ctx),
   mDBFile(dbfile),
   mDBGetSessionStmt(select_stmt),
   mDBDeleteSessionStmt(delete_stmt)
{
}

void SQLiteAuthenticator::start() {
    mDB = SQLite::getSingleton().open(mDBFile);
}

void SQLiteAuthenticator::stop() {
    mDB.reset();
}

bool SQLiteAuthenticator::checkTicket(const String& ticket) {
    bool found_ticket = false;

    int rc;
    char* remain;
    sqlite3_stmt* value_query_stmt;
    rc = sqlite3_prepare_v2(mDB->db(), mDBGetSessionStmt.c_str(), -1, &value_query_stmt, (const char**)&remain);
    SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing value query statement");
    if (rc != SQLITE_OK)
        return false;

    rc = sqlite3_bind_text(value_query_stmt, 1, ticket.data(), (int)ticket.size(), SQLITE_TRANSIENT);
    SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding key name to value query statement");
    if (rc != SQLITE_OK)
        return false;

    int step_rc = sqlite3_step(value_query_stmt);
    while(step_rc == SQLITE_ROW) {
        found_ticket = true;
        step_rc = sqlite3_step(value_query_stmt);
    }
    if (step_rc != SQLITE_DONE) {
        // reset the statement so it'll clean up properly
        rc = sqlite3_reset(value_query_stmt);
        SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing value query statement");
    }

    rc = sqlite3_finalize(value_query_stmt);
    SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing value query statement");

    return found_ticket;
}

void SQLiteAuthenticator::deleteTicket(const String& ticket) {
    int rc;
    char* remain;
    sqlite3_stmt* value_query_stmt;
    rc = sqlite3_prepare_v2(mDB->db(), mDBDeleteSessionStmt.c_str(), -1, &value_query_stmt, (const char**)&remain);
    SQLite::check_sql_error(mDB->db(), rc, NULL, "Error preparing value query statement");
    if (rc != SQLITE_OK)
        return;

    rc = sqlite3_bind_text(value_query_stmt, 1, ticket.data(), (int)ticket.size(), SQLITE_TRANSIENT);
    SQLite::check_sql_error(mDB->db(), rc, NULL, "Error binding key name to value query statement");
    if (rc != SQLITE_OK)
        return;

    int step_rc = sqlite3_step(value_query_stmt);
    if (step_rc != SQLITE_DONE) {
        // reset the statement so it'll clean up properly
        rc = sqlite3_reset(value_query_stmt);
        SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing value query statement");
    }

    rc = sqlite3_finalize(value_query_stmt);
    SQLite::check_sql_error(mDB->db(), rc, NULL, "Error finalizing value query statement");
}

void SQLiteAuthenticator::respond(Callback cb, bool result) {
    mContext->mainStrand->post(
        std::tr1::bind(cb, result)
    );
}

void SQLiteAuthenticator::authenticate(const UUID& obj_id, MemoryReference auth, Callback cb) {
    if (!mDB) {
        respond(cb, false);
        return;
    }

    // Treat the auth data as just a string. We should have some encoding and .
    String auth_ticket((const char*)auth.data(), (size_t)auth.size());

    bool found_ticket = checkTicket(auth_ticket);
    if (found_ticket) deleteTicket(auth_ticket);

    respond(cb, found_ticket);
}

} // namespace Sirikata
