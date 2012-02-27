/*  Sirikata -- Cassandra plugin
 *  Cassandra.hpp
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

#ifndef _CASSANDRA_HPP_
#define _CASSANDRA_HPP_

#include <sirikata/cassandra/Platform.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/locks.hpp>

#include <libcassandra/cassandra_factory.h>
#include <libcassandra/cassandra.h>
#include <libcassandra/column_family_definition.h>
#include <libcassandra/keyspace.h>
#include <libcassandra/keyspace_definition.h>
#include <libcassandra/util_functions.h>
#include <libcassandra/exception.h>


namespace Sirikata {

/** Represents a Cassandra connection. */
class SIRIKATA_CASSANDRA_EXPORT CassandraDB {
public:
    CassandraDB(const String& host, int port, const String& keyspace);
    ~CassandraDB();

    boost::shared_ptr<libcassandra::Cassandra> db() const;

    /// Get the name of the keyspace this connection operates within.
    String getKeyspace() const;

    /** Try to create a column family if it doesn't exist, log errors if it
     *  fails for a reason besides already existing. Always creates it under the
     *  current keyspace.
     */
    void createColumnFamily(const String& name, const String& column_type);

private:
    void initSchema(const String& keyspace);

    boost::shared_ptr<libcassandra::Cassandra> client;
};

typedef std::tr1::shared_ptr<CassandraDB> CassandraDBPtr;
typedef std::tr1::weak_ptr<CassandraDB> WeakCassandraDBPtr;

/** Class to manage Cassandra connections so they can be shared by multiple classes
 *  or objects.
 */
class SIRIKATA_CASSANDRA_EXPORT Cassandra : public AutoSingleton<Cassandra> {
public:
    Cassandra();
    ~Cassandra();

    /** Open a Cassandra database or get the existing reference to it.
     *  Simply discarding the resulting database reference is sufficient,
     *  there is no need for explicitly cleaning up the connection.
     *  \param name the name of the database to open or connect to
     *  \returns a shared ptr to the database connection
     */
    CassandraDBPtr open(const String& host, int port);

    static Cassandra& getSingleton();
    static void destroy();
private:
    typedef boost::thread_specific_ptr<WeakCassandraDBPtr> ThreadDBPtr;
    typedef std::map<String, std::tr1::shared_ptr<ThreadDBPtr> > DBMap;

    typedef boost::shared_mutex SharedMutex;
    typedef boost::shared_lock<SharedMutex> SharedLock;
    typedef boost::upgrade_lock<SharedMutex> UpgradeLock;
    typedef boost::upgrade_to_unique_lock<SharedMutex> UpgradedLock;
    typedef boost::unique_lock<SharedMutex> ExclusiveLock;

    DBMap mDBs;
    SharedMutex mDBMutex;

};

} // namespace Sirikata

#endif //_CASSANDRA_HPP_
