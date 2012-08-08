/*  Sirikata
 *  RedisObjectSegmentation.cpp
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

#include "RedisObjectSegmentation.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lambda/lambda.hpp>

#define REDISOSEG_LOG(lvl,msg) SILOG(redis_oseg, lvl, msg)

namespace Sirikata {

namespace {

void globalRedisConnectHandler(const redisAsyncContext *c) {
    REDISOSEG_LOG(insane, "Connected.");
}

void globalRedisDisconnectHandler(const redisAsyncContext *c, int status) {
    if (status == REDIS_OK) return;
    REDISOSEG_LOG(error, "Global error handler: " << c->errstr);
    RedisObjectSegmentation* oseg = (RedisObjectSegmentation*)c->data;
    oseg->disconnected();
}

void globalRedisAddRead(void *privdata) {
    RedisObjectSegmentation* oseg = (RedisObjectSegmentation*)privdata;
    oseg->addRead();
}

void globalRedisDelRead(void *privdata) {
    RedisObjectSegmentation* oseg = (RedisObjectSegmentation*)privdata;
    oseg->delRead();
}

void globalRedisAddWrite(void *privdata) {
    RedisObjectSegmentation* oseg = (RedisObjectSegmentation*)privdata;
    oseg->addWrite();
}

void globalRedisDelWrite(void *privdata) {
    RedisObjectSegmentation* oseg = (RedisObjectSegmentation*)privdata;
    oseg->delWrite();
}

void globalRedisCleanup(void *privdata) {
    RedisObjectSegmentation* oseg = (RedisObjectSegmentation*)privdata;
    oseg->cleanup();
}

// Basic state tracking for a request that uses Redis async api
struct RedisObjectOperationInfo {
    RedisObjectOperationInfo(RedisObjectSegmentation* _oseg, const UUID& _obj)
     : oseg(_oseg), obj(_obj), completed(false), refcount(0)
    {}

    void checkDestroy() {
        if (refcount == 0) delete this;
    }

    RedisObjectSegmentation* oseg;
    UUID obj;
    bool completed; // Callback invoked, successful or not

    // Only used in some cases, not required except for transactions
    uint8 refcount;
};
// State tracking for migrate changes. If we need to generate an ack, this
// requires additional info
struct RedisObjectMigratedOperationInfo {
    RedisObjectMigratedOperationInfo(RedisObjectSegmentation* _oseg, const UUID& _obj, ServerID _ackTo)
     : oseg(_oseg), obj(_obj), ackTo(_ackTo), completed(false), refcount(0)
    {}

    void checkDestroy() {
        if (refcount == 0) delete this;
    }

    RedisObjectSegmentation* oseg;
    UUID obj;
    ServerID ackTo;
    bool completed; // Callback invoked, successful or not

    // Only used in some cases, not required except for transactions
    uint8 refcount;
};

void globalRedisLookupObjectReadFinished(redisAsyncContext* c, void* _reply, void* privdata) {
    redisReply *reply = (redisReply*)_reply;
    RedisObjectOperationInfo* wi = (RedisObjectOperationInfo*)privdata;

    if (reply == NULL) {
        REDISOSEG_LOG(error, "Unknown redis error when reading object " << wi->obj.toString());
    }
    else if (reply->type == REDIS_REPLY_ERROR) {
        REDISOSEG_LOG(error, "Redis error when reading object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
        wi->oseg->failReadObject(wi->obj);
    }
    else if (reply->type == REDIS_REPLY_NIL) {
        REDISOSEG_LOG(error, "Redis got nil when reading object.");
        wi->oseg->failReadObject(wi->obj);
    }
    else if (reply->type == REDIS_REPLY_STRING) {
        wi->oseg->finishReadObject(wi->obj, String(reply->str, reply->len));
    }
    else {
        REDISOSEG_LOG(error, "Unexpected redis reply type when reading object " << wi->obj.toString() << ": " << reply->type);
    }

    delete wi;
}


void globalRedisAddNewObjectWriteFinished(redisAsyncContext* c, void* _reply, void* privdata) {
    redisReply *reply = (redisReply*)_reply;
    RedisObjectOperationInfo* wi = (RedisObjectOperationInfo*)privdata;

    // This code can handle both transaction and non-transaction writes (which we need
    // for older versions of the redis server). They can be cleanly separated
    // because transactions only have simple status replies + a bulk reply,
    // whereas the individual commands (set + expire) both return an
    // integer. Any other values like null or errors can be returned as errors
    // in both cases.

    wi->refcount--;
    // If we've already indicated a result, we can't go back
    if (wi->completed) {
        wi->checkDestroy();
        return;
    }


    if (reply == NULL)
    {
        REDISOSEG_LOG(error, "Unknown redis error when writing new object " << wi->obj.toString());
        wi->oseg->finishWriteNewObject(wi->obj, OSegWriteListener::UNKNOWN_ERROR);
        wi->completed = true;
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        REDISOSEG_LOG(error, "Redis error when writing new object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
        wi->oseg->finishWriteNewObject(wi->obj,OSegWriteListener::UNKNOWN_ERROR);
        wi->completed = true;
    }
    else if (reply->type == REDIS_REPLY_INTEGER)
    {
        if (reply->integer == 1)
        {
            // We need to get two of these, one for the SET(NX) one for the
            // EXPIRE. Given the set of commands, we expect the refcount for the
            // reply for EXPIRE to be 0 (we've already decremented and there's
            // no EXEC following it in non-transaction mode). If we get into
            // here (no previous error) and we're at that refcount, we can
            // indicate success
            if (wi->refcount == 0) {
                wi->oseg->finishWriteNewObject(wi->obj, OSegWriteListener::SUCCESS);
                wi->completed = true;
            }
        }
        else if (reply->integer == 0)
        {
            REDISOSEG_LOG(error, "Redis error when writing new object " << wi->obj.toString() << ": " << reply->integer<< " likely already registered.");
            wi->oseg->finishWriteNewObject(wi->obj,OSegWriteListener::OBJ_ALREADY_REGISTERED);
            wi->completed = true;
        }
        else
        {
            REDISOSEG_LOG(error, "Redis error when writing new object " << wi->obj.toString() << ": " << reply->integer<< " unknown error.");
            wi->oseg->finishWriteNewObject(wi->obj,OSegWriteListener::UNKNOWN_ERROR);
            wi->completed = true;
        }
    }
    else if (reply->type == REDIS_REPLY_STATUS && (String(reply->str, reply->len) == String("QUEUED") || String(reply->str, reply->len) == String("OK"))) {
        // Just an OK response to the initial MULTI call or a queued item in the
        // transaction, don't need to do anything until we get more feedback
    }
    else if (reply->type == REDIS_REPLY_ARRAY) {
        // Bulk reply
        if (reply->elements != 2) { // Set + expire
            wi->oseg->finishWriteNewObject(wi->obj, OSegWriteListener::UNKNOWN_ERROR);
        }
        else if (reply->element[0]->type != REDIS_REPLY_INTEGER || reply->element[1]->type != REDIS_REPLY_INTEGER) {
            wi->oseg->finishWriteNewObject(wi->obj, OSegWriteListener::UNKNOWN_ERROR);
        }
        else if (reply->element[0]->integer != 1) { // SET failed
            REDISOSEG_LOG(error, "Redis error when writing new object " << wi->obj.toString() << ": " << reply->integer<< " likely already registered.");
            wi->oseg->finishWriteNewObject(wi->obj,OSegWriteListener::OBJ_ALREADY_REGISTERED);
        }
        else if (reply->element[1]->integer != 1) { // EXPIRE failed
            REDISOSEG_LOG(error, "Redis error when writing new object expiry " << wi->obj.toString() << ": " << reply->integer<< " likely already registered.");
            wi->oseg->finishWriteNewObject(wi->obj,OSegWriteListener::UNKNOWN_ERROR);
        }
        else {
            // If we've gotten past all these other error checks, we seem to
            // have succeeded
            wi->oseg->finishWriteNewObject(wi->obj, OSegWriteListener::SUCCESS);
        }
        // No matter what we've completed if we got here since it's the EXEC response
        wi->completed = true;
    }
    else
    {
        REDISOSEG_LOG(error, "Unexpected redis reply type when writing new object " << wi->obj.toString() << ": " << reply->type);
        wi->oseg->finishWriteNewObject(wi->obj,OSegWriteListener::UNKNOWN_ERROR);
        wi->completed = true;
    }

    wi->checkDestroy();
}

void globalRedisAddMigratedObjectWriteFinished(redisAsyncContext* c, void* _reply, void* privdata) {
    redisReply *reply = (redisReply*)_reply;
    RedisObjectMigratedOperationInfo* wi = (RedisObjectMigratedOperationInfo*)privdata;

    wi->refcount--;
    // If we've already indicated a result, we can't go back
    if (wi->completed) {
        wi->checkDestroy();
        return;
    }

    if (reply == NULL) {
        REDISOSEG_LOG(error, "Unknown redis error when writing migrated object " << wi->obj.toString());
        wi->completed = true;
    }
    else if (reply->type == REDIS_REPLY_ERROR) {
        REDISOSEG_LOG(error, "Redis error when writing migrated object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
        wi->completed = true;
    }
    else if (reply->type == REDIS_REPLY_STATUS) {
        String reply_str(reply->str, reply->len);
        if (reply_str == String("QUEUED")) {
            // Just queuing up events, safe to ignore
        }
        else if (reply_str == String("OK")) {
            // For transactions, we'll be at refcount == 3 for the initial OK to MULTI
            if (wi->refcount == 3) {
                // Ignore result from MULTI
            }
            else if (wi->refcount == 1) { // Non-transaction will be at 1 after the SET
                // Still need the EXPIRE to succeed, do nothing
            }
            else {
                // Unexpected error
                REDISOSEG_LOG(error, "Unexpected status reply while writing migrated object " << wi->obj.toString() << ": " << reply_str);
                wi->completed = true;
            }
        }
        else {
            REDISOSEG_LOG(error, "Redis error when writing migrated object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
            wi->completed = true;
        }
    }
    else if (reply->type == REDIS_REPLY_INTEGER) {
        // Response to non-transactional EXPIRE
        if (reply->integer == 1) {
            // We need to get two of these, one for the SET(NX) one for the
            // EXPIRE. Given the set of commands, we expect the refcount for the
            // reply for EXPIRE to be 0 (we've already decremented and there's
            // no EXEC following it in non-transaction mode). If we get into
            // here (no previous error) and we're at that refcount, we can
            // indicate success
            if (wi->refcount == 0) {
                wi->oseg->finishWriteMigratedObject(wi->obj, wi->ackTo);
                wi->completed = true;
            }
        }
        else {
            REDISOSEG_LOG(error, "Redis error when writing migrated object " << wi->obj.toString() << ": " << reply->integer << " likely already registered.");
            wi->completed = true;
        }
    }
    else if (reply->type == REDIS_REPLY_ARRAY) {
        // Bulk reply
        if (reply->elements != 2) { // Set + expire
            REDISOSEG_LOG(error, "Invalid bulk reply to migrated object transaction " << wi->obj.toString() << ": " << reply->elements << " elements");
        }
        else if (reply->element[0]->type != REDIS_REPLY_STATUS || reply->element[1]->type != REDIS_REPLY_INTEGER) {
            REDISOSEG_LOG(error, "Invalid bulk reply types to migrated object transaction " << wi->obj.toString());
        }
        else if ( String(reply->element[0]->str, reply->element[0]->len) != "OK") { // SET failed
            REDISOSEG_LOG(error, "Unexpected status reply for SET for migrated object transaction " << wi->obj.toString());
        }
        else if (reply->element[1]->integer != 1) { // EXPIRE failed
            REDISOSEG_LOG(error, "Unexpected status reply for EXPIRE for migrated object transaction " << wi->obj.toString());
        }
        else {
            // If we've gotten past all these other error checks, we seem to
            // have succeeded
            wi->oseg->finishWriteMigratedObject(wi->obj, wi->ackTo);
        }
        // No matter what we've completed if we got here since it's the EXEC response
        wi->completed = true;
    }
    else {
        REDISOSEG_LOG(error, "Unexpected redis reply type when writing migrated object " << wi->obj.toString() << ": " << reply->type);
        wi->completed = true;
    }

    wi->checkDestroy();
}

void globalRedisDeleteFinished(redisAsyncContext* c, void* _reply, void* privdata) {
    redisReply *reply = (redisReply*)_reply;
    RedisObjectOperationInfo* wi = (RedisObjectOperationInfo*)privdata;

    if (reply == NULL) {
        REDISOSEG_LOG(error, "Unknown redis error when deleting object " << wi->obj.toString());
    }
    else if (reply->type == REDIS_REPLY_ERROR) {
        REDISOSEG_LOG(error, "Redis error when deleting object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
        wi->oseg->failReadObject(wi->obj);
    }
    else if (reply->type == REDIS_REPLY_INTEGER) {
        if (reply->integer != 1)
            REDISOSEG_LOG(error, "Redis error when deleting object " << wi->obj.toString() << ", got incorrect return value: " << reply->integer);
    }
    else {
        REDISOSEG_LOG(error, "Unexpected redis reply type when deleting object " << wi->obj.toString() << ": " << reply->type);
    }

    delete wi;
}

void globalRedisRefreshObjectTimeoutWriteFinished(redisAsyncContext* c, void* _reply, void* privdata) {
    redisReply *reply = (redisReply*)_reply;
    RedisObjectOperationInfo* wi = (RedisObjectOperationInfo*)privdata;

    wi->refcount--;
    // If we've already indicated a result, we can't go back. We haven't issued
    // a callback, but this just keeps track of whether we even can give a valid
    // callback anymore/should report any more issues.
    if (wi->completed) {
        wi->checkDestroy();
        return;
    }

    if (reply == NULL) {
        REDISOSEG_LOG(error, "Unknown redis error when refreshing object timeout " << wi->obj.toString());
        wi->completed = true;
    }
    else if (reply->type == REDIS_REPLY_ERROR) {
        REDISOSEG_LOG(error, "Redis error when refreshing object timeout " << wi->obj.toString() << ": " << String(reply->str, reply->len));
        wi->completed = true;
    }
    else if (reply->type == REDIS_REPLY_STATUS) {
        String reply_str(reply->str, reply->len);
        if (reply_str == String("OK")) {
            // Should just be the OK from the SET for old versions of Redis,
            // safe to ignore and wait for the EXPIRE reply
        }
        else {
            REDISOSEG_LOG(error, "Redis got unexpected or bad status reply when refreshing object timeout " <<  wi->obj.toString() << ": " << reply_str);
            wi->completed = true;
        }
    }
    else if (reply->type == REDIS_REPLY_INTEGER) {
        if (reply->integer != 1) {
            REDISOSEG_LOG(error, "Redis error when refreshing object timeout " << wi->obj.toString() << ", got incorrect return value: " << reply->integer);
            wi->completed = true;
        }
        // Otherwise succeeded
    }
    else {
        REDISOSEG_LOG(error, "Unexpected redis reply type when refreshing object timeout " << wi->obj.toString() << ": " << reply->type);
        wi->completed = true;
    }

    wi->checkDestroy();
}

} // namespace

RedisObjectSegmentation::RedisObjectSegmentation(SpaceContext* con, Network::IOStrand* o_strand, CoordinateSegmentation* cseg, OSegCache* cache, const String& redis_host, uint32 redis_port, const String& redis_prefix, Duration key_ttl, bool redis_has_transactions)
 : ObjectSegmentation(con, o_strand),
   mCSeg(cseg),
   mCache(cache),
   mRedisHost(redis_host),
   mRedisPort(redis_port),
   mRedisPrefix(redis_prefix),
   mRedisKeyTTL(key_ttl),
   mRedisHasTransactions(redis_has_transactions),
   mRedisContext(NULL),
   mRedisFD(NULL),
   mReading(false),
   mWriting(false),
   mExpiryTimer(
       Network::IOTimer::create(
           con->mainStrand,
           std::tr1::bind(&RedisObjectSegmentation::processExpiredObjects, this)
       )
   )
{
}

RedisObjectSegmentation::~RedisObjectSegmentation() {
    cleanup();
}

void RedisObjectSegmentation::start() {
    ObjectSegmentation::start();
    connect();
}

void RedisObjectSegmentation::stop() {
    mExpiryTimer->cancel();
    ObjectSegmentation::stop();
}

void RedisObjectSegmentation::connect() {
    Lock lck(mMutex);

    mRedisContext = redisAsyncConnect(mRedisHost.c_str(), mRedisPort);
    if (mRedisContext->err) {
        REDISOSEG_LOG(error, "Failed to connect to redis: " << mRedisContext->errstr);
        redisAsyncDisconnect(mRedisContext);
        mRedisContext = NULL;
    } else {
        REDISOSEG_LOG(insane, "Optimistically connected to redis.");
    }

    // This appears to be the only way to get a non-static 'argument' to the
    // connect and disconnect callbacks.
    mRedisContext->data = (void*)this;

    redisAsyncSetConnectCallback(mRedisContext, globalRedisConnectHandler);
    redisAsyncSetDisconnectCallback(mRedisContext, globalRedisDisconnectHandler);

    mRedisContext->ev.addRead = globalRedisAddRead;
    mRedisContext->ev.delRead = globalRedisDelRead;
    mRedisContext->ev.addWrite = globalRedisAddWrite;
    mRedisContext->ev.delWrite = globalRedisDelWrite;
    mRedisContext->ev.cleanup = globalRedisCleanup;
    mRedisContext->ev.data = this;

    // Wrap this connections file descripter in ASIO
    using boost::asio::posix::stream_descriptor;
    mRedisFD = new stream_descriptor(mContext->ioService->asioService());
    mRedisFD->assign(mRedisContext->c.fd);

    // Force one command through. This ensures the connection gets fully
    // initialized. Otherwise, we can end up leaving the connection idle, the
    // server disconnects, and because haven't started anything, the next
    // command fails and *then* we get the disconnect event. Performing one
    // command ensures we'll get the disconnect event ASAP after it occurs.
    redisAsyncCommand(mRedisContext, NULL, NULL, "PING");
}

void RedisObjectSegmentation::ensureConnected() {
    if (mRedisContext == NULL) connect();
}


// None of these event handlers need locks in them since they are invoked by
// redis commands that we've already protected by locks

void RedisObjectSegmentation::disconnected() {
    cleanup();
}

void RedisObjectSegmentation::addRead() {
    REDISOSEG_LOG(insane, "Add read");

    if (mReading) return;
    mReading = true;

    startRead();
}

void RedisObjectSegmentation::delRead() {
    REDISOSEG_LOG(insane, "Del read");

    assert(mReading);
    mReading = false;
}

void RedisObjectSegmentation::addWrite() {
    REDISOSEG_LOG(insane, "Add write");

    if (mWriting) return;
    mWriting = true;

    startWrite();
}

void RedisObjectSegmentation::delWrite() {
    REDISOSEG_LOG(insane, "Del write");

    assert(mWriting);
    mWriting = false;
}

void RedisObjectSegmentation::cleanup() {
    REDISOSEG_LOG(insane, "Cleanup");

    // This can be invoked by the destructor as well as via callback so it needs
    // a lock
    Lock lck(mMutex);

    mRedisContext = NULL;
    delete mRedisFD;
    mRedisFD = NULL;
    mReading = false;
    mWriting = false;
}


// startRead and startWrite are safe for locking as they are called either by
// redis events or from readHandler and writeHandler, which are also protected

void RedisObjectSegmentation::startRead() {
    if (mStopping || !mReading) return;
    mRedisFD->async_read_some(boost::asio::null_buffers(),
        boost::bind(&RedisObjectSegmentation::readHandler, this, boost::asio::placeholders::error));
}

void RedisObjectSegmentation::startWrite() {
    if (mStopping || !mWriting) return;
    mRedisFD->async_write_some(boost::asio::null_buffers(),
        boost::bind(&RedisObjectSegmentation::writeHandler, this, boost::asio::placeholders::error));
}


void RedisObjectSegmentation::readHandler(const boost::system::error_code& ec) {
    if (ec) {
        REDISOSEG_LOG(error, "Error in read handler.");
        return;
    }

    Lock lck(mMutex);
    redisAsyncHandleRead(mRedisContext);
    startRead();
}

void RedisObjectSegmentation::writeHandler(const boost::system::error_code& ec) {
    if (ec) {
        REDISOSEG_LOG(error, "Error in write handler.");
        return;
    }

    Lock lck(mMutex);
    redisAsyncHandleWrite(mRedisContext);
    startWrite();
}


OSegEntry RedisObjectSegmentation::cacheLookup(const UUID& obj_id) {
    // We only check the cache for statistics purposes
    return mCache->get(obj_id);
}


OSegEntry RedisObjectSegmentation::lookup(const UUID& obj_id) {
    // Check locally
    OSegMap::const_iterator it = mOSeg.find(obj_id);
    if (it != mOSeg.end()) return it->second;

    // Otherwise, kick off the lookup process and return null
    if (mStopping) return OSegEntry::null();
    RedisObjectOperationInfo* ri = new RedisObjectOperationInfo(this, obj_id);
    ensureConnected();
    {
        Lock lck(mMutex);
        redisAsyncCommand(mRedisContext, globalRedisLookupObjectReadFinished, ri, "GET %s%s", mRedisPrefix.c_str(), obj_id.toString().c_str());
    }
    return OSegEntry::null();
}

void RedisObjectSegmentation::finishReadObject(const UUID& obj_id, const String& data_str) {
    REDISOSEG_LOG(detailed, "Finished reading OSEG entry for object " << obj_id.toString());
    if (mStopping) return;

    OSegEntry data(OSegEntry::null());

    std::vector<String> parts;
    boost::algorithm::split(parts, data_str, boost::algorithm::is_any_of(":"));
    if (parts.size() == 2) {
        data.setServer(boost::lexical_cast<uint32>(parts[0]));
        // lexical_cast<float64> refuses to handle integral values,
        // e.g. 1000. Instead, we end up having to manually do the
        // stringstream thing.
        std::istringstream iss(parts[1]);
        float64 new_rad;
        iss >> new_rad;
        data.setRadius(new_rad);
    }

    if (!data.isNull()) mCache->insert(obj_id, data);
    mLookupListener->osegLookupCompleted(obj_id, data);
}

void RedisObjectSegmentation::failReadObject(const UUID& obj_id) {
    REDISOSEG_LOG(error, "Failed to read OSEG entry for object " << obj_id.toString());
    if (mStopping) return;
    mLookupListener->osegLookupCompleted(obj_id, OSegEntry::null());
}

void RedisObjectSegmentation::addNewObject(const UUID& obj_id, float radius) {
    if (mStopping) return;

    mOSeg[obj_id] = OSegEntry(mContext->id(), radius);

    RedisObjectOperationInfo* wi = new RedisObjectOperationInfo(this, obj_id);
    // Note: currently we're keeping compatibility with Redis 1.2. This means
    // that there aren't hashes on the server. Instead, we create and parse them
    // ourselves. This isn't so bad since they are all fixed format anyway.
    std::ostringstream os;
    os << mContext->id() << ":" << radius;
    String valstr = os.str();
    REDISOSEG_LOG(insane, "SETNX " << obj_id.toString() << " " << valstr);
    ensureConnected();
    {
        String obj_id_str = obj_id.toString();
        Lock lck(mMutex);
        if (mRedisHasTransactions) {
            wi->refcount++;
            redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "MULTI");
        }
        wi->refcount++;
        redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "SETNX %s%s %b", mRedisPrefix.c_str(), obj_id_str.c_str(), valstr.c_str(), valstr.size());
        wi->refcount++;
        redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "EXPIRE %s%s %d", mRedisPrefix.c_str(), obj_id_str.c_str(), (int32)mRedisKeyTTL.seconds());
        if (mRedisHasTransactions) {
            wi->refcount++;
            redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "EXEC");
        }
    }
}

void RedisObjectSegmentation::finishWriteNewObject(const UUID& obj_id, OSegWriteListener::OSegAddNewStatus status)
{
    REDISOSEG_LOG(detailed, "Finished writing OSEG entry for object "\
        << obj_id.toString() << " with status " << (int)status);

    if (mStopping) return;

    //only insert into cache if write was successful.
    if (status == OSegWriteListener::SUCCESS)
        mCache->insert(obj_id, mOSeg[obj_id]);

    mWriteListener->osegAddNewFinished(obj_id, status);

    // Schedule for updates
    scheduleObjectRefresh(obj_id);
}

void RedisObjectSegmentation::addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool generateAck) {
    if (mStopping) return;

    mOSeg[obj_id] = OSegEntry(mContext->id(), radius);

    RedisObjectMigratedOperationInfo* wi = new RedisObjectMigratedOperationInfo(this, obj_id, (generateAck ? idServerAckTo : NullServerID));
    // Note: currently we're keeping compatibility with Redis 1.2. This means
    // that there aren't hashes on the server. Instead, we create and parse them
    // ourselves. This isn't so bad since they are all fixed format anyway.
    std::ostringstream os;
    os << mContext->id() << ":" << radius;
    String valstr = os.str();
    REDISOSEG_LOG(insane, "SET " << obj_id.toString() << " " << valstr);
    ensureConnected();
    {
        String obj_id_str = obj_id.toString();
        Lock lck(mMutex);
        if (mRedisHasTransactions) {
            wi->refcount++;
            redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "MULTI");
        }
        wi->refcount++;
        redisAsyncCommand(mRedisContext, globalRedisAddMigratedObjectWriteFinished, wi, "SET %s%s %b", mRedisPrefix.c_str(), obj_id_str.c_str(), valstr.c_str(), valstr.size());
        wi->refcount++;
        redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "EXPIRE %s%s %d", mRedisPrefix.c_str(), obj_id_str.c_str(), (int32)mRedisKeyTTL.seconds());
        if (mRedisHasTransactions) {
            wi->refcount++;
            redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "EXEC");
        }
    }
}

void RedisObjectSegmentation::finishWriteMigratedObject(const UUID& obj_id, ServerID ackTo) {
    REDISOSEG_LOG(detailed, "Finished writing OSEG entry for migrated object " << obj_id.toString());
    if (mStopping) return;

    mCache->insert(obj_id, mOSeg[obj_id]);

    if (ackTo != NullServerID) {
        Sirikata::Protocol::OSeg::MigrateMessageAcknowledge oseg_ack_msg;
        oseg_ack_msg.set_m_servid_from(mContext->id());
        oseg_ack_msg.set_m_servid_to(ackTo);
        oseg_ack_msg.set_m_message_destination(ackTo);
        oseg_ack_msg.set_m_message_from(mContext->id());
        oseg_ack_msg.set_m_objid(obj_id);
        oseg_ack_msg.set_m_objradius( mOSeg[obj_id].radius() );
        queueMigAck(oseg_ack_msg);
    }
}

void RedisObjectSegmentation::removeObject(const UUID& obj_id) {
    if (mStopping) return;

    mOSeg.erase(obj_id);
    mTimeouts.get<objid_tag>().erase(obj_id);
    RedisObjectOperationInfo* wi = new RedisObjectOperationInfo(this, obj_id);
    ensureConnected();
    {
        Lock lck(mMutex);
        redisAsyncCommand(mRedisContext, globalRedisDeleteFinished, wi, "DEL %s%s", mRedisPrefix.c_str(), obj_id.toString().c_str());
    }
}

bool RedisObjectSegmentation::clearToMigrate(const UUID& obj_id) {
    if (mStopping) return false;

    // This can happen if the object is already migrating from this server. In
    // that case, it shouldn't start migrating again.
    // FIXME right now this *might* cover the case of an object that OSeg was
    // notified is migrating to this server but not finished yet?
    if (mOSeg.find(obj_id) == mOSeg.end()) return false;

    return true;
}

void RedisObjectSegmentation::migrateObject(const UUID& obj_id, const OSegEntry& new_server_id) {
    if (mStopping) return;

    // We "migrate" the object by removing it. The other server is responsible
    // for updating Redis
    mOSeg.erase(obj_id);
    mTimeouts.get<objid_tag>().erase(obj_id);
}

void RedisObjectSegmentation::handleMigrateMessageAck(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg) {
    if (mStopping) return;

    OSegEntry data(msg.m_servid_from(), msg.m_objradius());
    UUID obj_id = msg.m_objid();

    mCache->insert(obj_id, data);

    // Finally, this lets the server know the migration has been acked and the
    // object can disconnect
    mWriteListener->osegMigrationAcknowledged(obj_id);

    // Schedule for updates
    scheduleObjectRefresh(obj_id);
}

void RedisObjectSegmentation::handleUpdateOSegMessage(const Sirikata::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg) {
    // Just a cache invalidation/update
    mCache->insert(update_oseg_msg.m_objid(), OSegEntry(update_oseg_msg.servid_obj_on(), update_oseg_msg.m_objradius()));
}



void RedisObjectSegmentation::scheduleObjectRefresh(const UUID& obj_id) {
    Time new_expiry = mContext->simTime() + (mRedisKeyTTL/2);

    ObjectTimeoutsByID& by_id = mTimeouts.get<objid_tag>();
    ObjectTimeoutsByID::iterator it = by_id.find(obj_id);
    if (it != by_id.end()) {
        ObjectTimeoutsByExpiration& by_expiry = mTimeouts.get<expires_tag>();
        ObjectTimeoutsByExpiration::iterator exp_it = mTimeouts.project<expires_tag>(it);
        by_expiry.modify_key(exp_it, boost::lambda::_1=new_expiry);
    }
    else {
        // Should only need to trigger timer if we went empty
        bool was_empty = mTimeouts.empty();
        mTimeouts.insert( ObjectTimeout(obj_id, new_expiry) );
        if (was_empty) startTimeoutHandler();
    }
}

void RedisObjectSegmentation::startTimeoutHandler() {
    // Because we always add a fixed TTL, we should never have to change a
    // timer, only add one.
    if (mTimeouts.empty()) return;

    Time first_expires = mTimeouts.get<expires_tag>().begin()->expires;
    Duration tout = mContext->simTime() - first_expires;
    mExpiryTimer->wait(tout);
}

void RedisObjectSegmentation::processExpiredObjects() {
    Time tnow = mContext->simTime();
    ObjectTimeoutsByExpiration& by_expiry = mTimeouts.get<expires_tag>();
    while(!by_expiry.empty() &&
        tnow > by_expiry.begin()->expires) {
        refreshObjectTimeout(by_expiry.begin()->objid);
        // Don't delete, just update the timeout for the next update
        Time new_expiry = mContext->simTime() + (mRedisKeyTTL/2);
        by_expiry.modify_key(by_expiry.begin(), boost::lambda::_1=new_expiry);
    }
    startTimeoutHandler();
}

void RedisObjectSegmentation::refreshObjectTimeout(const UUID& obj_id) {
    if (mStopping) return;

    assert(mOSeg.find(obj_id) != mOSeg.end());

    RedisObjectOperationInfo* wi = new RedisObjectOperationInfo(this, obj_id);
    ensureConnected();
    REDISOSEG_LOG(insane, "Refreshing timeout for " << obj_id);
    {
        String obj_id_str = obj_id.toString();
        // Note: currently we're keeping compatibility with Redis 1.2. This means
        // that there aren't hashes on the server. Instead, we create and parse them
        // ourselves. This isn't so bad since they are all fixed format anyway.
        std::ostringstream os;
        os << mContext->id() << ":" << mOSeg[obj_id].radius();
        String valstr = os.str();

        Lock lck(mMutex);
        // If we're on older versions of redis, then just setting EXPIRE again
        // isn't enough -- the TTL wasn't reset if they already had one. Not
        // sure exactly which versions this is on, for now use whether it has
        // transactions or not as an indicator. Since they won't have
        // transactions anyway, this code and the completion handler can be a
        // bit simpler
        if (!mRedisHasTransactions) {
            wi->refcount++;
            redisAsyncCommand(mRedisContext, globalRedisRefreshObjectTimeoutWriteFinished, wi, "SET %s%s %b", mRedisPrefix.c_str(), obj_id_str.c_str(), valstr.c_str(), valstr.size());
        }
        wi->refcount++;
        redisAsyncCommand(mRedisContext, globalRedisRefreshObjectTimeoutWriteFinished, wi, "EXPIRE %s%s %d", mRedisPrefix.c_str(), obj_id_str.c_str(), (int32)mRedisKeyTTL.seconds());
    }
}

} // namespace Sirikata
