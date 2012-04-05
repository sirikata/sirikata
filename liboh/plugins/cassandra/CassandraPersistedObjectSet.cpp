/*  Sirikata
 *  CassandraPersistentObjectSet.cpp
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

#include "CassandraPersistedObjectSet.hpp"
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>

#define CF_NAME "objects" // Column Family Name

namespace Sirikata {
namespace OH {

CassandraPersistedObjectSet::CassandraPersistedObjectSet(ObjectHostContext* ctx, const String& host, int port, const String& oh_id)
 : mContext(ctx),
   mDBHost(host),
   mDBPort(port),
   mOHostID(oh_id),
   mDB()
{
}

CassandraPersistedObjectSet::~CassandraPersistedObjectSet()
{
}

void CassandraPersistedObjectSet::start() {
    // Initialize and start the thread for IO work. This is only separated as a
    // thread rather than strand because we don't have proper multithreading in
    // cppoh.
    mIOService = new Network::IOService("CassandraPersistedObjectSet");
    mWork = new Network::IOWork(*mIOService, "CassandraPersistedObjectSet IO Thread");
    mThread = new Sirikata::Thread("CassandraPersistedObjectSet IO", std::tr1::bind(&Network::IOService::runNoReturn, mIOService));

    mIOService->post(std::tr1::bind(&CassandraPersistedObjectSet::initDB, this), "CassandraPersistedObjectSet::initDB");
}

void CassandraPersistedObjectSet::initDB() {
    CassandraDBPtr db = Cassandra::getSingleton().open(mDBHost, mDBPort);
    mDB = db;

    mDB->createColumnFamily("objects", "Super");
}

void CassandraPersistedObjectSet::stop() {
    // Just kill the work that keeps the IO thread alive and wait for thread to
    // finish, i.e. for outstanding transactions to complete
    delete mWork;
    mWork = NULL;
    mThread->join();
    delete mThread;
    mThread = NULL;
    delete mIOService;
    mIOService = NULL;
}


void CassandraPersistedObjectSet::requestPersistedObject(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb, const String& timestamp) {
    mIOService->post(
        std::tr1::bind(&CassandraPersistedObjectSet::performUpdate, this, internal_id, script_type, script_args, script_contents, cb, timestamp),
        "CassandraPersistedObjectSet::performUpdate"
    );
}

void CassandraPersistedObjectSet::performUpdate(const UUID& internal_id, const String& script_type, const String& script_args, const String& script_contents, RequestCallback cb, const String& timestamp) {
    bool success = true;

    String id_str = internal_id.rawHexData();

    // FIXME need a better way to wrap these three values
    String script_value = "#type#"+script_type+"#args#"+script_args+"#contents#"+script_contents;

    try{
        mDB->db()->insertColumn(mOHostID, CF_NAME, timestamp, id_str, script_value);
    }
    catch(...){
        std::cout <<"Exception Caught when perform Update"<<std::endl;
        success = false;
    }

    if (cb != 0)
        mContext->mainStrand->post(std::tr1::bind(cb, success), "CassandraPersistedObjectSet::performUpdate callback");
}

} //end namespace OH
} //end namespace Sirikata
