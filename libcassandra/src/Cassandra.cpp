/*  Sirikata Cassandra Plugin
 *  Cassandra.cpp
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
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sirikata/cassandra/Cassandra.hpp>
#include <boost/thread.hpp>


AUTO_SINGLETON_INSTANCE(Sirikata::Cassandra);

namespace Sirikata {

CassandraDB::CassandraDB(const String& host, int port, const String& keyspace) {
    libcassandra::CassandraFactory cf(host, port);
    client=boost::shared_ptr<libcassandra::Cassandra>(cf.create());

    try {
        client->setKeyspace(keyspace);
    }
    catch (org::apache::cassandra::InvalidRequestException &ire) {
        initSchema(keyspace);
    }
}

CassandraDB::~CassandraDB() {
}

    boost::shared_ptr<libcassandra::Cassandra> CassandraDB::db() const {
    return client;
}

String CassandraDB::getKeyspace() const {
    return client->getCurrentKeyspace();
}

void CassandraDB::initSchema(const String& keyspace) {
    try {
        // create keyspace
        libcassandra::KeyspaceDefinition ks_def;
        ks_def.setName(keyspace);
        client->createKeyspace(ks_def);
        client->setKeyspace(keyspace);
    }
    catch (org::apache::cassandra::NotFoundException &ire) {
        SILOG(cassandra, error, "NotFoundException Caught");
    }
    catch (org::apache::cassandra::InvalidRequestException &ire) {
        SILOG(cassandra, error, ire.why);
    }
    catch (...) {
        SILOG(cassandra, error, "Other Exception Caught");
    }
}

void CassandraDB::createColumnFamily(const String& name, const String& column_type) {
    // Make sure persistence and lease column family definitions exist.
    try {
        libcassandra::ColumnFamilyDefinition persistence_cf_def;
        persistence_cf_def.setName(name);
        persistence_cf_def.setColumnType(column_type);
        persistence_cf_def.setKeyspaceName(getKeyspace());
        client->createColumnFamily(persistence_cf_def);
    }
    catch (org::apache::cassandra::NotFoundException &ire) {
        SILOG(cassandra, error, "NotFoundException Caught");
    }
    catch (org::apache::cassandra::InvalidRequestException &ire) {
        // We'll get an InvalidRequest exception with message
        //  'x already exists in keyspace y'. Ignore those as we already have
        // the  column family we're trying to create.
        if (ire.why.find("already exists in keyspace") == std::string::npos)
            SILOG(cassandra, error, ire.why);
    }
    catch (...) {
        SILOG(cassandra, error, "Other Exception Caught");
    }
}

namespace {
boost::shared_mutex sSingletonMutex;
}

Cassandra::Cassandra() {
}

Cassandra::~Cassandra() {
}

Cassandra& Cassandra::getSingleton() {
    boost::unique_lock<boost::shared_mutex> lck(sSingletonMutex);

    return AutoSingleton<Cassandra>::getSingleton();
}

void Cassandra::destroy() {
    boost::unique_lock<boost::shared_mutex> lck(sSingletonMutex);

    AutoSingleton<Cassandra>::destroy();
}

CassandraDBPtr Cassandra::open(const String& host, int port) {
    UpgradeLock upgrade_lock(mDBMutex);

    // First get the thread local storage for this database
    DBMap::iterator it = mDBs.find(host);
    if (it == mDBs.end()) {
        // the thread specific store hasn't even been allocated
        UpgradedLock upgraded_lock(upgrade_lock);
        // verify another thread hasn't added it, then add it
        it = mDBs.find(host);
        if (it == mDBs.end()) {
            std::tr1::shared_ptr<ThreadDBPtr> newDb(new ThreadDBPtr());
            mDBs[host]=newDb;
            it = mDBs.find(host);
        }
    }
    assert(it != mDBs.end());
    std::tr1::shared_ptr<ThreadDBPtr> thread_db_ptr_ptr = it->second;
    assert(thread_db_ptr_ptr);

    // Now get the thread specific weak database connection
    WeakCassandraDBPtr* weak_db_ptr_ptr = thread_db_ptr_ptr->get();
    if (weak_db_ptr_ptr == NULL) {
        // we don't have a weak pointer for this thread
        UpgradedLock upgraded_lock(upgrade_lock);
        weak_db_ptr_ptr = thread_db_ptr_ptr->get();
        if (weak_db_ptr_ptr == NULL) { // verify and create
            weak_db_ptr_ptr = new WeakCassandraDBPtr();
            thread_db_ptr_ptr->reset( weak_db_ptr_ptr );
        }
    }
    assert(weak_db_ptr_ptr != NULL);

    CassandraDBPtr db;
    db = weak_db_ptr_ptr->lock();
    if (!db) {
        // the weak pointer for this thread is NULL
        UpgradedLock upgraded_lock(upgrade_lock);
        db = weak_db_ptr_ptr->lock();
        if (!db) {
            // TODO(ewencp) make the keyspace configurable
            db = CassandraDBPtr(new CassandraDB(host, port, "sirikata"));
            *weak_db_ptr_ptr = db;
        }
    }
    assert(db);

    return db;
}

} // namespace Sirikata
