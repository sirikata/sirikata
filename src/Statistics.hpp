/*  cbr
 *  Statistics.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_STATISTICS_HPP_
#define _CBR_STATISTICS_HPP_

#include "Utility.hpp"
#include "MotionVector.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "Message.hpp"

#include <boost/thread/recursive_mutex.hpp>

namespace CBR {

template<typename T>
struct Batch {
    static const uint16 max_size = 65535;
    uint16 size;
    T items[max_size];

    Batch() : size(0) {}

    bool full() const {
        return (size >= max_size);
    }

    uint32 avail() const {
        return max_size - size;
    }
};

class BatchedBuffer {
public:
    BatchedBuffer();

    // write the specified number of bytes from the pointer to the buffer
    void write(const void* buf, uint32 nbytes);

    void flush();

    // write the buffer to an ostream
    void store(FILE* os);
private:
    typedef Batch<uint8> ByteBatch;
    ByteBatch* filling;
    std::deque<ByteBatch*> batches;
};


class ServerIDMap;

class Trace {
public:
    static const uint8 ProximityTag = 0;
    static const uint8 ObjectLocationTag = 1;
    static const uint8 SubscriptionTag = 2;
    static const uint8 ServerDatagramQueueInfoTag = 3;
    static const uint8 ServerDatagramQueuedTag = 4;
    static const uint8 ServerDatagramSentTag = 5;
    static const uint8 ServerDatagramReceivedTag = 6;
    static const uint8 PacketQueueInfoTag = 7;
    static const uint8 PacketSentTag = 8;
    static const uint8 PacketReceivedTag = 9;
    static const uint8 SegmentationChangeTag = 10;
    static const uint8 ObjectBeginMigrateTag = 11;
    static const uint8 ObjectAcknowledgeMigrateTag = 12;
    static const uint8 ServerLocationTag = 13;
    static const uint8 ServerObjectEventTag = 14;
    static const uint8 ObjectSegmentationCraqLookupRequestAnalysisTag = 15;
    static const uint8 ObjectSegmentationProcessedRequestAnalysisTag = 16;
    static const uint8 ObjectPingTag = 17;
    static const uint8 RoundTripMigrationTimeAnalysisTag = 18;
    static const uint8 OSegTrackedSetResultAnalysisTag   = 19;
    static const uint8 OSegShutdownEventTag              = 20;
    static const uint8 MessageTimestampTag = 21;
    static const uint8 ObjectGeneratedLocationTag = 22;
    static const uint8 OSegCacheResponseTag = 23;
    static const uint8 OSegLookupNotOnServerAnalysisTag = 24;

    enum MessagePath {
        CREATED,
        SPACE_OUTGOING_MESSAGE,
        SPACE_SERVER_MESSAGE_QUEUE,
        HANDLE_OBJECT_HOST_MESSAGE,
        SELF_LOOP,
        FORWARDED,
        DISPATCHED,
        DELIVERED,
        DESTROYED,
        DROPPED,
        OH_ENQUEUED,
        OH_DEQUEUED,
        HIT_NETWORK,
        OH_DROPPED,
        NUM_PATHS
    };

    Trace(const String& filename);

    void setServerIDMap(ServerIDMap* sidmap);
    //bool timestampMessage(const Time&t, MessagePath path,const Network::Chunk&);
    void timestampMessage(const Time&t, uint64 packetId, MessagePath path, ObjectMessagePort optionalMessageSourcePort=0, ObjectMessagePort optionalMessageDestPort=0, ServerMessagePort optionalMessageType = SERVER_PORT_UNPROCESSED_PACKET);
    void prox(const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc);
    void objectLoc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc);
    void objectGenLoc(const Time& t, const UUID& source, const TimedMotionVector3f& loc);
    void subscription(const Time& t, const UUID& receiver, const UUID& source, bool start);

    // Server received a loc update
    void serverLoc(const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc);
    // Object tracking change
    void serverObjectEvent(const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc);


    void serverDatagramQueueInfo(const Time& t, const ServerID& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight);
    void serverDatagramQueued(const Time& t, const ServerID& dest, uint64 id, uint32 size);
    void serverDatagramSent(const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size);
    void serverDatagramReceived(const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size);

    void packetQueueInfo(const Time& t, const Address4& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight);

    void ping(const Time&sent, const UUID&src, const Time&recv, const UUID& dst, uint64 id, double distance, uint64 uniquePacketId);
    void packetSent(const Time& t, const Address4& dest, uint32 size);
    void packetReceived(const Time& t, const Address4& src, uint32 size);

    void segmentationChanged(const Time& t, const BoundingBox3f& bbox, const ServerID& serverID);

    void objectBeginMigrate(const Time& t, const UUID& ojb_id, const ServerID migrate_from, const ServerID migrate_to);
    void objectAcknowledgeMigrate(const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to);

    void objectSegmentationCraqLookupRequest(const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo);
    void objectSegmentationLookupNotOnServerRequest(const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo);


    void objectSegmentationProcessedRequest(const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 stillInQueue);

    void objectMigrationRoundTrip(const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, int numMilliseconds);

  void processOSegTrackedSetResults(const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, int numMilliseconds);

  void processOSegShutdownEvents(const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet);

  void osegCacheResponse(const Time &t, const ServerID& sID, const UUID& obj);

    void prepareShutdown();
    void shutdown();

private:
    // Thread which flushes data to disk periodically
    void storageThread(const String& filename);

    BatchedBuffer data;
    ServerIDMap* mServerIDMap;
    bool mShuttingDown;

    Thread* mStorageThread;
    Sirikata::AtomicValue<bool> mFinishStorage;

    boost::recursive_mutex mMutex;
}; // class Trace

} // namespace CBR

#endif //_CBR_STATISTICS_HPP_
