/*  cbr
 *  Statistics.cpp
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

#include "Statistics.hpp"
#include "Message.hpp"
#include "Options.hpp"
#include "CBR_Header.pbj.hpp"
#include <iostream>

#include <boost/thread/locks.hpp>

namespace CBR {

BatchedBuffer::BatchedBuffer()
 : filling(NULL)
{
}

// write the specified number of bytes from the pointer to the buffer
void BatchedBuffer::write(const void* buf, uint32 nbytes) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    const uint8* bufptr = (const uint8*)buf;
    while( nbytes > 0 ) {
        if (filling == NULL)
            filling = new ByteBatch();

        uint32 to_copy = std::min(filling->avail(), nbytes);

        memcpy( &filling->items[filling->size], bufptr, to_copy);
        filling->size += to_copy;
        bufptr += to_copy;
        nbytes -= to_copy;

        if (filling->full())
            flush();
    }
}

void BatchedBuffer::write(const IOVec* iov, uint32 iovcnt) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    for(uint32 i = 0; i < iovcnt; i++)
        write(iov[i].base, iov[i].len);
}

void BatchedBuffer::flush() {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    if (filling == NULL)
        return;

    batches.push_back(filling);
    filling = NULL;
}

// write the buffer to an ostream
void BatchedBuffer::store(FILE* os) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    std::deque<ByteBatch*> bufs;
    batches.swap(bufs);

    for(std::deque<ByteBatch*>::iterator it = bufs.begin(); it != bufs.end(); it++) {
        ByteBatch* bb = *it;
        fwrite((void*)&(bb->items[0]), 1, bb->size, os);
        delete bb;
    }
}


const uint8 Trace::MessageTimestampTag;
const uint8 Trace::MessageCreationTimestampTag;

const uint8 Trace::ObjectPingTag;
const uint8 Trace::ProximityTag;
const uint8 Trace::ObjectLocationTag;
const uint8 Trace::ServerDatagramQueuedTag;
const uint8 Trace::ServerDatagramSentTag;
const uint8 Trace::ServerDatagramReceivedTag;
const uint8 Trace::PacketQueueInfoTag;
const uint8 Trace::PacketSentTag;
const uint8 Trace::PacketReceivedTag;
const uint8 Trace::SegmentationChangeTag;
const uint8 Trace::ObjectBeginMigrateTag;
const uint8 Trace::ObjectAcknowledgeMigrateTag;

const uint8 Trace::ServerLocationTag;
const uint8 Trace::ServerObjectEventTag;

const uint8 Trace::ObjectSegmentationCraqLookupRequestAnalysisTag;
const uint8 Trace::ObjectSegmentationProcessedRequestAnalysisTag;

const uint8 Trace::RoundTripMigrationTimeAnalysisTag;
const uint8 Trace::OSegTrackedSetResultAnalysisTag;
const uint8 Trace::OSegShutdownEventTag;
const uint8 Trace::ObjectGeneratedLocationTag;
const uint8 Trace::OSegCacheResponseTag;
const uint8 Trace::OSegLookupNotOnServerAnalysisTag;
const uint8 Trace::OSegCumulativeTraceAnalysisTag;

OptionValue* Trace::mLogObject;
OptionValue* Trace::mLogLocProx;
OptionValue* Trace::mLogOSeg;
OptionValue* Trace::mLogCSeg;
OptionValue* Trace::mLogMigration;
OptionValue* Trace::mLogDatagram;
OptionValue* Trace::mLogPacket;
OptionValue* Trace::mLogPing;
OptionValue* Trace::mLogMessage;

#define TRACE_OBJECT_NAME        "trace-object"
#define TRACE_LOCPROX_NAME       "trace-locprox"
#define TRACE_OSEG_NAME          "trace-oseg"
#define TRACE_CSEG_NAME          "trace-cseg"
#define TRACE_MIGRATION_NAME     "trace-migration"
#define TRACE_DATAGRAM_NAME      "trace-datagram"
#define TRACE_PACKET_NAME        "trace-packet"
#define TRACE_PING_NAME          "trace-ping"
#define TRACE_MESSAGE_NAME       "trace-message"

void Trace::InitOptions() {
    mLogObject = new OptionValue(TRACE_OBJECT_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogLocProx = new OptionValue(TRACE_LOCPROX_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogOSeg = new OptionValue(TRACE_OSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogCSeg = new OptionValue(TRACE_CSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogMigration = new OptionValue(TRACE_MIGRATION_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogDatagram = new OptionValue(TRACE_DATAGRAM_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogPacket = new OptionValue(TRACE_PACKET_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogPing = new OptionValue(TRACE_PING_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogMessage = new OptionValue(TRACE_MESSAGE_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");

    InitializeOptions::module(CBR_MODULE)
        .addOption(mLogObject)
        .addOption(mLogLocProx)
        .addOption(mLogOSeg)
        .addOption(mLogCSeg)
        .addOption(mLogMigration)
        .addOption(mLogDatagram)
        .addOption(mLogPacket)
        .addOption(mLogPing)
        .addOption(mLogMessage)
        ;
}

Trace::Trace(const String& filename)
 : mShuttingDown(false),
   mStorageThread(NULL),
   mFinishStorage(false)
{
    mStorageThread = new Thread( std::tr1::bind(&Trace::storageThread, this, filename) );
}

void Trace::prepareShutdown() {
    mShuttingDown = true;
}

void Trace::shutdown() {
    data.flush();
    mFinishStorage = true;
    mStorageThread->join();
    delete mStorageThread;
}

void Trace::storageThread(const String& filename) {
    FILE* of = fopen(filename.c_str(), "wb");

    while( !mFinishStorage.read() ) {
        data.store(of);
        fflush(of);

        usleep( Duration::seconds(1).toMicroseconds() );
    }
    data.store(of);
    fflush(of);

    fsync(fileno(of));
    fclose(of);
}

void Trace::writeRecord(uint16 type_hint, BatchedBuffer::IOVec* data_orig, uint32 iovcnt) {
    assert(iovcnt < 30);

    BatchedBuffer::IOVec data_vec[32];

    uint32 total_size = 0;
    for(uint32 i = 0; i < iovcnt; i++)
        total_size += data_orig[i].len;


    data_vec[0] = BatchedBuffer::IOVec(&total_size, sizeof(total_size));
    data_vec[1] = BatchedBuffer::IOVec(&type_hint, sizeof(type_hint));
    for(uint32 i = 0; i < iovcnt; i++)
        data_vec[i+2] = data_orig[i];

    data.write(data_vec, iovcnt+2);
}

// This macro simplifies creating the methods that check if we should actually
// perform the trace.
#define CREATE_TRACE_CHECK_DEF(___name , ___log_var)  \
    bool Trace:: check ## ___name () const {          \
        return ___log_var->as<bool>();                \
    }

#define CREATE_TRACE_EVAL_DEF(___name , ... ) \
    void Trace:: ___name ( __VA_ARGS__ )

// This macro takes care of everything -- just specify the name, the
// OptionValue* to check, argument types, and then follow it with the body.
#define CREATE_TRACE_DEF(___name , ___log_var, ... )    \
    CREATE_TRACE_CHECK_DEF(___name, ___log_var)         \
    CREATE_TRACE_EVAL_DEF(___name, __VA_ARGS__)


CREATE_TRACE_DEF(prox, mLogObject, const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&source, sizeof(source)),
        BatchedBuffer::IOVec(&entered, sizeof(entered)),
        BatchedBuffer::IOVec(&loc, sizeof(loc))
    };
    writeRecord(ProximityTag, data_vec, num_data);
}

CREATE_TRACE_DEF(objectGenLoc, mLogObject, const Time& t, const UUID& source, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&source, sizeof(source)),
        BatchedBuffer::IOVec(&loc, sizeof(loc)),
    };
    writeRecord(ObjectGeneratedLocationTag, data_vec, num_data);
}

CREATE_TRACE_DEF(objectLoc, mLogObject, const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&source, sizeof(source)),
        BatchedBuffer::IOVec(&loc, sizeof(loc)),
    };
    writeRecord(ObjectLocationTag, data_vec, num_data);
}

CREATE_TRACE_DEF(timestampMessageCreation, mLogMessage, const Time&sent, uint64 uid, MessagePath path, ObjectMessagePort srcprt, ObjectMessagePort dstprt) {
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&sent, sizeof(sent)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
        BatchedBuffer::IOVec(&path, sizeof(path)),
        BatchedBuffer::IOVec(&srcprt, sizeof(srcprt)),
        BatchedBuffer::IOVec(&dstprt, sizeof(dstprt)),
    };
    writeRecord(MessageCreationTimestampTag, data_vec, num_data);
}

CREATE_TRACE_DEF(timestampMessage, mLogMessage, const Time&sent, uint64 uid, MessagePath path) {
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&sent, sizeof(sent)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
        BatchedBuffer::IOVec(&path, sizeof(path)),
    };
    writeRecord(MessageTimestampTag, data_vec, num_data);
}

CREATE_TRACE_DEF(ping, mLogPing, const Time& src, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint64 uid) {
    if (mShuttingDown) return;

    const uint32 num_data = 7;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&src, sizeof(src)),
        BatchedBuffer::IOVec(&sender, sizeof(sender)),
        BatchedBuffer::IOVec(&dst, sizeof(dst)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&id, sizeof(id)),
        BatchedBuffer::IOVec(&distance, sizeof(distance)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
    };
    writeRecord(ObjectPingTag, data_vec, num_data);
}

CREATE_TRACE_DEF(serverLoc, mLogLocProx, const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&sender, sizeof(sender)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&obj, sizeof(obj)),
        BatchedBuffer::IOVec(&loc, sizeof(loc)),
    };
    writeRecord(ServerLocationTag, data_vec, num_data);
}

CREATE_TRACE_DEF(serverObjectEvent, mLogLocProx, const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc) {
    if (mShuttingDown) return;

    uint8 raw_added = (added ? 1 : 0);

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&source, sizeof(source)),
        BatchedBuffer::IOVec(&dest, sizeof(dest)),
        BatchedBuffer::IOVec(&obj, sizeof(obj)),
        BatchedBuffer::IOVec(&raw_added, sizeof(raw_added)),
        BatchedBuffer::IOVec(&loc, sizeof(loc)),
    };
    writeRecord(ServerObjectEventTag, data_vec, num_data);
}

CREATE_TRACE_DEF(serverDatagramQueued, mLogDatagram, const Time& t, const ServerID& dest, uint64 id, uint32 size) {
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&dest, sizeof(dest)),
        BatchedBuffer::IOVec(&id, sizeof(id)),
        BatchedBuffer::IOVec(&size, sizeof(size)),
    };
    writeRecord(ServerDatagramQueuedTag, data_vec, num_data);
}

CREATE_TRACE_DEF(serverDatagramSent, mLogDatagram, const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size) {
    if (mShuttingDown) return;

    const uint32 num_data = 7;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&start_time, sizeof(start_time)),
        BatchedBuffer::IOVec(&dest, sizeof(dest)),
        BatchedBuffer::IOVec(&id, sizeof(id)),
        BatchedBuffer::IOVec(&size, sizeof(size)),
        BatchedBuffer::IOVec(&weight, sizeof(weight)),
        BatchedBuffer::IOVec(&start_time, sizeof(start_time)),
        BatchedBuffer::IOVec(&end_time, sizeof(end_time)),
    };
    writeRecord(ServerDatagramSentTag, data_vec, num_data);
}

CREATE_TRACE_DEF(serverDatagramReceived, mLogDatagram, const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size) {
    if (mShuttingDown) return;

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&start_time, sizeof(start_time)),
        BatchedBuffer::IOVec(&src, sizeof(src)),
        BatchedBuffer::IOVec(&id, sizeof(id)),
        BatchedBuffer::IOVec(&size, sizeof(size)),
        BatchedBuffer::IOVec(&start_time, sizeof(start_time)),
        BatchedBuffer::IOVec(&end_time, sizeof(end_time)),
    };
    writeRecord(ServerDatagramReceivedTag, data_vec, num_data);
}

CREATE_TRACE_DEF(packetQueueInfo, mLogPacket, const Time& t, const ServerID& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight) {
    if (mShuttingDown) return;

    const uint32 num_data = 8;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&dest, sizeof(dest)),
        BatchedBuffer::IOVec(&send_size, sizeof(send_size)),
        BatchedBuffer::IOVec(&send_queued, sizeof(send_queued)),
        BatchedBuffer::IOVec(&send_weight, sizeof(send_weight)),
        BatchedBuffer::IOVec(&receive_size, sizeof(receive_size)),
        BatchedBuffer::IOVec(&receive_queued, sizeof(receive_queued)),
        BatchedBuffer::IOVec(&receive_weight, sizeof(receive_weight)),
    };
    writeRecord(PacketQueueInfoTag, data_vec, num_data);
}

CREATE_TRACE_DEF(packetSent, mLogPacket, const Time& t, const ServerID& dest, uint32 size) {
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&dest, sizeof(dest)),
        BatchedBuffer::IOVec(&size, sizeof(size)),
    };
    writeRecord(PacketSentTag, data_vec, num_data);
}

CREATE_TRACE_DEF(packetReceived, mLogPacket, const Time& t, const ServerID& src, uint32 size) {
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&src, sizeof(src)),
        BatchedBuffer::IOVec(&size, sizeof(size)),
    };
    writeRecord(PacketReceivedTag, data_vec, num_data);
}

CREATE_TRACE_DEF(segmentationChanged, mLogCSeg, const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&bbox, sizeof(bbox)),
        BatchedBuffer::IOVec(&serverID, sizeof(serverID)),
    };
    writeRecord(SegmentationChangeTag, data_vec, num_data);
}

CREATE_TRACE_DEF(objectBeginMigrate, mLogMigration, const Time& t, const UUID& obj_id, const ServerID migrate_from, const ServerID migrate_to) {
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
        BatchedBuffer::IOVec(&migrate_from, sizeof(migrate_from)),
        BatchedBuffer::IOVec(&migrate_to, sizeof(migrate_to)),
    };
    writeRecord(ObjectBeginMigrateTag, data_vec, num_data);
  }

CREATE_TRACE_DEF(objectAcknowledgeMigrate, mLogMigration, const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to) {
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
        BatchedBuffer::IOVec(&acknowledge_from, sizeof(acknowledge_from)),
        BatchedBuffer::IOVec(&acknowledge_to, sizeof(acknowledge_to)),
    };
    writeRecord(ObjectAcknowledgeMigrateTag, data_vec, num_data);
  }


CREATE_TRACE_DEF(objectSegmentationCraqLookupRequest, mLogOSeg, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo){
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
        BatchedBuffer::IOVec(&sID_lookupTo, sizeof(sID_lookupTo)),
    };
    writeRecord(ObjectSegmentationCraqLookupRequestAnalysisTag, data_vec, num_data);
  }

CREATE_TRACE_DEF(objectSegmentationProcessedRequest, mLogOSeg, const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 objectsInQueue)
  {
    if (mShuttingDown) return;

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
        BatchedBuffer::IOVec(&sID_processor, sizeof(sID_processor)),
        BatchedBuffer::IOVec(&sID, sizeof(sID)),
        BatchedBuffer::IOVec(&dTime, sizeof(dTime)),
        BatchedBuffer::IOVec(&objectsInQueue, sizeof(objectsInQueue)),
    };
    writeRecord(ObjectSegmentationProcessedRequestAnalysisTag, data_vec, num_data);
  }


CREATE_TRACE_DEF(objectMigrationRoundTrip, mLogMigration, const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, int numMilliseconds)
{
  if (mShuttingDown) return;

  const uint32 num_data = 5;
  BatchedBuffer::IOVec data_vec[num_data] = {
      BatchedBuffer::IOVec(&t, sizeof(t)),
      BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
      BatchedBuffer::IOVec(&sID_migratingFrom, sizeof(sID_migratingFrom)),
      BatchedBuffer::IOVec(&sID_migratingTo, sizeof(sID_migratingTo)),
      BatchedBuffer::IOVec(&numMilliseconds, sizeof(numMilliseconds)),
  };
  writeRecord(RoundTripMigrationTimeAnalysisTag, data_vec, num_data);
}

CREATE_TRACE_DEF(processOSegTrackedSetResults, mLogOSeg, const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, int numMilliseconds)
{
  if (mShuttingDown) return;

  const uint32 num_data = 4;
  BatchedBuffer::IOVec data_vec[num_data] = {
      BatchedBuffer::IOVec(&t, sizeof(t)),
      BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
      BatchedBuffer::IOVec(&sID_migratingTo, sizeof(sID_migratingTo)),
      BatchedBuffer::IOVec(&numMilliseconds, sizeof(numMilliseconds)),
  };
  writeRecord(OSegTrackedSetResultAnalysisTag, data_vec, num_data);
}

CREATE_TRACE_DEF(processOSegShutdownEvents, mLogOSeg, const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet)
{
  std::cout<<"\n\n**********oseg shutdown:  \n";
  std::cout<<"\tsid:                              "<<sID<<"\n";
  std::cout<<"\tnum lookups:                      "<<num_lookups<<"\n";
  std::cout<<"\tnum_on_this_server:               "<<num_on_this_server<<"\n";
  std::cout<<"\tnum_cache_hits:                   "<<num_cache_hits<<"\n";
  std::cout<<"\tnum_craq_lookups:                 "<<num_craq_lookups<<"\n";
  std::cout<<"\tnum_migration_not_complete_yet:   "<< num_migration_not_complete_yet<<"\n\n";
  std::cout<<"***************************\n\n";

  const uint32 num_data = 7;
  BatchedBuffer::IOVec data_vec[num_data] = {
      BatchedBuffer::IOVec(&t, sizeof(t)),
      BatchedBuffer::IOVec(&num_lookups, sizeof(num_lookups)),
      BatchedBuffer::IOVec(&num_on_this_server, sizeof(num_on_this_server)),
      BatchedBuffer::IOVec(&num_cache_hits, sizeof(num_cache_hits)),
      BatchedBuffer::IOVec(&num_craq_lookups, sizeof(num_craq_lookups)),
      BatchedBuffer::IOVec(&num_time_elapsed_cache_eviction, sizeof(num_time_elapsed_cache_eviction)),
      BatchedBuffer::IOVec(&num_migration_not_complete_yet, sizeof(num_migration_not_complete_yet)),
  };
  writeRecord(OSegShutdownEventTag, data_vec, num_data);
}




CREATE_TRACE_DEF(osegCacheResponse, mLogOSeg, const Time &t, const ServerID& sID, const UUID& obj_id)
{
  if (mShuttingDown) return;

  const uint32 num_data = 3;
  BatchedBuffer::IOVec data_vec[num_data] = {
      BatchedBuffer::IOVec(&t, sizeof(t)),
      BatchedBuffer::IOVec(&sID, sizeof(sID)),
      BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
  };
  writeRecord(OSegCacheResponseTag, data_vec, num_data);
}


CREATE_TRACE_DEF(objectSegmentationLookupNotOnServerRequest, mLogOSeg, const Time& t, const UUID& obj_id, const ServerID &sID_lookerupper)
{
  if (mShuttingDown) return;

  const uint32 num_data = 3;
  BatchedBuffer::IOVec data_vec[num_data] = {
      BatchedBuffer::IOVec(&t, sizeof(t)),
      BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
      BatchedBuffer::IOVec(&sID_lookerupper, sizeof(sID_lookerupper)),
  };
  writeRecord(OSegLookupNotOnServerAnalysisTag, data_vec, num_data);
}


CREATE_TRACE_DEF(osegCumulativeResponse, mLogOSeg, const Time &t, OSegLookupTraceToken* traceToken)
{
    if (traceToken == NULL || mShuttingDown)
        return;

    const uint32 num_data = 2;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&traceToken, sizeof(OSegLookupTraceToken)),
    };
    writeRecord(OSegCumulativeTraceAnalysisTag, data_vec, num_data);
}

} // namespace CBR
