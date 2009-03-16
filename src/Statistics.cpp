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

namespace CBR {

std::ostream& BandwidthStatistics::Packet::write(std::ostream& os) {
    os << server << " " << GetUniqueIDServerID(id) << " " << GetUniqueIDMessageID(id) << " " << size << " " << time.raw() << std::endl;
    return os;
}

BandwidthStatistics::~BandwidthStatistics() {
    for(std::vector<PacketBatch*>::iterator it = queuedBatches.begin(); it != queuedBatches.end(); it++) {
        PacketBatch* pb = *it;
        delete pb;
    }
    queuedBatches.clear();

    for(std::vector<PacketBatch*>::iterator it = sentBatches.begin(); it != sentBatches.end(); it++) {
        PacketBatch* pb = *it;
        delete pb;
    }
    sentBatches.clear();

    for(std::vector<PacketBatch*>::iterator it = receivedBatches.begin(); it != receivedBatches.end(); it++) {
        PacketBatch* pb = *it;
        delete pb;
    }
    receivedBatches.clear();
}

void BandwidthStatistics::queued(const ServerID& dest, uint32 id, uint32 size, const Time& t) {
    if (queuedBatches.empty() || queuedBatches.back()->full())
        queuedBatches.push_back( new PacketBatch() );

    PacketBatch* pb = queuedBatches.back();
    pb->items[pb->size] = Packet(dest, id, size, t);
    pb->size++;
}

void BandwidthStatistics::sent(const ServerID& dest, uint32 id, uint32 size, const Time& t) {
    if (sentBatches.empty() || sentBatches.back()->full())
        sentBatches.push_back( new PacketBatch() );

    PacketBatch* pb = sentBatches.back();
    pb->items[pb->size] = Packet(dest, id, size, t);
    pb->size++;
}

void BandwidthStatistics::received(const ServerID& src, uint32 id, uint32 size, const Time& t) {
    if (receivedBatches.empty() || receivedBatches.back()->full())
        receivedBatches.push_back( new PacketBatch() );

    PacketBatch* pb = receivedBatches.back();
    pb->items[pb->size] = Packet(src, id, size, t);
    pb->size++;
}

void BandwidthStatistics::save(const String& filename) {
    std::ofstream of(filename.c_str(), std::ios::out);

    of << "Queued" << std::endl;
    of << "DestServer ServerID MessageID Size Time" << std::endl;
    for(std::vector<PacketBatch*>::iterator it = queuedBatches.begin(); it != queuedBatches.end(); it++) {
        PacketBatch* pb = *it;
        pb->write(of);
    }

    of << "Sent" << std::endl;
    of << "DestServer ServerID MessageID Size Time" << std::endl;
    for(std::vector<PacketBatch*>::iterator it = sentBatches.begin(); it != sentBatches.end(); it++) {
        PacketBatch* pb = *it;
        pb->write(of);
    }

    of << "Received" << std::endl;
    of << "SourceServer ServerID MessageID Size Time" << std::endl;
    for(std::vector<PacketBatch*>::iterator it = receivedBatches.begin(); it != receivedBatches.end(); it++) {
        PacketBatch* pb = *it;
        pb->write(of);
    }
}


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


const char ObjectTrace::ProximityTag = 'p';
const char ObjectTrace::LocationTag = 'l';
const char ObjectTrace::SubscriptionTag = 's';

void ObjectTrace::prox(const Time& t, const UUID& receiver, const UUID& source, bool entered) {
    data.write( &ProximityTag, sizeof(ProximityTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &entered, sizeof(entered) );
}

void ObjectTrace::loc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
    data.write( &LocationTag, sizeof(LocationTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &loc, sizeof(loc) );
}

void ObjectTrace::subscription(const Time& t, const UUID& receiver, const UUID& source, bool start) {
    data.write( &SubscriptionTag, sizeof(SubscriptionTag) );
    data.write( &t, sizeof(t) );
    data.write( &receiver, sizeof(receiver) );
    data.write( &source, sizeof(source) );
    data.write( &start, sizeof(start) );
}

void ObjectTrace::save(const String& filename) {
    std::ofstream of(filename.c_str(), std::ios::out | std::ios::binary);

    data.write(of);
}

} // namespace CBR
