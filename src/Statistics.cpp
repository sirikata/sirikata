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


//#define TRACE_OBJECT
//#define TRACE_LOCPROX
#define TRACE_OSEG
//#define TRACE_CSEG
//#define TRACE_OSEG_CUMULATIVE

#define TRACE_MIGRATION
//#define TRACE_DATAGRAM
//#define TRACE_PACKET
#define TRACE_PING
#define TRACE_MESSAGE
#define TRACE_ROUND_TRIP_MIGRATION_TIME
#define TRACE_OSEG_TRACKED_SET_RESULTS
#define TRACE_OSEG_SHUTTING_DOWN
#define TRACE_OSEG_CACHE_RESPONSE
#define TRACE_OSEG_NOT_ON_SERVER_LOOKUP


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

void Trace::prox(const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc) {
#ifdef TRACE_OBJECT
    if (mShuttingDown) return;

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ProximityTag, sizeof(ProximityTag) },
        { &t, sizeof(t) },
        { &receiver, sizeof(receiver) },
        { &source, sizeof(source) },
        { &entered, sizeof(entered) },
        { &loc, sizeof(loc) }
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::objectGenLoc(const Time& t, const UUID& source, const TimedMotionVector3f& loc) {
#ifdef TRACE_OBJECT
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectGeneratedLocationTag, sizeof(ObjectGeneratedLocationTag) },
        { &t, sizeof(t) },
        { &source, sizeof(source) },
        { &loc, sizeof(loc) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::objectLoc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
#ifdef TRACE_OBJECT
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectLocationTag, sizeof(ObjectLocationTag) },
        { &t, sizeof(t) },
        { &receiver, sizeof(receiver) },
        { &source, sizeof(source) },
        { &loc, sizeof(loc) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::timestampMessageCreation(const Time&sent, uint64 uid, MessagePath path, ObjectMessagePort srcprt, ObjectMessagePort dstprt) {
#ifdef TRACE_MESSAGE
    if (mShuttingDown) return;

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &MessageCreationTimestampTag, sizeof(MessageCreationTimestampTag) },
        { &sent, sizeof(sent) },
        { &uid, sizeof(uid) },
        { &path, sizeof(path) },
        { &srcprt, sizeof(srcprt) },
        { &dstprt, sizeof(dstprt) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::timestampMessage(const Time&sent, uint64 uid, MessagePath path) {
#ifdef TRACE_MESSAGE
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &MessageTimestampTag, sizeof(MessageTimestampTag) },
        { &sent, sizeof(sent) },
        { &uid, sizeof(uid) },
        { &path, sizeof(path) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::ping(const Time& src, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint64 uid) {
#ifdef TRACE_PING
    if (mShuttingDown) return;

    const uint32 num_data = 8;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectPingTag, sizeof(ObjectPingTag) },
        { &src, sizeof(src) },
        { &sender, sizeof(sender) },
        { &dst, sizeof(dst) },
        { &receiver, sizeof(receiver) },
        { &id, sizeof(id) },
        { &distance, sizeof(distance) },
        { &uid, sizeof(uid) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::serverLoc(const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc) {
#ifdef TRACE_LOCPROX
    if (mShuttingDown) return;

    const uint32 num_data = 6;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ServerLocationTag, sizeof(ServerLocationTag) },
        { &t, sizeof(t) },
        { &sender, sizeof(sender) },
        { &receiver, sizeof(receiver) },
        { &obj, sizeof(obj) },
        { &loc, sizeof(loc) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::serverObjectEvent(const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc) {
#ifdef TRACE_LOCPROX
    if (mShuttingDown) return;

    uint8 raw_added = (added ? 1 : 0);

    const uint32 num_data = 7;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ServerObjectEventTag, sizeof(ServerObjectEventTag) },
        { &t, sizeof(t) },
        { &source, sizeof(source) },
        { &dest, sizeof(dest) },
        { &obj, sizeof(obj) },
        { &raw_added, sizeof(raw_added) },
        { &loc, sizeof(loc) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::serverDatagramQueued(const Time& t, const ServerID& dest, uint64 id, uint32 size) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ServerDatagramQueuedTag, sizeof(ServerDatagramQueuedTag) },
        { &t, sizeof(t) },
        { &dest, sizeof(dest) },
        { &id, sizeof(id) },
        { &size, sizeof(size) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;

    const uint32 num_data = 8;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ServerDatagramSentTag, sizeof(ServerDatagramSentTag) },
        { &start_time, sizeof(start_time) },
        { &dest, sizeof(dest) },
        { &id, sizeof(id) },
        { &size, sizeof(size) },
        { &weight, sizeof(weight) },
        { &start_time, sizeof(start_time) },
        { &end_time, sizeof(end_time) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::serverDatagramReceived(const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;

    const uint32 num_data = 7;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ServerDatagramReceivedTag, sizeof(ServerDatagramReceivedTag) },
        { &start_time, sizeof(start_time) },
        { &src, sizeof(src) },
        { &id, sizeof(id) },
        { &size, sizeof(size) },
        { &start_time, sizeof(start_time) },
        { &end_time, sizeof(end_time) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::packetQueueInfo(const Time& t, const ServerID& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight) {
#ifdef TRACE_PACKET
    if (mShuttingDown) return;

    const uint32 num_data = 9;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &PacketQueueInfoTag, sizeof(PacketQueueInfoTag) },
        { &t, sizeof(t) },
        { &dest, sizeof(dest) },
        { &send_size, sizeof(send_size) },
        { &send_queued, sizeof(send_queued) },
        { &send_weight, sizeof(send_weight) },
        { &receive_size, sizeof(receive_size) },
        { &receive_queued, sizeof(receive_queued) },
        { &receive_weight, sizeof(receive_weight) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::packetSent(const Time& t, const ServerID& dest, uint32 size) {
#ifdef TRACE_PACKET
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &PacketSentTag, sizeof(PacketSentTag) },
        { &t, sizeof(t) },
        { &dest, sizeof(dest) },
        { &size, sizeof(size) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::packetReceived(const Time& t, const ServerID& src, uint32 size) {
#ifdef TRACE_PACKET
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &PacketReceivedTag, sizeof(PacketReceivedTag) },
        { &t, sizeof(t) },
        { &src, sizeof(src) },
        { &size, sizeof(size) },
    };
    data.write(data_vec, num_data);
#endif
}

void Trace::segmentationChanged(const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
#ifdef TRACE_CSEG
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &SegmentationChangeTag, sizeof(SegmentationChangeTag) },
        { &t, sizeof(t) },
        { &bbox, sizeof(bbox) },
        { &serverID, sizeof(serverID) },
    };
    data.write(data_vec, num_data);
#endif
}

  void Trace::objectBeginMigrate(const Time& t, const UUID& obj_id, const ServerID migrate_from, const ServerID migrate_to)
  {
#ifdef TRACE_MIGRATION
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectBeginMigrateTag, sizeof(ObjectBeginMigrateTag) },
        { &t, sizeof(t) },
        { &obj_id, sizeof(obj_id) },
        { &migrate_from, sizeof(migrate_from) },
        { &migrate_to, sizeof(migrate_to) },
    };
    data.write(data_vec, num_data);
#endif
  }

  void Trace::objectAcknowledgeMigrate(const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to)
  {
#ifdef TRACE_MIGRATION
    if (mShuttingDown) return;

    const uint32 num_data = 5;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectAcknowledgeMigrateTag, sizeof(ObjectAcknowledgeMigrateTag) },
        { &t, sizeof(t) },
        { &obj_id, sizeof(obj_id) },
        { &acknowledge_from, sizeof(acknowledge_from) },
        { &acknowledge_to, sizeof(acknowledge_to) },
    };
    data.write(data_vec, num_data);
#endif
  }


  void Trace::objectSegmentationCraqLookupRequest(const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo)
  {
#ifdef TRACE_OSEG
    if (mShuttingDown) return;

    const uint32 num_data = 4;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectSegmentationCraqLookupRequestAnalysisTag, sizeof(ObjectSegmentationCraqLookupRequestAnalysisTag) },
        { &t, sizeof(t) },
        { &obj_id, sizeof(obj_id) },
        { &sID_lookupTo, sizeof(sID_lookupTo) },
    };
    data.write(data_vec, num_data);
#endif
  }

  void Trace::objectSegmentationProcessedRequest(const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 objectsInQueue)
  {
#ifdef TRACE_OSEG
    if (mShuttingDown) return;

    const uint32 num_data = 7;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &ObjectSegmentationProcessedRequestAnalysisTag, sizeof(ObjectSegmentationProcessedRequestAnalysisTag) },
        { &t, sizeof(t) },
        { &obj_id, sizeof(obj_id) },
        { &sID_processor, sizeof(sID_processor) },
        { &sID, sizeof(sID) },
        { &dTime, sizeof(dTime) },
        { &objectsInQueue, sizeof(objectsInQueue) },
    };
    data.write(data_vec, num_data);
#endif
  }


void Trace::objectMigrationRoundTrip(const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, int numMilliseconds)
{
#ifdef TRACE_ROUND_TRIP_MIGRATION_TIME
  if (mShuttingDown) return;

  const uint32 num_data = 6;
  BatchedBuffer::IOVec data_vec[num_data] = {
      { &RoundTripMigrationTimeAnalysisTag, sizeof(RoundTripMigrationTimeAnalysisTag) },
      { &t, sizeof(t) },
      { &obj_id, sizeof(obj_id) },
      { &sID_migratingFrom, sizeof(sID_migratingFrom) },
      { &sID_migratingTo, sizeof(sID_migratingTo) },
      { &numMilliseconds, sizeof(numMilliseconds) },
  };
  data.write(data_vec, num_data);
#endif
}

void Trace::processOSegTrackedSetResults(const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, int numMilliseconds)
{
#ifdef TRACE_OSEG_TRACKED_SET_RESULTS
  if (mShuttingDown) return;

  const uint32 num_data = 5;
  BatchedBuffer::IOVec data_vec[num_data] = {
      { &OSegTrackedSetResultAnalysisTag, sizeof(OSegTrackedSetResultAnalysisTag) },
      { &t, sizeof(t) },
      { &obj_id, sizeof(obj_id) },
      { &sID_migratingTo, sizeof(sID_migratingTo) },
      { &numMilliseconds, sizeof(numMilliseconds) },
  };
  data.write(data_vec, num_data);
#endif
}

void Trace::processOSegShutdownEvents(const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet)
{
#ifdef TRACE_OSEG_SHUTTING_DOWN
  std::cout<<"\n\n**********oseg shutdown:  \n";
  std::cout<<"\tsid:                              "<<sID<<"\n";
  std::cout<<"\tnum lookups:                      "<<num_lookups<<"\n";
  std::cout<<"\tnum_on_this_server:               "<<num_on_this_server<<"\n";
  std::cout<<"\tnum_cache_hits:                   "<<num_cache_hits<<"\n";
  std::cout<<"\tnum_craq_lookups:                 "<<num_craq_lookups<<"\n";
  std::cout<<"\tnum_migration_not_complete_yet:   "<< num_migration_not_complete_yet<<"\n\n";
  std::cout<<"***************************\n\n";

  const uint32 num_data = 8;
  BatchedBuffer::IOVec data_vec[num_data] = {
      { &OSegShutdownEventTag, sizeof(OSegShutdownEventTag) },
      { &t, sizeof(t) },
      { &num_lookups, sizeof(num_lookups) },
      { &num_on_this_server, sizeof(num_on_this_server) },
      { &num_cache_hits, sizeof(num_cache_hits) },
      { &num_craq_lookups, sizeof(num_craq_lookups) },
      { &num_time_elapsed_cache_eviction, sizeof(num_time_elapsed_cache_eviction) },
      { &num_migration_not_complete_yet, sizeof(num_migration_not_complete_yet) },
  };
  data.write(data_vec, num_data);
#endif
}




void Trace::osegCacheResponse(const Time &t, const ServerID& sID, const UUID& obj_id)
{
#ifdef TRACE_OSEG_CACHE_RESPONSE
  if (mShuttingDown) return;

  const uint32 num_data = 4;
  BatchedBuffer::IOVec data_vec[num_data] = {
      { &OSegCacheResponseTag, sizeof(OSegCacheResponseTag) },
      { &t, sizeof(t) },
      { &sID, sizeof(sID) },
      { &obj_id, sizeof(obj_id) },
  };
  data.write(data_vec, num_data);
#endif
}


void Trace::objectSegmentationLookupNotOnServerRequest(const Time& t, const UUID& obj_id, const ServerID &sID_lookerupper)
{
#ifdef TRACE_OSEG_NOT_ON_SERVER_LOOKUP
  if (mShuttingDown) return;

  const uint32 num_data = 4;
  BatchedBuffer::IOVec data_vec[num_data] = {
      { &OSegLookupNotOnServerAnalysisTag, sizeof(OSegLookupNotOnServerAnalysisTag) },
      { &t, sizeof(t) },
      { &obj_id, sizeof(obj_id) },
      { &sID_lookerupper, sizeof(sID_lookerupper) },
  };
  data.write(data_vec, num_data);
#endif
}


  void Trace::osegCumulativeResponse(const Time &t, OSegLookupTraceToken* traceToken)
  {
    if (traceToken == NULL)
      return;

    if (mShuttingDown)
    {
      delete traceToken;
      return;
    }

    #ifdef TRACE_OSEG_CUMULATIVE

    const uint32 num_data = 3;
    BatchedBuffer::IOVec data_vec[num_data] = {
        { &OSegCumulativeTraceAnalysisTag, sizeof(OSegCumulativeTraceAnalysisTag) },
        { &t, sizeof(t) },
        { &traceToken, sizeof(OSegLookupTraceToken) },
    };
    data.write(data_vec, num_data);
    #endif

    delete traceToken;
  }

} // namespace CBR
