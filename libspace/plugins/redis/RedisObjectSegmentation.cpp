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

#define REDISOSEG_LOG(lvl,msg) SILOG(redis_oseg, lvl, "[REDISOSEG] " << msg)

namespace Sirikata {

void globalRedisErrorHandler(const redisAsyncContext *c, int status) {
    if (status == REDIS_OK) return;
    REDISOSEG_LOG(error, "Global error handler: " << c->errstr);
}

RedisObjectSegmentation::RedisObjectSegmentation(SpaceContext* con, Network::IOStrand* o_strand, CoordinateSegmentation* cseg, OSegCache* cache, const String& redis_host, uint32 redis_port)
 : ObjectSegmentation(con, o_strand),
   mCSeg(cseg),
   mCache(cache)
{
    mRedisContext = redisAsyncConnect(redis_host.c_str(), redis_port);
    if (mRedisContext->err) {
        REDISOSEG_LOG(error, "Failed to connect to redis: " << mRedisContext->errstr);
        redisAsyncDisconnect(mRedisContext);
        mRedisContext = NULL;
    }
    redisAsyncSetDisconnectCallback(mRedisContext, globalRedisErrorHandler);
}

OSegEntry RedisObjectSegmentation::cacheLookup(const UUID& obj_id) {
    // We only check the cache for statistics purposes
    return mCache->get(obj_id);
}

OSegEntry RedisObjectSegmentation::lookup(const UUID& obj_id) {
    // Redis, so all objects should be available.
    OSegMap::const_iterator it = mOSeg.find(obj_id);
    if (it == mOSeg.end()) {
        REDISOSEG_LOG(warn, "Couldn't find object OSegEntry in RedisObjectSegmentation.");
        return OSegEntry::null();
    }

    return it->second;
}

void RedisObjectSegmentation::addNewObject(const UUID& obj_id, float radius) {
    mOSeg[obj_id] = OSegEntry(mContext->id(), radius);
    mWriteListener->osegWriteFinished(obj_id);
}

void RedisObjectSegmentation::addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool) {
    mOSeg[obj_id] = OSegEntry(mContext->id(), radius);
}

void RedisObjectSegmentation::removeObject(const UUID& obj_id) {
    mOSeg.erase(obj_id);
}

bool RedisObjectSegmentation::clearToMigrate(const UUID& obj_id) {
    REDISOSEG_LOG(error, "RedisObjectSegmentation got clearToMigrate call which should be impossible with single server.");
    return false;
}

void RedisObjectSegmentation::migrateObject(const UUID& obj_id, const OSegEntry& new_server_id) {
    REDISOSEG_LOG(error, "RedisObjectSegmentation got migrateObject call which should be impossible with single server.");
}

} // namespace Sirikata
