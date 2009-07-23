#include <util/Platform.hpp>
#include "SQLite.hpp"

namespace Sirikata {
void SQLite::check_sql_error(sqlite3* db, int rc, char** sql_error_msg, std::string msg)
{
	if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
		std::cout << msg << " ... ";
		if (sql_error_msg && *sql_error_msg) {
			std::cout << *sql_error_msg;
			sqlite3_free(*sql_error_msg);
			*sql_error_msg = NULL;
		}
		else {
			std::cout << sqlite3_errmsg(db);
		}
		std::cout << std::endl;
	}
}
SQLiteDB::SQLiteDB(const String& name) {
    int rc;

    rc = sqlite3_open( name.c_str(), &mObjectDB );
    if (rc) {
        std::string errormsg = std::string("Couldn't open specified object DB: ") + String( sqlite3_errmsg(mObjectDB) );
        sqlite3_close(mObjectDB);
        throw std::runtime_error(errormsg);
    }
}

SQLiteDB::~SQLiteDB() {
    sqlite3_close(mObjectDB);
}

sqlite3* SQLiteDB::db() const {
    return mObjectDB;
}


AUTO_SINGLETON_INSTANCE(SQLite);

SQLite::SQLite() {
}

SQLite::~SQLite() {
}

SQLiteDBPtr SQLite::open(const String& name) {
    UpgradeLock upgrade_lock(mDBMutex);

    // First get the thread local storage for this database
    DBMap::iterator it = mDBs.find(name);
    if (it == mDBs.end()) {
        // the thread specific store hasn't even been allocated
        UpgradedLock upgraded_lock(upgrade_lock);
        // verify another thread hasn't added it, then add it
        it = mDBs.find(name);
        if (it == mDBs.end()) {
            std::tr1::shared_ptr<ThreadDBPtr> newDb(new ThreadDBPtr());
            mDBs[name]=newDb;
            it = mDBs.find(name);
        }
    }
    assert(it != mDBs.end());
    std::tr1::shared_ptr<ThreadDBPtr> thread_db_ptr_ptr = it->second;
    assert(thread_db_ptr_ptr);

    // Now get the thread specific weak database connection
    WeakSQLiteDBPtr* weak_db_ptr_ptr = thread_db_ptr_ptr->get();
    if (weak_db_ptr_ptr == NULL) {
        // we don't have a weak pointer for this thread
        UpgradedLock upgraded_lock(upgrade_lock);
        weak_db_ptr_ptr = thread_db_ptr_ptr->get();
        if (weak_db_ptr_ptr == NULL) { // verify and create
            weak_db_ptr_ptr = new WeakSQLiteDBPtr();
            thread_db_ptr_ptr->reset( weak_db_ptr_ptr );
        }
    }
    assert(weak_db_ptr_ptr != NULL);

    SQLiteDBPtr db;
    db = weak_db_ptr_ptr->lock();
    if (!db) {
        // the weak pointer for this thread is NULL
        UpgradedLock upgraded_lock(upgrade_lock);
        db = weak_db_ptr_ptr->lock();
        if (!db) {
            db = SQLiteDBPtr(new SQLiteDB(name));
            *weak_db_ptr_ptr = db;
        }
    }
    assert(db);

    return db;
}

} // namespace Sirikata
