/*  cbr
 *  Analysis.hpp
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

#ifndef _CBR_ANALYSIS_HPP_
#define _CBR_ANALYSIS_HPP_

#include "Utility.hpp"
#include "Time.hpp"
#include "MotionVector.hpp"
#include "ServerNetwork.hpp"

namespace CBR {

struct Event;
struct ObjectEvent;
struct ServerDatagramEvent;
struct ServerDatagramSentEvent;
struct ServerDatagramQueuedEvent;
struct ServerDatagramReceivedEvent;
struct ServerDatagramQueueInfoEvent;
struct PacketEvent;
struct PacketQueueInfoEvent;
class ObjectFactory;

/** Error of observed vs. true object locations over simulation period. */
class LocationErrorAnalysis {
public:
    LocationErrorAnalysis(const char* opt_name, const uint32 nservers);
    ~LocationErrorAnalysis();

    // Return true if observer was watching seen at any point during the simulation
    bool observed(const UUID& observer, const UUID& seen) const;

    // Return the average error in the approximation of an object over its observed period, sampled at the given rate.
    double averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate, ObjectFactory* obj_factory) const;
    // Return the average error in object position approximation over all observers and observed objects, sampled at the given rate.
    double globalAverageError(const Duration& sampling_rate, ObjectFactory* obj_factory) const;
protected:
    typedef std::vector<ObjectEvent*> EventList;
    typedef std::map<UUID, EventList*> ObjectEventListMap;

    EventList* getEventList(const UUID& observer) const;

    ObjectEventListMap mEventLists;
}; // class LocationErrorAnalysis

/** Does analysis of bandwidth, e.g. checking total bandwidth in and out of a server,
 *  checking relative bandwidths when under load, etc.
 */
class BandwidthAnalysis {
public:
    BandwidthAnalysis(const char* opt_name, const uint32 nservers);
    ~BandwidthAnalysis();

    void computeSendRate(const ServerID& sender, const ServerID& receiver) const;
    void computeReceiveRate(const ServerID& sender, const ServerID& receiver) const;

    void computeWindowedDatagramSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);
    void computeWindowedDatagramReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);

    void computeWindowedPacketSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);
    void computeWindowedPacketReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);

    void dumpDatagramQueueInfo(const ServerID& sender, const ServerID& receiver, std::ostream& summary_out, std::ostream& detail_out);
    void dumpPacketQueueInfo(const ServerID& sender, const ServerID& receiver, std::ostream& summary_out, std::ostream& detail_out);


    void windowedDatagramSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);
    void windowedDatagramReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);

    void windowedPacketSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);
    void windowedPacketReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);


   void computeJFI(const ServerID& server_id) const;

private:
    typedef std::vector<PacketEvent*> PacketEventList;
    typedef std::map<ServerID, PacketEventList*> ServerPacketEventListMap;
    PacketEventList mEmptyPacketEventList;
    typedef std::vector<ServerDatagramEvent*> DatagramEventList;
    typedef std::map<ServerID, DatagramEventList*> ServerDatagramEventListMap;
    DatagramEventList mEmptyDatagramEventList;

    typedef std::vector<PacketQueueInfoEvent*> PacketQueueInfoEventList;
    typedef std::map<ServerID, PacketQueueInfoEventList*> ServerPacketQueueInfoEventListMap;
    PacketQueueInfoEventList mEmptyPacketQueueInfoEventList;
    typedef std::vector<ServerDatagramQueueInfoEvent*> DatagramQueueInfoEventList;
    typedef std::map<ServerID, DatagramQueueInfoEventList*> ServerDatagramQueueInfoEventListMap;
    DatagramQueueInfoEventList mEmptyDatagramQueueInfoEventList;

    DatagramEventList::const_iterator datagramBegin(const ServerID& server) const;
    DatagramEventList::const_iterator datagramEnd(const ServerID& server) const;
    PacketEventList::const_iterator packetBegin(const ServerID& server) const;
    PacketEventList::const_iterator packetEnd(const ServerID& server) const;

    DatagramQueueInfoEventList::const_iterator datagramQueueInfoBegin(const ServerID& server) const;
    DatagramQueueInfoEventList::const_iterator datagramQueueInfoEnd(const ServerID& server) const;
    PacketQueueInfoEventList::const_iterator packetQueueInfoBegin(const ServerID& server) const;
    PacketQueueInfoEventList::const_iterator packetQueueInfoEnd(const ServerID& server) const;

    const DatagramEventList* getDatagramEventList(const ServerID& server) const;
    const PacketEventList* getPacketEventList(const ServerID& server) const;

    const DatagramQueueInfoEventList* getDatagramQueueInfoEventList(const ServerID& server) const;
    const PacketQueueInfoEventList* getPacketQueueInfoEventList(const ServerID& server) const;

    template<typename EventType, typename EventIteratorType>
    void computeJFI(const ServerID& sender, const ServerID& filter) const;

    ServerPacketEventListMap mPacketEventLists;
    ServerDatagramEventListMap mDatagramEventLists;

    ServerPacketQueueInfoEventListMap mPacketQueueInfoEventLists;
    ServerDatagramQueueInfoEventListMap mDatagramQueueInfoEventLists;

    uint32_t mNumberOfServers;
}; // class BandwidthAnalysis



/** Does analysis of bandwidth, e.g. checking total bandwidth in and out of a server,
 *  checking relative bandwidths when under load, etc.
 */
class LatencyAnalysis {
    class PacketData {
        uint32 mSize;
        uint64 mId;
        ServerID source;
        ServerID dest;
        Time _send_start_time;
        Time _send_end_time;
        Time _receive_start_time;
        Time _receive_end_time;
        friend class LatencyAnalysis;
    public:

        PacketData();
        void addPacketSentEvent(ServerDatagramQueuedEvent*);
        void addPacketReceivedEvent(ServerDatagramReceivedEvent*);
    };
    class SourceDestinationPair{public:
        ServerID source;
        ServerID destination;
        SourceDestinationPair(const ServerID& src, const ServerID&dst) {
            source=src;
            destination=dst;
        }
        bool operator <  (const SourceDestinationPair &Pair) const{
            if (Pair.source==source) return destination<Pair.destination;
            return source<Pair.source;
        }
        bool operator ==(const SourceDestinationPair &Pair) const{
            return Pair.source==source&&Pair.destination==destination;
        }
        class Hasher{
        public:
            size_t operator()(const SourceDestinationPair&Pair)const {
                return std::tr1::hash<ServerID>()(Pair.source)^std::tr1::hash<ServerID>()(Pair.destination);
            }
        };
    };
public:
    typedef std::tr1::unordered_multimap<SourceDestinationPair,PacketData,SourceDestinationPair::Hasher> ServerPairPacketMap;
    ServerPairPacketMap mServerPairPacketMap;
    LatencyAnalysis(const char* opt_name, const uint32 nservers);
    ~LatencyAnalysis();


private:
    uint32_t mNumberOfServers;
}; // class BandwidthAnalysis




class ObjectSegmentationAnalysis
{

private:
  std::vector<Time> objectBeginMigrateTimes;
  std::vector<UUID> objectBeginMigrateID;
  std::vector<ServerID> objectBeginMigrateMigrateFrom;
  std::vector<ServerID> objectBeginMigrateMigrateTo;


  std::vector<Time> objectAcknowledgeMigrateTimes;
  std::vector<UUID> objectAcknowledgeMigrateID;
  std::vector<ServerID> objectAcknowledgeAcknowledgeFrom;
  std::vector<ServerID> objectAcknowledgeAcknowledgeTo;

public:
  ObjectSegmentationAnalysis(const char* opt_name, const uint32 nservers);

  void printData(std::ostream &fileOut);
  ~ObjectSegmentationAnalysis();

}; //class ObjectSegmentationAnalysis


} // namespace CBR

#endif //_CBR_ANALYSIS_HPP_
