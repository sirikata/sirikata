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
#include <iostream>

#include "ServerNetworkImpl.hpp"

namespace CBR {

// write the specified number of bytes from the pointer to the buffer
void BatchedBuffer::write(const void* buf, uint32 nbytes) {
    const uint8* bufptr = (const uint8*)buf;
    while( nbytes > 0 ) {
        if (batches.empty() || batches.back()->full())
            batches.push_back( new ByteBatch() );

        ByteBatch* batch = batches.back();
        uint32 avail = batch->max_size - batch->size;
        uint32 to_copy = std::min(avail, nbytes);

        memcpy( &batch->items[batch->size], bufptr, to_copy);
        batch->size += to_copy;
        bufptr += to_copy;
        nbytes -= to_copy;
    }
}

// write the buffer to an ostream
void BatchedBuffer::write(std::ostream& os) {
    for(std::vector<ByteBatch*>::iterator it = batches.begin(); it != batches.end(); it++) {
        ByteBatch* bb = *it;
        os.write( (const char*)&(bb->items[0]) , bb->size );
    }
}


const uint8 Trace::ProximityTag;
const uint8 Trace::LocationTag;
const uint8 Trace::SubscriptionTag;
const uint8 Trace::ServerDatagramQueuedTag;
const uint8 Trace::ServerDatagramSentTag;
const uint8 Trace::ServerDatagramReceivedTag;


static uint32 GetMessageUniqueID(const Network::Chunk& msg) {
    uint32 offset = 0;
    offset += sizeof(ServerMessageHeader);
    offset += 1; // size of msg type

    uint32 uid;
    memcpy(&uid, &msg[offset], sizeof(uint32));
    return uid;
}



void Trace::prox(const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc) {
    data.write( &ProximityTag, sizeof(ProximityTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &entered, sizeof(entered) );
    data.write( &loc, sizeof(loc) );
}

void Trace::loc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
    data.write( &LocationTag, sizeof(LocationTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &loc, sizeof(loc) );
}

void Trace::subscription(const Time& t, const UUID& receiver, const UUID& source, bool start) {
    data.write( &SubscriptionTag, sizeof(SubscriptionTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &start, sizeof(start) );
}

void Trace::serverDatagramQueued(const Time& t, const ServerID& dest, uint32 id, uint32 size) {
    data.write( &ServerDatagramQueuedTag, sizeof(ServerDatagramQueuedTag) );
    data.write( &t, sizeof(t) );
    data.write( &dest, sizeof(dest) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, const ServerID& dest, const Network::Chunk& data) {
    uint32 id = GetMessageUniqueID(data);
    uint32 size = data.size();

    serverDatagramSent(start_time, end_time, dest, id, size);
}

void Trace::serverDatagramSent(const Time& start_time, const Time& end_time, const ServerID& dest, uint32 id, uint32 size) {
    data.write( &ServerDatagramSentTag, sizeof(ServerDatagramSentTag) );
    data.write( &start_time, sizeof(start_time) ); // using either start_time or end_time works since the ranges are guaranteed not to overlap
    data.write( &dest, sizeof(dest) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
    data.write( &start_time, sizeof(start_time) );
    data.write( &end_time, sizeof(end_time) );
}

void Trace::serverDatagramReceived(const Time& start_time, const Time& end_time, const ServerID& src, uint32 id, uint32 size) {
    data.write( &ServerDatagramReceivedTag, sizeof(ServerDatagramReceivedTag) );
    data.write( &start_time, sizeof(start_time) ); // using either start_time or end_time works since the ranges are guaranteed not to overlap
    data.write( &src, sizeof(src) );
    data.write( &id, sizeof(id) );
    data.write( &size, sizeof(size) );
    data.write( &start_time, sizeof(start_time) );
    data.write( &end_time, sizeof(end_time) );
}

void Trace::save(const String& filename) {
    std::ofstream of(filename.c_str(), std::ios::out | std::ios::binary);

    data.write(of);
}

} // namespace CBR
