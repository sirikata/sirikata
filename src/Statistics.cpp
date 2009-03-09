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
#include <iostream>

namespace CBR {

BandwidthStatistics::~BandwidthStatistics() {
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

void BandwidthStatistics::sent(const ServerID& dest, uint32 id, uint32 size, const Time& t) {
    if (sentBatches.empty() || sentBatches.back()->full())
        sentBatches.push_back( new PacketBatch() );

    PacketBatch* pb = sentBatches.back();
    pb->packets[pb->size].server = dest;
    pb->packets[pb->size].id = id;
    pb->packets[pb->size].size = size;
    pb->packets[pb->size].time = t;
    pb->size++;
}

void BandwidthStatistics::received(const ServerID& src, uint32 id, uint32 size, const Time& t) {
    if (receivedBatches.empty() || receivedBatches.back()->full())
        receivedBatches.push_back( new PacketBatch() );

    PacketBatch* pb = receivedBatches.back();
    pb->packets[pb->size].server = src;
    pb->packets[pb->size].id = id;
    pb->packets[pb->size].size = size;
    pb->packets[pb->size].time = t;
    pb->size++;
}

void BandwidthStatistics::save(const String& filename) {
    std::ofstream of(filename.c_str(), std::ios::out);

    of << "Sent" << std::endl;
    of << "DestServer ServerID MessageID Size Time" << std::endl;
    for(std::vector<PacketBatch*>::iterator it = sentBatches.begin(); it != sentBatches.end(); it++) {
        PacketBatch* pb = *it;
        for(uint16 i = 0; i < pb->size; i++)
            of << pb->packets[i].server << " " << GetUniqueIDServerID(pb->packets[i].id) << " " << GetUniqueIDMessageID(pb->packets[i].id) << " " << pb->packets[i].size << " " << pb->packets[i].time.raw() << std::endl;
    }

    of << "Received" << std::endl;
    of << "SourceServer ServerID MessageID Size Time" << std::endl;
    for(std::vector<PacketBatch*>::iterator it = receivedBatches.begin(); it != receivedBatches.end(); it++) {
        PacketBatch* pb = *it;
        for(uint16 i = 0; i < pb->size; i++)
            of << pb->packets[i].server << " " << GetUniqueIDServerID(pb->packets[i].id) << " " << GetUniqueIDMessageID(pb->packets[i].id) << " " << pb->packets[i].size << " " << pb->packets[i].time.raw() << std::endl;
    }
}

} // namespace CBR
