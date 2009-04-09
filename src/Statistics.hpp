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
#include "Time.hpp"
#include "MotionVector.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"

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

    std::ostream& write(std::ostream& os) {
        for(uint32 i = 0; i < size; i++)
            items[i].write(os);
        return os;
    }
};

class BatchedBuffer {
public:
    // write the specified number of bytes from the pointer to the buffer
    void write(const void* buf, uint32 nbytes);
    // write the buffer to an ostream
    void write(std::ostream& os);
private:
    typedef Batch<uint8> ByteBatch;
    std::vector<ByteBatch*> batches;
};


class ServerIDMap;

class Trace {
public:
    static const uint8 ProximityTag = 0;
    static const uint8 LocationTag = 1;
    static const uint8 SubscriptionTag = 2;
    static const uint8 ServerDatagramQueuedTag = 3;
    static const uint8 ServerDatagramSentTag = 4;
    static const uint8 ServerDatagramReceivedTag = 5;
    static const uint8 PacketSentTag = 6;
    static const uint8 PacketReceivedTag = 7;

    Trace();

    void setServerIDMap(ServerIDMap* sidmap);

    void prox(const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc);
    void loc(const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc);
    void subscription(const Time& t, const UUID& receiver, const UUID& source, bool start);

    void serverDatagramQueued(const Time& t, const ServerID& dest, uint32 id, uint32 size);
    void serverDatagramSent(const Time& start_time, const Time& end_time, const ServerID& dest, const Network::Chunk& data);
    void serverDatagramSent(const Time& start_time, const Time& end_time, const ServerID& dest, uint32 id, uint32 size);
    void serverDatagramReceived(const Time& start_time, const Time& end_time, const ServerID& src, uint32 id, uint32 size);

    void packetSent(const Time& t, const Address4& dest, uint32 size);
    void packetReceived(const Time& t, const Address4& src, uint32 size);

    void save(const String& filename);
private:
    BatchedBuffer data;
    ServerIDMap* mServerIDMap;
}; // class Trace

} // namespace CBR

#endif //_CBR_STATISTICS_HPP_
