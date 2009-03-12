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


std::ostream& LocationUpdate::write(std::ostream& os) {
    os << receiver.readableHexData() << " "
       << source.readableHexData() << " "
       << " ( " << location.time().raw() << " " << location.value().position().toString() << " " << location.value().velocity().toString() << " ) "
       << time.raw()
       << std::endl;
    return os;
}

std::istream& LocationUpdate::read(std::istream& is) {
    std::string receiver_human;
    std::string source_human;
    std::string garbage;
    uint64 loc_time_raw;
    Vector3f loc_pos, loc_vel;
    uint64 time_raw;

    is >> receiver_human
       >> source_human
       >> garbage >> loc_time_raw >> loc_pos >> loc_vel >> garbage
       >> time_raw;

    receiver = UUID(receiver_human);
    source = UUID(source_human);
    location = TimedMotionVector3f( Time(loc_time_raw), MotionVector3f(loc_pos, loc_vel) );
    time = Time(time_raw);

    return is;
}

LocationStatistics::~LocationStatistics() {
    for(std::vector<LocationUpdateBatch*>::iterator it = batches.begin(); it != batches.end(); it++) {
        LocationUpdateBatch* pb = *it;
        delete pb;
    }
    batches.clear();
}

void LocationStatistics::update(const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc, const Time& t) {
    if (batches.empty() || batches.back()->full())
        batches.push_back( new LocationUpdateBatch() );

    LocationUpdateBatch* pb = batches.back();
    pb->items[pb->size] = LocationUpdate(receiver, source, loc, t);
    pb->size++;
}

void LocationStatistics::save(const String& filename) {
    std::ofstream of(filename.c_str(), std::ios::out);

    //of << "Receiver Source Location Time" << std::endl;
    for(std::vector<LocationUpdateBatch*>::iterator it = batches.begin(); it != batches.end(); it++) {
        LocationUpdateBatch* pb = *it;
        pb->write(of);
    }
}



CompiledLocationStatistics::CompiledLocationStatistics(const char* opt_name, const uint32 nservers) {
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            LocationUpdate lu;
            try {
                lu.read(is);
            }
            catch(...) {
                break;
            }

            ObjectViewMap::iterator view_it = mViewMap.find(lu.receiver);
            if (view_it == mViewMap.end()) {
                mViewMap[lu.receiver] = new ObjectMotionSequenceMap;
                view_it = mViewMap.find(lu.receiver);
            }
            assert(view_it != mViewMap.end());
            ObjectMotionSequenceMap* update_sequence_map = view_it->second;

            ObjectMotionSequenceMap::iterator motion_it = update_sequence_map->find(lu.source);
            if (motion_it == update_sequence_map->end()) {
                (*update_sequence_map)[lu.source] = new MotionUpdateSequence;
                motion_it = update_sequence_map->find(lu.source);
            }
            assert(motion_it != update_sequence_map->end());
            MotionUpdateSequence* update_sequence = motion_it->second;

            update_sequence->updates.push_back( MotionUpdate( lu.time, lu.location ) );
        }
    }

    // sort each location update sequence by *time received*, run through and drop any updates which arrived after newer updates
    for(ObjectViewMap::iterator view_it = mViewMap.begin(); view_it != mViewMap.end(); view_it++) {
        ObjectMotionSequenceMap* sequence_map = view_it->second;
        for(ObjectMotionSequenceMap::iterator motion_it = sequence_map->begin(); motion_it != sequence_map->end(); motion_it++) {
            MotionUpdateSequence* sequence = motion_it->second;

            std::sort(sequence->updates.begin(), sequence->updates.end(), MotionUpdateTimeComparator());

            std::vector<MotionUpdate> out_of_date_culled;
            Time most_recent(0);
            for(std::vector<MotionUpdate>::iterator update_it = sequence->updates.begin(); update_it != sequence->updates.end(); update_it++) {
                if (most_recent < update_it->value().time()) {
                    out_of_date_culled.push_back(*update_it);
                    most_recent = update_it->value().time();
                }
            }
            sequence->updates.swap(out_of_date_culled);
        }
    }
}

CompiledLocationStatistics::~CompiledLocationStatistics() {
    for(ObjectViewMap::iterator view_it = mViewMap.begin(); view_it != mViewMap.end(); view_it++) {
        ObjectMotionSequenceMap* sequence_map = view_it->second;
        for(ObjectMotionSequenceMap::iterator motion_it = sequence_map->begin(); motion_it != sequence_map->end(); motion_it++) {
            MotionUpdateSequence* sequence = motion_it->second;
            delete sequence;
        }
        delete sequence_map;
    }
}


} // namespace CBR
