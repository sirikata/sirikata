/*  Sirikata -- SQLite plugin
 *  SQLite.hpp
 *
 *  Copyright (c) 2008, Stanford University
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

#ifndef _SQLITE_HPP_
#define _SQLITE_HPP_

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/tss.hpp>
#include <sqlite3.h>

namespace Sirikata { namespace Persistence {

/** Represents a SQLite database connection. */
class SQLiteDB {
public:
    SQLiteDB(const String& name);
    ~SQLiteDB();

    sqlite3* db() const;
private:
    sqlite3* mObjectDB;
};

typedef std::tr1::shared_ptr<SQLiteDB> SQLiteDBPtr;
typedef std::tr1::weak_ptr<SQLiteDB> WeakSQLiteDBPtr;

/** Class to manage SQLite connections so they can be shared by multiple classes
 *  or objects.
 */
class SQLite :public AutoSingleton<SQLite>{
public:
    SQLite();
    ~SQLite();

    /** Open a SQLite database or get the existing reference to it.
     *  Simply discarding the resulting database reference is sufficient,
     *  there is no need for explicitly cleaning up the connection.
     *  \param name the name of the database to open or connect to
     *  \returns a shared ptr to the database connection
     */
    SQLiteDBPtr open(const String& name);
    static void check_sql_error(sqlite3* db, int rc, char** sql_error_msg, std::string msg);
private:
    typedef boost::thread_specific_ptr<WeakSQLiteDBPtr> ThreadDBPtr;
    typedef std::map<String, std::tr1::shared_ptr<ThreadDBPtr> > DBMap;

    typedef boost::shared_mutex SharedMutex;
    typedef boost::shared_lock<SharedMutex> SharedLock;
    typedef boost::upgrade_lock<SharedMutex> UpgradeLock;
    typedef boost::upgrade_to_unique_lock<SharedMutex> UpgradedLock;
    typedef boost::unique_lock<SharedMutex> ExclusiveLock;

    DBMap mDBs;
    SharedMutex mDBMutex;
};

} }// namespace Sirikata::Persistence

#endif //_SQLITE_HPP_
