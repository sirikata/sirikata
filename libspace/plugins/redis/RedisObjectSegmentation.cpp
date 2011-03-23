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

#define REDISOSEG_LOG(lvl,msg) SILOG(redis_oseg, lvl, msg)

namespace Sirikata {

namespace {

void globalRedisConnectHandler(const redisAsyncContext *c) {
    REDISOSEG_LOG(insane, "Connected.");
}

void globalRedisDisconnectHandler(const redisAsyncContext *c, int status) {
    if (status == REDIS_OK) return;
    REDISOSEG_LOG(error, "Global error handler: " << c->errstr);
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
    RedisObjectSegmentation* oseg;
    UUID obj;
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
    else if (reply->type == REDIS_REPLY_STRING) {
        wi->oseg->finishReadObject(wi->obj, String(reply->str, reply->len));
    }
    else {
        REDISOSEG_LOG(error, "Unexpected redis reply type when reading object " << wi->obj.toString() << ": " << reply->type);
    }

    delete wi;

    if (reply != NULL)
        freeReplyObject(reply);
}

void globalRedisAddNewObjectWriteFinished(redisAsyncContext* c, void* _reply, void* privdata) {
    redisReply *reply = (redisReply*)_reply;
    RedisObjectOperationInfo* wi = (RedisObjectOperationInfo*)privdata;

    if (reply == NULL) {
        REDISOSEG_LOG(error, "Unknown redis error when writing new object " << wi->obj.toString());
    }
    else if (reply->type == REDIS_REPLY_ERROR) {
        REDISOSEG_LOG(error, "Redis error when writing new object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
    }
    else if (reply->type == REDIS_REPLY_STATUS) {
        if (String(reply->str, reply->len) == String("OK"))
            wi->oseg->finishWriteNewObject(wi->obj);
        else
            REDISOSEG_LOG(error, "Redis error when writing new object " << wi->obj.toString() << ": " << String(reply->str, reply->len));
    }
    else {
        REDISOSEG_LOG(error, "Unexpected redis reply type when writing new object " << wi->obj.toString() << ": " << reply->type);
    }

    delete wi;

    if (reply != NULL)
        freeReplyObject(reply);
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

    if (reply != NULL)
        freeReplyObject(reply);
}

} // namespace

RedisObjectSegmentation::RedisObjectSegmentation(SpaceContext* con, Network::IOStrand* o_strand, CoordinateSegmentation* cseg, OSegCache* cache, const String& redis_host, uint32 redis_port)
 : ObjectSegmentation(con, o_strand),
   mCSeg(cseg),
   mCache(cache),
   mReading(false),
   mWriting(false)
{
    mRedisContext = redisAsyncConnect(redis_host.c_str(), redis_port);
    if (mRedisContext->err) {
        REDISOSEG_LOG(error, "Failed to connect to redis: " << mRedisContext->errstr);
        redisAsyncDisconnect(mRedisContext);
        mRedisContext = NULL;
    } else {
        REDISOSEG_LOG(insane, "Optimistically connected to redis.");
    }
    redisAsyncSetConnectCallback(mRedisContext, globalRedisConnectHandler);
    redisAsyncSetDisconnectCallback(mRedisContext, globalRedisDisconnectHandler);

    mRedisContext->evAddRead = globalRedisAddRead;
    mRedisContext->evDelRead = globalRedisDelRead;
    mRedisContext->evAddWrite = globalRedisAddWrite;
    mRedisContext->evDelWrite = globalRedisDelWrite;
    mRedisContext->evCleanup = globalRedisCleanup;
    mRedisContext->_adapter_data = this;

    // Wrap this connections file descripter in ASIO
    using boost::asio::posix::stream_descriptor;
    mRedisFD = new stream_descriptor(mContext->ioService->asioService());
    mRedisFD->assign(mRedisContext->c.fd);
}

RedisObjectSegmentation::~RedisObjectSegmentation() {
    delete mRedisFD;
    mRedisFD = NULL;
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
}

void RedisObjectSegmentation::startRead() {
    if (!mReading) return;
    mRedisFD->async_read_some(boost::asio::null_buffers(),
        boost::bind(&RedisObjectSegmentation::readHandler, this, boost::asio::placeholders::error));
}

void RedisObjectSegmentation::startWrite() {
    if (!mWriting) return;
    mRedisFD->async_write_some(boost::asio::null_buffers(),
        boost::bind(&RedisObjectSegmentation::writeHandler, this, boost::asio::placeholders::error));
}

void RedisObjectSegmentation::readHandler(const boost::system::error_code& ec) {
    if (ec) {
        REDISOSEG_LOG(error, "Error in read handler.");
        return;
    }

    redisAsyncHandleRead(mRedisContext);
    startRead();
}

void RedisObjectSegmentation::writeHandler(const boost::system::error_code& ec) {
    if (ec) {
        REDISOSEG_LOG(error, "Error in write handler.");
        return;
    }

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
    RedisObjectOperationInfo* ri = new RedisObjectOperationInfo();
    ri->oseg = this;
    ri->obj = obj_id;
    redisAsyncCommand(mRedisContext, globalRedisLookupObjectReadFinished, ri, "GET %s", obj_id.toString().c_str());
    return OSegEntry::null();
}

void RedisObjectSegmentation::finishReadObject(const UUID& obj_id, const String& data_str) {
    REDISOSEG_LOG(detailed, "Finished reading OSEG entry for object " << obj_id.toString());

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
    mLookupListener->osegLookupCompleted(obj_id, OSegEntry::null());
}

void RedisObjectSegmentation::addNewObject(const UUID& obj_id, float radius) {
    mOSeg[obj_id] = OSegEntry(mContext->id(), radius);

    RedisObjectOperationInfo* wi = new RedisObjectOperationInfo();
    wi->oseg = this;
    wi->obj = obj_id;
    // Note: currently we're keeping compatibility with Redis 1.2. This means
    // that there aren't hashes on the server. Instead, we create and parse them
    // ourselves. This isn't so bad since they are all fixed format anyway.
    std::ostringstream os;
    os << mContext->id() << ":" << radius;
    String valstr = os.str();
    REDISOSEG_LOG(insane, "SET " << obj_id.toString() << " " << valstr);
    redisAsyncCommand(mRedisContext, globalRedisAddNewObjectWriteFinished, wi, "SET %s %b", obj_id.toString().c_str(), valstr.c_str(), valstr.size());
}

void RedisObjectSegmentation::finishWriteNewObject(const UUID& obj_id) {
    REDISOSEG_LOG(detailed, "Finished writing OSEG entry for object " << obj_id.toString());
    mWriteListener->osegWriteFinished(obj_id);
}

void RedisObjectSegmentation::addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool) {
    mOSeg[obj_id] = OSegEntry(mContext->id(), radius);
}

void RedisObjectSegmentation::removeObject(const UUID& obj_id) {
    mOSeg.erase(obj_id);
    RedisObjectOperationInfo* wi = new RedisObjectOperationInfo();
    wi->oseg = this;
    wi->obj = obj_id;
    redisAsyncCommand(mRedisContext, globalRedisDeleteFinished, wi, "DEL %s", obj_id.toString().c_str());
}

bool RedisObjectSegmentation::clearToMigrate(const UUID& obj_id) {
    REDISOSEG_LOG(error, "RedisObjectSegmentation got clearToMigrate call which should be impossible with single server.");
    return false;
}

void RedisObjectSegmentation::migrateObject(const UUID& obj_id, const OSegEntry& new_server_id) {
    REDISOSEG_LOG(error, "RedisObjectSegmentation got migrateObject call which should be impossible with single server.");
}

} // namespace Sirikata
