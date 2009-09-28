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

#include "ServerNetworkImpl.hpp"

#include "ServerIDMap.hpp"


//#define TRACE_OBJECT
//#define TRACE_LOCPROX
#define TRACE_OSEG
//#define TRACE_CSEG

#define TRACE_MIGRATION
//#define TRACE_DATAGRAM
//#define TRACE_PACKET
//#define TRACE_PING
//#define TRACE_MESSAGE
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

void BatchedBuffer::flush() {
    batches.push(filling);
    filling = NULL;
}

// write the buffer to an ostream
void BatchedBuffer::store(FILE* os) {
    std::deque<ByteBatch*> bufs;
    batches.swap(bufs);

    for(std::deque<ByteBatch*>::iterator it = bufs.begin(); it != bufs.end(); it++) {
        ByteBatch* bb = *it;
        fwrite((void*)&(bb->items[0]), 1, bb->size, os);
        delete bb;
    }
}
const uint8 Trace::MessageTimestampTag;
const uint8 Trace::ObjectPingTag;
const uint8 Trace::ProximityTag;
const uint8 Trace::ObjectLocationTag;
const uint8 Trace::SubscriptionTag;
const uint8 Trace::ServerDatagramQueueInfoTag;
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


  
static uint64 GetMessageUniqueID(const Network::Chunk& msg) {
    uint64 offset = 0;
    offset += sizeof(ServerMessageHeader);
    offset += 1; // size of msg type

    uint64 uid;
    memcpy(&uid, &msg[offset], sizeof(uint64));
    return uid;
}


Trace::Trace(const String& filename)
 : mServerIDMap(NULL),
   mShuttingDown(false),
   mStorageThread(NULL),
   mFinishStorage(false)
{
    mStorageThread = new boost::thread( std::tr1::bind(&Trace::storageThread, this, filename) );
}

void Trace::setServerIDMap(ServerIDMap* sidmap) {
    mServerIDMap = sidmap;
}

void Trace::prepareShutdown() {
    mShuttingDown = true;
}

void Trace::shutdown() {
    mFinishStorage = true;
    mStorageThread->join();
    data.flush();
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

    data.write( &ProximityTag, sizeof(ProximityTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &entered, sizeof(entered) );
    data.write( &loc, sizeof(loc) );
#endif
}

void Trace::objectGenLoc(const Time& t, const UUID& source, const TimedMotionVector3f& loc) {
#ifdef TRACE_OBJECT
    if (mShuttingDown) return;

    data.write( &ObjectGeneratedLocationTag, sizeof(ObjectGeneratedLocationTag) );
    data.write( &t, sizeof(t) );
    data.write( &source, sizeof(source) );
    data.write( &loc, sizeof(loc) );
#endif
}

void Trace::objectLoc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
#ifdef TRACE_OBJECT
    if (mShuttingDown) return;

    data.write( &ObjectLocationTag, sizeof(ObjectLocationTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &loc, sizeof(loc) );
#endif
}

void Trace::subscription(const Time& t, const UUID& receiver, const UUID& source, bool start) {
#ifdef TRACE_OBJECT
    if (mShuttingDown) return;

    data.write( &SubscriptionTag, sizeof(SubscriptionTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &start, sizeof(start) );
#endif
}
bool Trace::timestampMessage(const Time&t, MessagePath path,const Network::Chunk&data){
#ifdef TRACE_MESSAGE
    uint32 offset=0;
    Message*result=NULL;
    offset = Message::deserialize(data,offset,&result);
    ObjectMessage *om=dynamic_cast<ObjectMessage*>(result);
    if (om) {
        timestampMessage(t,om->contents.unique(),path,om->contents.source_port(),om->contents.dest_port(),result->type());
    }else return false;
#endif
    return true;
}
void Trace::timestampMessage(const Time&sent, uint64 uid, MessagePath path, unsigned short srcprt, unsigned short dstprt, unsigned char msg_type) {
#ifdef TRACE_MESSAGE
    if (mShuttingDown) return;

    data.write( &MessageTimestampTag, sizeof(MessageTimestampTag) );
    data.write( &sent, sizeof(sent) );
    data.write( &uid, sizeof(uid) );
    data.write( &path, sizeof(path) );
    data.write( &srcprt, sizeof(srcprt) );
    data.write( &dstprt, sizeof(dstprt) );
    data.write( &msg_type, sizeof(msg_type) );
#endif
}

void Trace::ping(const Time& src, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint64 uid) {
#ifdef TRACE_PING
    if (mShuttingDown) return;

    data.write( &ObjectPingTag, sizeof(ObjectPingTag) );
    data.write( &src, sizeof(src) );
    data.write( &sender, sizeof(sender) );
    data.write( &dst, sizeof(dst) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &id, sizeof(id) );
    data.write( &distance, sizeof(distance) );
    data.write( &uid, sizeof(uid) );
#endif
}

void Trace::serverLoc(const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc) {
#ifdef TRACE_LOCPROX
    if (mShuttingDown) return;

    data.write( &ServerLocationTag, sizeof(ServerLocationTag) );
    data.write( &t, sizeof(t) );
    data.write( &sender, sizeof(sender) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &obj, sizeof(obj) );
    data.write( &loc, sizeof(loc) );
#endif
}

void Trace::serverObjectEvent(const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc) {
#ifdef TRACE_LOCPROX
    if (mShuttingDown) return;

    data.write( &ServerObjectEventTag, sizeof(ServerObjectEventTag) );
    data.write( &t, sizeof(t) );
    data.write( &source, sizeof(source) );
    data.write( &dest, sizeof(dest) );
    data.write( &obj, sizeof(obj) );
    uint8 raw_added = (added ? 1 : 0);
    data.write( &raw_added, sizeof(raw_added) );
    data.write( &loc, sizeof(loc) );
#endif
}

void Trace::serverDatagramQueueInfo(const Time& t, const ServerID& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;
    data.write( &ServerDatagramQueueInfoTag, sizeof(ServerDatagramQueueInfoTag) );
    data.write( &t, sizeof(t) );
    data.write( &dest, sizeof(dest) );
    data.write( &send_size, sizeof(send_size) );
    data.write( &send_queued, sizeof(send_queued) );
    data.write( &send_weight, sizeof(send_weight) );
    data.write( &receive_size, sizeof(receive_size) );
    data.write( &receive_queued, sizeof(receive_queued) );
    data.write( &receive_weight, sizeof(receive_weight) );
#endif
}

void Trace::serverDatagramQueued(const Time& t, const ServerID& dest, uint64 id, uint32 size) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;
    data.write( &ServerDatagramQueuedTag, sizeof(ServerDatagramQueuedTag) );
    data.write( &t, sizeof(t) );
    data.write( &dest, sizeof(dest) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
#endif
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, float weight, const ServerID& dest, const Network::Chunk& data) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;
    uint64 id = GetMessageUniqueID(data);
    uint32 size = data.size();

    serverDatagramSent(start_time, end_time, weight, dest, id, size);
#endif
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;
    data.write( &ServerDatagramSentTag, sizeof(ServerDatagramSentTag) );
    data.write( &start_time, sizeof(start_time) ); // using either start_time or end_time works since the ranges are guaranteed not to overlap
    data.write( &dest, sizeof(dest) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
    data.write( &weight, sizeof(weight));
    data.write( &start_time, sizeof(start_time) );
    data.write( &end_time, sizeof(end_time) );
#endif
}

void Trace::serverDatagramReceived(const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size) {
#ifdef TRACE_DATAGRAM
    if (mShuttingDown) return;
    data.write( &ServerDatagramReceivedTag, sizeof(ServerDatagramReceivedTag) );
    data.write( &start_time, sizeof(start_time) ); // using either start_time or end_time works since the ranges are guaranteed not to overlap
    data.write( &src, sizeof(src) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
    data.write( &start_time, sizeof(start_time) );
    data.write( &end_time, sizeof(end_time) );
#endif
}

void Trace::packetQueueInfo(const Time& t, const Address4& dest, uint32 send_size, uint32 send_queued, float send_weight, uint32 receive_size, uint32 receive_queued, float receive_weight) {
#ifdef TRACE_PACKET
    if (mShuttingDown) return;
    data.write( &PacketQueueInfoTag, sizeof(PacketQueueInfoTag) );
    data.write( &t, sizeof(t) );
    ServerID* dest_server_id = mServerIDMap->lookupInternal(dest);
    assert(dest_server_id);
    data.write( dest_server_id, sizeof(ServerID) );
    data.write( &send_size, sizeof(send_size) );
    data.write( &send_queued, sizeof(send_queued) );
    data.write( &send_weight, sizeof(send_weight) );
    data.write( &receive_size, sizeof(receive_size) );
    data.write( &receive_queued, sizeof(receive_queued) );
    data.write( &receive_weight, sizeof(receive_weight) );
#endif
}

void Trace::packetSent(const Time& t, const Address4& dest, uint32 size) {
#ifdef TRACE_PACKET
    if (mShuttingDown) return;
    data.write( &PacketSentTag, sizeof(PacketSentTag) );
    data.write( &t, sizeof(t) );
    assert(mServerIDMap);
    ServerID* dest_server_id = mServerIDMap->lookupInternal(dest);
    assert(dest_server_id);
    data.write( dest_server_id, sizeof(ServerID) );
    data.write( &size, sizeof(size) );
#endif
}

void Trace::packetReceived(const Time& t, const Address4& src, uint32 size) {
#ifdef TRACE_PACKET
    if (mShuttingDown) return;
    data.write( &PacketReceivedTag, sizeof(PacketReceivedTag) );
    data.write( &t, sizeof(t) );
    assert(mServerIDMap);
    ServerID* src_server_id = mServerIDMap->lookupInternal(src);
    assert(src_server_id);
    data.write( src_server_id, sizeof(ServerID) );
    data.write( &size, sizeof(size) );
#endif
}

void Trace::segmentationChanged(const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
#ifdef TRACE_CSEG
    if (mShuttingDown) return;
    data.write( &SegmentationChangeTag, sizeof(SegmentationChangeTag) );
    data.write( &t, sizeof(t) );
    data.write( &bbox, sizeof(bbox) );
    data.write( &serverID, sizeof(serverID) );
#endif
}

  void Trace::objectBeginMigrate(const Time& t, const UUID& obj_id, const ServerID migrate_from, const ServerID migrate_to)
  {
#ifdef TRACE_MIGRATION
    if (mShuttingDown) return;

    data.write(&ObjectBeginMigrateTag, sizeof(ObjectBeginMigrateTag));
    data.write(&t, sizeof(t));

    //    printf("\n\n******In Statistics.cpp.  Have an object begin migrate message. \n\n");

    data.write(&obj_id, sizeof(obj_id));
    data.write(&migrate_from, sizeof(migrate_from));
    data.write(&migrate_to,sizeof(migrate_to));
#endif
  }

  void Trace::objectAcknowledgeMigrate(const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to)
  //  void Trace::objectAcknowledgeMigrate(const Time& t, const UUID& obj_id, const ServerID acknowledge_from, acknowledge_to)
  {

#ifdef TRACE_MIGRATION
    if (mShuttingDown) return;

    //    printf("\n\n******In Statistics.cpp.  Have an object ack migrate message. \n\n");
    data.write(&ObjectAcknowledgeMigrateTag, sizeof(ObjectAcknowledgeMigrateTag));
    data.write(&t, sizeof(t));
    data.write(&obj_id, sizeof(obj_id));
    data.write(&acknowledge_from, sizeof(ServerID));
    data.write(&acknowledge_to,sizeof(ServerID));
#endif
  }


  void Trace::objectSegmentationCraqLookupRequest(const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo)
  {
#ifdef TRACE_OSEG

    if (mShuttingDown) return;


    data.write(&ObjectSegmentationCraqLookupRequestAnalysisTag, sizeof(ObjectSegmentationCraqLookupRequestAnalysisTag));
    data.write(&t, sizeof(t));
    data.write(&obj_id, sizeof(obj_id));
    data.write(&sID_lookupTo, sizeof(sID_lookupTo));

#endif

  }
  void Trace::objectSegmentationProcessedRequest(const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 objectsInQueue)
  {
#ifdef TRACE_OSEG
    if (mShuttingDown) return;

    data.write(&ObjectSegmentationProcessedRequestAnalysisTag, sizeof(ObjectSegmentationProcessedRequestAnalysisTag));
    data.write(&t, sizeof(t));
    data.write(&obj_id, sizeof(obj_id));
    data.write(&sID_processor, sizeof(sID_processor));
    data.write(&sID, sizeof(sID));
    data.write(&dTime, sizeof(dTime));
    data.write(&objectsInQueue,sizeof(objectsInQueue));
#endif
  }




void Trace::objectMigrationRoundTrip(const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, int numMilliseconds)
{
#ifdef TRACE_ROUND_TRIP_MIGRATION_TIME
  if (mShuttingDown) return;

  data.write(&RoundTripMigrationTimeAnalysisTag, sizeof(RoundTripMigrationTimeAnalysisTag));
  data.write(&t, sizeof(t));
  data.write(&obj_id, sizeof(obj_id));
  data.write(&sID_migratingFrom, sizeof(sID_migratingFrom));
  data.write(&sID_migratingTo, sizeof(sID_migratingTo));
  data.write(&numMilliseconds, sizeof(numMilliseconds));

#endif
}

void Trace::processOSegTrackedSetResults(const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, int numMilliseconds)
{
#ifdef TRACE_OSEG_TRACKED_SET_RESULTS

  if (mShuttingDown) return;

  data.write(&OSegTrackedSetResultAnalysisTag, sizeof(OSegTrackedSetResultAnalysisTag));
  data.write(&t, sizeof(t));
  data.write(&obj_id,sizeof(obj_id));
  data.write(&sID_migratingTo, sizeof(sID_migratingTo));
  data.write(&numMilliseconds, sizeof(numMilliseconds));

#endif
}

void Trace::processOSegShutdownEvents(const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet)
{
#ifdef TRACE_OSEG_SHUTTING_DOWN

  std::cout<<"\n\n\nGOT INTO PROCESS  OSEG SHUTDOWN EVENT\n\n";

  std::cout<<"\n\n**********oseg shutdown:  \n";
  std::cout<<"\tsid:                              "<<sID<<"\n";
  std::cout<<"\tnum lookups:                      "<<num_lookups<<"\n";
  std::cout<<"\tnum_on_this_server:               "<<num_on_this_server<<"\n";
  std::cout<<"\tnum_cache_hits:                   "<<num_cache_hits<<"\n";
  std::cout<<"\tnum_craq_lookups:                 "<<num_craq_lookups<<"\n";
  std::cout<<"\tnum_migration_not_complete_yet:   "<< num_migration_not_complete_yet<<"\n\n";
  std::cout<<"***************************\n\n";


  data.write(&OSegShutdownEventTag, sizeof(OSegShutdownEventTag));
  data.write(&t,sizeof(t));
  data.write(&num_lookups,sizeof(num_lookups));
  data.write(&num_on_this_server,sizeof(num_on_this_server));
  data.write(&num_cache_hits,sizeof(num_cache_hits));
  data.write(&num_craq_lookups,sizeof(num_craq_lookups));
  data.write(&num_time_elapsed_cache_eviction, sizeof(num_time_elapsed_cache_eviction));
  data.write(&num_migration_not_complete_yet, sizeof(num_migration_not_complete_yet));

#endif
}


void Trace::osegCacheResponse(const Time &t, const ServerID& sID, const UUID& obj_id)
{
#ifdef TRACE_OSEG_CACHE_RESPONSE
  if (mShuttingDown) return;

  data.write(&OSegCacheResponseTag, sizeof(OSegCacheResponseTag));
  data.write(&t, sizeof(t));
  data.write(&sID, sizeof(sID));
  data.write(&obj_id,sizeof(obj_id));
  
#endif
}


void Trace::objectSegmentationLookupNotOnServerRequest(const Time& t, const UUID& obj_id, const ServerID &sID_lookerupper)
{
#ifdef TRACE_OSEG_NOT_ON_SERVER_LOOKUP
  

  if (mShuttingDown) return;

  
  data.write(&OSegLookupNotOnServerAnalysisTag, sizeof (OSegLookupNotOnServerAnalysisTag));
  data.write(&t, sizeof(t));
  data.write(&obj_id, sizeof(obj_id));
  data.write(&sID_lookerupper, sizeof(sID_lookerupper));
  
#endif
}


} // namespace CBR
