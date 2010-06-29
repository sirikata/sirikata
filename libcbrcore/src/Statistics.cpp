/*  Sirikata
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

#include <sirikata/cbrcore/Statistics.hpp>
#include <sirikata/cbrcore/Message.hpp>
#include <sirikata/cbrcore/Options.hpp>
#include <sirikata/core/options/Options.hpp>

#include <iostream>

#include <boost/thread/locks.hpp>


namespace Sirikata {

BatchedBuffer::BatchedBuffer()
 : filling(NULL)
{
}

// write the specified number of bytes from the pointer to the buffer
void BatchedBuffer::write(const void* buf, uint32 nbytes)
{
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


namespace Trace {

const uint8 ProximityTag = 0;
const uint8 ObjectLocationTag = 1;
const uint8 ServerDatagramQueuedTag = 4;
const uint8 ServerDatagramSentTag = 5;
const uint8 ServerDatagramReceivedTag = 6;
const uint8 SegmentationChangeTag = 10;
const uint8 ObjectBeginMigrateTag = 11;
const uint8 ObjectAcknowledgeMigrateTag = 12;
const uint8 ServerLocationTag = 13;
const uint8 ServerObjectEventTag = 14;
const uint8 ObjectSegmentationCraqLookupRequestAnalysisTag = 15;
const uint8 ObjectSegmentationProcessedRequestAnalysisTag = 16;
const uint8 ObjectPingTag = 17;
const uint8 ObjectPingCreatedTag = 32;
const uint8 RoundTripMigrationTimeAnalysisTag = 18;
const uint8 OSegTrackedSetResultAnalysisTag   = 19;
const uint8 OSegShutdownEventTag              = 20;
const uint8 ObjectGeneratedLocationTag = 22;
const uint8 OSegCacheResponseTag = 23;
const uint8 OSegLookupNotOnServerAnalysisTag = 24;
const uint8 OSegCumulativeTraceAnalysisTag   = 25;
const uint8 OSegCraqProcessTag                 = 26;
const uint8 MessageTimestampTag = 30;
const uint8 MessageCreationTimestampTag = 31;

const uint8 ObjectConnectedTag = 33;

OptionValue* Trace::mLogLocProx;
OptionValue* Trace::mLogOSeg;
OptionValue* Trace::mLogOSegCumulative;
OptionValue* Trace::mLogCSeg;
OptionValue* Trace::mLogMigration;
OptionValue* Trace::mLogDatagram;
OptionValue* Trace::mLogPacket;
OptionValue* Trace::mLogPing;
OptionValue* Trace::mLogMessage;

#define TRACE_LOCPROX_NAME                  "trace-locprox"
#define TRACE_OSEG_NAME                     "trace-oseg"
#define TRACE_OSEG_CUMULATIVE_NAME          "trace-oseg-cumulative"
#define TRACE_CSEG_NAME                     "trace-cseg"
#define TRACE_MIGRATION_NAME                "trace-migration"
#define TRACE_DATAGRAM_NAME                 "trace-datagram"
#define TRACE_PACKET_NAME                   "trace-packet"
#define TRACE_PING_NAME                     "trace-ping"
#define TRACE_MESSAGE_NAME                  "trace-message"

void Trace::InitOptions() {
    mLogLocProx = new OptionValue(TRACE_LOCPROX_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogOSeg = new OptionValue(TRACE_OSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogOSegCumulative = new OptionValue(TRACE_OSEG_CUMULATIVE_NAME,"true",Sirikata::OptionValueType<bool>(),"Log oseg cumulative trace data");
    mLogCSeg = new OptionValue(TRACE_CSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogMigration = new OptionValue(TRACE_MIGRATION_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogDatagram = new OptionValue(TRACE_DATAGRAM_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogPacket = new OptionValue(TRACE_PACKET_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogPing = new OptionValue(TRACE_PING_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogMessage = new OptionValue(TRACE_MESSAGE_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");

    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(mLogLocProx)
        .addOption(mLogOSeg)
        .addOption(mLogOSegCumulative)
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

#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
        Sleep( Duration::seconds(1).toMilliseconds() );
#else
        usleep( Duration::seconds(1).toMicroseconds() );
#endif
    }
    data.store(of);
    fflush(of);

// FIXME #91
#if SIRIKATA_PLATFORM != SIRIKATA_WINDOWS
    fsync(fileno(of));
#endif
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


std::ostream&Drops::output(std::ostream&output) {
    for (int i=0;i<NUM_DROPS;++i) {
        if (d[i]&&n[i]) {
            output<<n[i]<<':'<<d[i]<<'\n';
        }
    }
    return output;
}

Trace::~Trace() {
    std::cout<<"Summary of drop data:\n";
    drops.output(std::cout)<<"EOF\n";
}


CREATE_TRACE_DEF(Trace, timestampMessageCreation, mLogMessage, const Time&sent, uint64 uid, MessagePath path, ObjectMessagePort srcprt, ObjectMessagePort dstprt) {
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

CREATE_TRACE_DEF(Trace, timestampMessage, mLogMessage, const Time&sent, uint64 uid, MessagePath path) {
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&sent, sizeof(sent)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
        BatchedBuffer::IOVec(&path, sizeof(path)),
    };
    writeRecord(MessageTimestampTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, pingCreated, mLogPing, const Time& src, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint32 sz) {
    if (mShuttingDown) return;

    const uint32 num_data = 7;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&src, sizeof(src)),
        BatchedBuffer::IOVec(&sender, sizeof(sender)),
        BatchedBuffer::IOVec(&dst, sizeof(dst)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&id, sizeof(id)),
        BatchedBuffer::IOVec(&distance, sizeof(distance)),
        BatchedBuffer::IOVec(&sz, sizeof(sz)),
    };
    writeRecord(ObjectPingCreatedTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, ping, mLogPing, const Time& src, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint64 uid, uint32 sz) {
    if (mShuttingDown) return;

    const uint32 num_data = 8;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&src, sizeof(src)),
        BatchedBuffer::IOVec(&sender, sizeof(sender)),
        BatchedBuffer::IOVec(&dst, sizeof(dst)),
        BatchedBuffer::IOVec(&receiver, sizeof(receiver)),
        BatchedBuffer::IOVec(&id, sizeof(id)),
        BatchedBuffer::IOVec(&distance, sizeof(distance)),
        BatchedBuffer::IOVec(&uid, sizeof(uid)),
        BatchedBuffer::IOVec(&sz, sizeof(sz)),
    };
    writeRecord(ObjectPingTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, serverLoc, mLogLocProx, const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc) {
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

CREATE_TRACE_DEF(Trace, serverObjectEvent, mLogLocProx, const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc) {
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

CREATE_TRACE_DEF(Trace, serverDatagramQueued, mLogDatagram, const Time& t, const ServerID& dest, uint64 id, uint32 size) {
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

CREATE_TRACE_DEF(Trace, serverDatagramSent, mLogDatagram, const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size) {
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

CREATE_TRACE_DEF(Trace, serverDatagramReceived, mLogDatagram, const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size) {
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

CREATE_TRACE_DEF(Trace, segmentationChanged, mLogCSeg, const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&bbox, sizeof(bbox)),
        BatchedBuffer::IOVec(&serverID, sizeof(serverID)),
    };
    writeRecord(SegmentationChangeTag, data_vec, num_data);
}

CREATE_TRACE_DEF(Trace, objectBeginMigrate, mLogMigration, const Time& t, const UUID& obj_id, const ServerID migrate_from, const ServerID migrate_to) {
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

CREATE_TRACE_DEF(Trace, objectAcknowledgeMigrate, mLogMigration, const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to) {
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


CREATE_TRACE_DEF(Trace, objectSegmentationCraqLookupRequest, mLogOSeg, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo){
    if (mShuttingDown) return;

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        BatchedBuffer::IOVec(&t, sizeof(t)),
        BatchedBuffer::IOVec(&obj_id, sizeof(obj_id)),
        BatchedBuffer::IOVec(&sID_lookupTo, sizeof(sID_lookupTo)),
    };
    writeRecord(ObjectSegmentationCraqLookupRequestAnalysisTag, data_vec, num_data);
  }

CREATE_TRACE_DEF(Trace, objectSegmentationProcessedRequest, mLogOSeg, const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 objectsInQueue)
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


CREATE_TRACE_DEF(Trace, objectMigrationRoundTrip, mLogMigration, const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, int numMilliseconds)
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

CREATE_TRACE_DEF(Trace, processOSegTrackedSetResults, mLogOSeg, const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, int numMilliseconds)
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

CREATE_TRACE_DEF(Trace, processOSegShutdownEvents, mLogOSeg, const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet)
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




CREATE_TRACE_DEF(Trace, osegCacheResponse, mLogOSeg, const Time &t, const ServerID& sID, const UUID& obj_id)
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


CREATE_TRACE_DEF(Trace, objectSegmentationLookupNotOnServerRequest, mLogOSeg, const Time& t, const UUID& obj_id, const ServerID &sID_lookerupper)
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


CREATE_TRACE_DEF(Trace, osegCumulativeResponse, mLogOSegCumulative, const Time &t, OSegLookupTraceToken* traceToken)
{
  if (traceToken == NULL || mShuttingDown)
    return;

  const uint32 num_data = 2;
  BatchedBuffer::IOVec data_vec[num_data] = {
    BatchedBuffer::IOVec(&t, sizeof(t)),
    BatchedBuffer::IOVec(traceToken, sizeof(OSegLookupTraceToken)),
  };

  writeRecord(OSegCumulativeTraceAnalysisTag, data_vec, num_data);
}


//   void Trace::osegProcessedCraqTime(const Time&t, const Duration& dur, uint32 numProc, uint32 sizeIncomingString)
//   {
//     if (mShuttingDown)
//       return;

//     #ifdef TRACE_OSEG_PROCESS_CRAQ
//     const uint32 num_data = 4;
//     BatchedBuffer::IOVec data_vec[num_data] =
//       {
//         BatchedBuffer::IOVec(&t, sizeof(t)),
//         BatchedBuffer::IOVec(&dur, sizeof(Duration)),
//         BatchedBuffer::IOVec(&numProc,sizeof(numProc)),
//         BatchedBuffer::IOVec(&sizeIncomingString,sizeof(sizeIncomingString)),
//       };
//     writeRecord(OSegCraqProcessTag, data_vec, num_data);
//     #endif
//   }

} // namespace Trace

} // namespace Sirikata
