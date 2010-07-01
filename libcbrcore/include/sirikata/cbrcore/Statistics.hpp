/*  Sirikata
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

#ifndef _SIRIKATA_STATISTICS_HPP_
#define _SIRIKATA_STATISTICS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Thread.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>

#include <boost/thread/recursive_mutex.hpp>

namespace Sirikata {

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

#define TRACE_DROP(nam) ((mContext->trace()->drops.n[::Sirikata::Trace::Drops::nam]=#nam )&&++(mContext->trace()->drops.d[::Sirikata::Trace::Drops::nam]))

namespace Trace {

struct Drops {
    enum {
        OH_DROPPED_AT_SEND,
        OH_DROPPED_AT_RECEIVE_QUEUE,
        SPACE_DROPPED_AT_MAIN_STRAND_CROSSING,
        DROPPED_AT_FORWARDED_LOCALLY,
        DROPPED_DURING_FORWARDING,
        DROPPED_DURING_FORWARDING_ROUTING,
        DROPPED_AT_SPACE_ENQUEUED,
        DROPPED_CSFQ_OVERFLOW,
        DROPPED_CSFQ_PROBABILISTIC,
        NUM_DROPS
    };
    uint64 d[NUM_DROPS];
    const char*n[NUM_DROPS];
    Drops() {
        memset(d,0,NUM_DROPS*sizeof(uint64));
        memset(n,0,NUM_DROPS*sizeof(const char*));
    }
    std::ostream&output(std::ostream&output);
};

#define ProximityTag 0
#define ObjectLocationTag 1
#define ServerDatagramQueuedTag 4
#define ServerDatagramSentTag 5
#define ServerDatagramReceivedTag 6
#define SegmentationChangeTag 10

#define MigrationBeginTag 11
#define MigrationAckTag 12
#define MigrationRoundTripTag 18

#define ServerLocationTag 13
#define ServerObjectEventTag 14
#define ObjectSegmentationCraqLookupRequestAnalysisTag 15
#define ObjectSegmentationProcessedRequestAnalysisTag 16
#define ObjectPingTag 17
#define ObjectPingCreatedTag 32

#define OSegTrackedSetResultAnalysisTag   19
#define OSegShutdownEventTag              20
#define ObjectGeneratedLocationTag 22
#define OSegCacheResponseTag 23
#define OSegLookupNotOnServerAnalysisTag 24
#define OSegCumulativeTraceAnalysisTag   25
#define MessageTimestampTag 30
#define MessageCreationTimestampTag 31

#define ObjectConnectedTag 33

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

    OSEG_CACHE_CHECK_STARTED,
    OSEG_CACHE_CHECK_FINISHED,

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

class Trace {
public:
    Drops drops;

    ~Trace();

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



// This macro simplifies creating the methods that check if we should actually
// perform the trace.
#define CREATE_TRACE_CHECK_DEF(__klass, ___name , ___log_var)    \
    bool __klass :: check ## ___name () const {          \
        return ___log_var->as<bool>();                \
    }

#define CREATE_TRACE_EVAL_DEF(__klass, ___name , ... )   \
    void __klass :: ___name ( __VA_ARGS__ )

// This macro takes care of everything -- just specify the name, the
// OptionValue* to check, argument types, and then follow it with the body.
#define CREATE_TRACE_DEF(__klass, ___name , ___log_var, ... )    \
    CREATE_TRACE_CHECK_DEF(__klass, ___name, ___log_var)          \
    CREATE_TRACE_EVAL_DEF(__klass, ___name, __VA_ARGS__)


    CREATE_TRACE_DECL(timestampMessageCreation, const Time&t, uint64 packetId, MessagePath path, ObjectMessagePort optionalMessageSourcePort=0, ObjectMessagePort optionalMessageDestPort=0);
    CREATE_TRACE_DECL(timestampMessage, const Time&t, uint64 packetId, MessagePath path);


    // Helper to prepend framing (size and payload type hint)
    void writeRecord(uint16 type_hint, BatchedBuffer::IOVec* data, uint32 iovcnt);

    // Helper to prepend framing (size and payload type hint)
    template<typename T>
    void writeRecord(uint16 type_hint, const T& pl) {
        if (mShuttingDown) return;

        std::string serialized_pl;
        bool serialized_success = pl.SerializeToString(&serialized_pl);
        assert(serialized_success);

        const uint32 num_data = 1;
        BatchedBuffer::IOVec data_vec[num_data] = {
            BatchedBuffer::IOVec(&(serialized_pl[0]), serialized_pl.size())
        };
        writeRecord(type_hint, data_vec, num_data);
    }


public:

  void prepareShutdown();
  void shutdown();

private:
    // Thread which flushes data to disk periodically
    void storageThread(const String& filename);

    BatchedBuffer data;
    bool mShuttingDown;

    Thread* mStorageThread;
    Sirikata::AtomicValue<bool> mFinishStorage;

    // OptionValues that turn tracing on/off
    static OptionValue* mLogMessage;
}; // class Trace

} // namespace Trace

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

} // namespace Sirikata




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

#endif //_SIRIKATA_STATISTICS_HPP_
