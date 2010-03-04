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
#include "Message.hpp"
#include "OSegLookupTraceToken.hpp"

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
    struct IOVec {
        IOVec()
                : base(NULL),
                  len(0)
        {}

        IOVec(const void* _b, uint32 _l)
                : base(_b),
                  len(_l)
        {}

        const void* base;
        uint32 len;
    };

    BatchedBuffer();

    void write(const IOVec* iov, uint32 iovcnt);

    void flush();

    // write the buffer to an ostream
    void store(FILE* os);
private:
    // write the specified number of bytes from the pointer to the buffer
    void write(const void* buf, uint32 nbytes);

    typedef Batch<uint8> ByteBatch;
    boost::recursive_mutex mMutex;
    ByteBatch* filling;
    std::deque<ByteBatch*> batches;
};


class Trace {
public:
    static const uint8 ProximityTag = 0;
    static const uint8 ObjectLocationTag = 1;
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
    static const uint8 ObjectGeneratedLocationTag = 22;
    static const uint8 OSegCacheResponseTag = 23;
    static const uint8 OSegLookupNotOnServerAnalysisTag = 24;
    static const uint8 OSegCumulativeTraceAnalysisTag   = 25;

    static const uint8 MessageTimestampTag = 30;
    static const uint8 MessageCreationTimestampTag = 31;


    enum MessagePath {
        NONE, // Used when tag is needed but we don't have a name for it

        // Object Host Checkpoints
        CREATED,
        DESTROYED,
        OH_HIT_NETWORK,
        OH_DROPPED_AT_SEND,
        OH_NET_RECEIVED,
        OH_DROPPED_AT_RECEIVE_QUEUE,
        OH_RECEIVED,
        SPACE_DROPPED_AT_MAIN_STRAND_CROSSING,
        // Space Checkpoints
        HANDLE_OBJECT_HOST_MESSAGE,
        HANDLE_SPACE_MESSAGE,

        FORWARDED_LOCALLY,
        DROPPED_AT_FORWARDED_LOCALLY,

        FORWARDING_STARTED,

        FORWARDED_LOCALLY_SLOW_PATH,

        // FIXME could follow FORWARDED_LOCALLY_SLOW_PATH or OSEG_LOOKUP_STARTED
        DROPPED_DURING_FORWARDING,

        OSEG_LOOKUP_STARTED,
        OSEG_CACHE_LOOKUP_FINISHED,
        OSEG_SERVER_LOOKUP_FINISHED,
        OSEG_LOOKUP_FINISHED,

        SPACE_TO_SPACE_ENQUEUED,
        DROPPED_AT_SPACE_ENQUEUED,

        SPACE_TO_SPACE_HIT_NETWORK,

        SPACE_TO_SPACE_READ_FROM_NET,
        SPACE_TO_SPACE_SMR_DEQUEUED,

        SPACE_TO_OH_ENQUEUED,

        NUM_PATHS
    };

    // Initialize options which flip recording of trace data on and off
    static void InitOptions();

    Trace(const String& filename);


#define CREATE_TRACE_CHECK_DECL(___name)           \
    bool check ## ___name () const;
#define CREATE_TRACE_EVAL_DECL(___name, ...)    \
    void ___name( __VA_ARGS__ );

#define CREATE_TRACE_DECL(___name, ...)         \
    CREATE_TRACE_CHECK_DECL(___name)            \
    CREATE_TRACE_EVAL_DECL(___name, __VA_ARGS__)

    CREATE_TRACE_DECL(timestampMessageCreation, const Time&t, uint64 packetId, MessagePath path, ObjectMessagePort optionalMessageSourcePort=0, ObjectMessagePort optionalMessageDestPort=0);
    CREATE_TRACE_DECL(timestampMessage, const Time&t, uint64 packetId, MessagePath path);

    CREATE_TRACE_DECL(prox, const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc);
    CREATE_TRACE_DECL(objectLoc, const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc);
    CREATE_TRACE_DECL(objectGenLoc, const Time& t, const UUID& source, const TimedMotionVector3f& loc);

    // Server received a loc update
    CREATE_TRACE_DECL(serverLoc, const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc);
    // Object tracking change
    CREATE_TRACE_DECL(serverObjectEvent, const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc);


    CREATE_TRACE_DECL(serverDatagramQueued, const Time& t, const ServerID& dest, uint64 id, uint32 size);
    CREATE_TRACE_DECL(serverDatagramSent, const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size);
    CREATE_TRACE_DECL(serverDatagramReceived, const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size);

    CREATE_TRACE_DECL(packetQueueInfo, const Time& t, const ServerID& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight);

    CREATE_TRACE_DECL(ping, const Time&sent, const UUID&src, const Time&recv, const UUID& dst, uint64 id, double distance, uint64 uniquePacketId);
    CREATE_TRACE_DECL(packetSent, const Time& t, const ServerID& dest, uint32 size);
    CREATE_TRACE_DECL(packetReceived, const Time& t, const ServerID& src, uint32 size);

    CREATE_TRACE_DECL(segmentationChanged, const Time& t, const BoundingBox3f& bbox, const ServerID& serverID);

    CREATE_TRACE_DECL(objectBeginMigrate, const Time& t, const UUID& ojb_id, const ServerID migrate_from, const ServerID migrate_to);
    CREATE_TRACE_DECL(objectAcknowledgeMigrate, const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to);

    CREATE_TRACE_DECL(objectSegmentationCraqLookupRequest, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo);
    CREATE_TRACE_DECL(objectSegmentationLookupNotOnServerRequest, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo);


    CREATE_TRACE_DECL(objectSegmentationProcessedRequest, const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 stillInQueue);

    CREATE_TRACE_DECL(objectMigrationRoundTrip, const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, int numMilliseconds);

    CREATE_TRACE_DECL(processOSegTrackedSetResults, const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, int numMilliseconds);

    CREATE_TRACE_DECL(processOSegShutdownEvents, const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet);

    CREATE_TRACE_DECL(osegCacheResponse, const Time &t, const ServerID& sID, const UUID& obj);

    CREATE_TRACE_DECL(osegCumulativeResponse, const Time &t, OSegLookupTraceToken* traceToken);

public:
  void prepareShutdown();
  void shutdown();

private:
    // Helper to prepend framing (size and payload type hint)
    void writeRecord(uint16 type_hint, BatchedBuffer::IOVec* data, uint32 iovcnt);

    // Thread which flushes data to disk periodically
    void storageThread(const String& filename);

    BatchedBuffer data;
    bool mShuttingDown;

    Thread* mStorageThread;
    Sirikata::AtomicValue<bool> mFinishStorage;

    // OptionValues that turn tracing on/off
    static OptionValue* mLogObject;
    static OptionValue* mLogLocProx;
    static OptionValue* mLogOSeg;
    static OptionValue* mLogCSeg;
    static OptionValue* mLogMigration;
    static OptionValue* mLogDatagram;
    static OptionValue* mLogPacket;
    static OptionValue* mLogPing;
    static OptionValue* mLogMessage;
}; // class Trace

// This is how you should *actually*
#define TRACE(___trace, ___name, ...)            \
    {                                            \
        if ( ___trace-> check ## ___name () )    \
            ___trace-> ___name ( __VA_ARGS__ );  \
    } while(0)
// To check that you aren't using tracing via the incorrect interface (that is,
// directly instead of via macros) uncomment the following and mark the DECL's
// in Trace as private.
//#undef TRACE
//#define TRACE(___trace, ___name, ...)

// This version of the TRACE macro automatically uses mContext->trace() and
// passes mContext->simTime() as the first argument, which is the most common
// form.
#define CONTEXT_TRACE(___name, ...)                 \
    TRACE( mContext->trace(), ___name, mContext->simTime(), __VA_ARGS__)

// This version is like the above, but you can specify the time yourself.  Use
// this if you already called Context::simTime() recently. (It also works for
// cases where the first parameter is not the current time.)
#define CONTEXT_TRACE_NO_TIME(___name, ...)             \
    TRACE( mContext->trace(), ___name, __VA_ARGS__)

} // namespace CBR




#ifdef CBR_TIMESTAMP_PACKETS
// The most complete macro, allows you to specify everything
#define TIMESTAMP_FULL(trace, time, packetId, path) TRACE(trace, timestampMessage, time, packetId, path)

// Slightly simplified version, works everywhere mContext->trace() and mContext->simTime() are valid
#define TIMESTAMP_SIMPLE(packetId, path) TIMESTAMP_FULL(mContext->trace(), mContext->simTime(), packetId, path)

// Further simplified version, works as long as packet is a valid pointer to a packet at the time this is called
#define TIMESTAMP(packet, path) TIMESTAMP_SIMPLE(packet->unique(), path)

// In cases where you need to split the time between recording the info for a timestamp and actually performing
// the timestamp, use a _START _END combination. Generally this should only be needed if you are timestamping
// after a message might be deleted or is out of your control (e.g. you pushed onto a queue or destroyed it).
#define TIMESTAMP_START(prefix, packet)                                 \
    Sirikata::uint64 prefix ## _uniq = packet->unique();

#define TIMESTAMP_END(prefix, path) TIMESTAMP_SIMPLE(prefix ## _uniq, path)

// When the message has been wrapped in a space node -> space node message, we
// need to use a special form, using the payload_id recorded in that message's
// header
#define TIMESTAMP_PAYLOAD(packet, path) TIMESTAMP_SIMPLE(packet->payload_id(), path)

#define TIMESTAMP_PAYLOAD_START(prefix, packet)                                 \
    Sirikata::uint64 prefix ## _uniq = packet->payload_id();

#define TIMESTAMP_PAYLOAD_END(prefix, path) TIMESTAMP_SIMPLE(prefix ## _uniq, path)


#define TIMESTAMP_CREATED(packet, path) TRACE(mContext->trace(), timestampMessageCreation, mContext->simTime(), packet->unique(), path, packet->source_port(), packet->dest_port())

#else //CBR_TIMESTAMP_PACKETS
#define TIMESTAMP_FULL(trace, time, packetId, path)
#define TIMESTAMP_SIMPLE(packetId, path)
#define TIMESTAMP(packet, path)
#define TIMESTAMP_START(prefix, packet)
#define TIMESTAMP_END(prefix, path)
#define TIMESTAMP_PAYLOAD(packet, path)
#define TIMESTAMP_PAYLOAD_START(prefix, packet)
#define TIMESTAMP_PAYLOAD_END(prefix, path)
#define TIMESTAMP_CREATED(packet, path)
#endif //CBR_TIMESTAMP_PACKETS

#endif //_CBR_STATISTICS_HPP_
