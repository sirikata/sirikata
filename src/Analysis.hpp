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
#include "MotionVector.hpp"
#include "AnalysisEvents.hpp"
#include "OSegLookupTraceToken.hpp"


namespace CBR {

struct Event;
struct ObjectEvent;
struct ServerDatagramEvent;
struct ServerDatagramSentEvent;
struct ServerDatagramQueuedEvent;
struct ServerDatagramReceivedEvent;
struct ServerDatagramQueueInfoEvent;
struct PacketQueueInfoEvent;

/** Error of observed vs. true object locations over simulation period. */
class LocationErrorAnalysis {
public:
    LocationErrorAnalysis(const char* opt_name, const uint32 nservers);
    ~LocationErrorAnalysis();

    // Return true if observer was watching seen at any point during the simulation
    bool observed(const UUID& observer, const UUID& seen) const;

    // Return the average error in the approximation of an object over its observed period, sampled at the given rate.
    double averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate) const;
    // Return the average error in object position approximation over all observers and observed objects, sampled at the given rate.
    double globalAverageError(const Duration& sampling_rate) const;
protected:
    typedef std::vector<Event*> EventList;
    typedef std::map<UUID, EventList*> ObjectEventListMap;
    typedef std::map<ServerID, EventList*> ServerEventListMap;

    EventList* getEventList(const UUID& observer) const;
    EventList* getEventList(const ServerID& observer) const;

    ObjectEventListMap mEventLists;
    ServerEventListMap mServerEventLists;
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
    // No datagram receive queues

    void windowedPacketSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);
    void windowedPacketReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out);


   void computeJFI(const ServerID& server_id) const;

private:
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

    DatagramQueueInfoEventList::const_iterator datagramQueueInfoBegin(const ServerID& server) const;
    DatagramQueueInfoEventList::const_iterator datagramQueueInfoEnd(const ServerID& server) const;
    PacketQueueInfoEventList::const_iterator packetQueueInfoBegin(const ServerID& server) const;
    PacketQueueInfoEventList::const_iterator packetQueueInfoEnd(const ServerID& server) const;

    const DatagramEventList* getDatagramEventList(const ServerID& server) const;

    const DatagramQueueInfoEventList* getDatagramQueueInfoEventList(const ServerID& server) const;
    const PacketQueueInfoEventList* getPacketQueueInfoEventList(const ServerID& server) const;

    template<typename EventType, typename EventIteratorType>
    void computeJFI(const ServerID& sender, const ServerID& filter) const;

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

public:
    LatencyAnalysis(const char* opt_name, const uint32 nservers);
    ~LatencyAnalysis();

private:
    uint32_t mNumberOfServers;
}; // class BandwidthAnalysis


  //all of oseg analyses

  static const int OSEG_SECOND_TO_RAW_CONVERSION_FACTOR = 1000000;

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

  static bool compareObjectBeginMigrateEvts(ObjectBeginMigrateEvent A, ObjectBeginMigrateEvent B);
  static bool compareObjectAcknowledgeMigrateEvts(ObjectAcknowledgeMigrateEvent A, ObjectAcknowledgeMigrateEvent B);
  void convertToEvtsAndSort(std::vector<ObjectBeginMigrateEvent> &sortedBeginMigrateEvents, std::vector<ObjectAcknowledgeMigrateEvent> &sortedAcknowledgeMigrateEvents);

public:
  ObjectSegmentationAnalysis(const char* opt_name, const uint32 nservers);

  void printData(std::ostream &fileOut, bool sortByTime=true);
  ~ObjectSegmentationAnalysis();

}; //class ObjectSegmentationAnalysis




class ObjectSegmentationCraqLookupRequestsAnalysis
{
private:
  std::vector<Time> times;
  std::vector<UUID> obj_ids;
  std::vector<ServerID> sID_lookup;

  void convertToEvtsAndSort(std::vector<ObjectCraqLookupEvent>&);
  static bool compareEvts(ObjectCraqLookupEvent A, ObjectCraqLookupEvent B);

public:
  ObjectSegmentationCraqLookupRequestsAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream &fileOut, bool sortByTime=true);
  ~ObjectSegmentationCraqLookupRequestsAnalysis();
};



class ObjectSegmentationLookupNotOnServerRequestsAnalysis
{
private:
  std::vector<Time> times;
  std::vector<UUID> obj_ids;
  std::vector<ServerID> sID_lookup;

  void convertToEvtsAndSort(std::vector<ObjectLookupNotOnServerEvent>&);
  static bool compareEvts(ObjectLookupNotOnServerEvent A, ObjectLookupNotOnServerEvent B);

public:
  ObjectSegmentationLookupNotOnServerRequestsAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream &fileOut, bool sortByTime=true);
  ~ObjectSegmentationLookupNotOnServerRequestsAnalysis();
};



class ObjectSegmentationProcessedRequestsAnalysis
{
private:
  std::vector<Time> times;
  std::vector<UUID> obj_ids;
  std::vector<ServerID> sID_processor;
  std::vector<ServerID> sID_objectOn;
  std::vector<uint32> dTimes;
  std::vector<uint32> stillInQueues;

  void convertToEvtsAndSort(std::vector<ObjectLookupProcessedEvent>&);
  static bool compareEvts(ObjectLookupProcessedEvent A, ObjectLookupProcessedEvent B);
public:
  ObjectSegmentationProcessedRequestsAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream &fileOut, bool sortByTime = true, int processedAfter =0);
  void printDataCSV(std::ostream &fileOut, bool sortedByTime = true, int processAfter=0);
  ~ObjectSegmentationProcessedRequestsAnalysis();
};




class ObjectMigrationRoundTripAnalysis
{
private:
  std::vector< ObjectMigrationRoundTripEvent> allRoundTripEvts;
  static bool compareEvts (ObjectMigrationRoundTripEvent A, ObjectMigrationRoundTripEvent B);

public:
  ObjectMigrationRoundTripAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream &fileOut, int processedAfter= 0);
  ~ObjectMigrationRoundTripAnalysis();

};



class OSegTrackedSetResultsAnalysis
{
private:
  std::vector<OSegTrackedSetResultsEvent> allTrackedSetResultsEvts;
  static bool compareEvts(OSegTrackedSetResultsEvent A, OSegTrackedSetResultsEvent B);

public:
  OSegTrackedSetResultsAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream &fileOut, int processedAfter = 0);
  ~OSegTrackedSetResultsAnalysis();
};


  class OSegShutdownAnalysis
  {
  private:
    std::vector<OSegShutdownEvent> allShutdownEvts;

  public:
    OSegShutdownAnalysis(const char* opt_name, const uint32 nservers);
    ~OSegShutdownAnalysis();
    void printData(std::ostream &fileOut);
  };

  class OSegCacheResponseAnalysis
  {
  private:
    std::vector<OSegCacheResponseEvent> allCacheResponseEvts;
    static bool compareEvts(OSegCacheResponseEvent A, OSegCacheResponseEvent B);

  public:
    OSegCacheResponseAnalysis(const char* opt_name, const uint32 nservers);
    ~OSegCacheResponseAnalysis();
    void printData(std::ostream &fileOut, int processAfter =0);
  };


  class OSegCumulativeTraceAnalysis
  {
  private:
    static const uint64_t OSEG_CUMULATIVE_ANALYSIS_SECONDS_TO_MICROSECONDS = 1000000;
    std::vector<OSegCumulativeEvent*> allTraces;
    void filterShorterPath(uint64 time_after_microseconds);
    void generateAllData();
    void generateCacheTime();
    void generateGetCraqLookupPostTime();
    void generateCraqLookupTime();
    void generateCraqLookupNotAlreadyLookingUpTime();
    void generateManagerPostTime();
    void generateManagerEnqueueTime();
    void generateManagerDequeueTime();
    void generateConnectionPostTime();
    void generateConnectionNetworkQueryTime();
    void generateConnectionNetworkTime();
    void generateReturnPostTime();
    void generateLookupReturnTime();
    void generateCompleteLookupTime();
    void generateFullTime();

    uint64 mInitialTime;

    struct CumulativeTraceData
    {
      uint64 cacheTime;
      uint64 craqLookupPostTime;
      uint64 craqLookupTime;
      uint64 craqLookupNotAlreadyLookingUpTime;
      uint64 managerPostTime;
      uint64 managerEnqueueTime;
      uint64 managerDequeueTime;
      uint64 connectionPostTime;
      uint64 connectionNetworkQueryTime;
      uint64 connectionsNetworkTime;
      uint64 returnPostTime;
      uint64 lookupReturnsTime;
      uint64 completeLookupTime;
      uint64 fullTime;
    };

    struct OSegCumulativeDurationComparator
    {
      bool operator()(const CumulativeTraceData* lhs, const CumulativeTraceData* rhs) const
      {
        return (lhs->completeLookupTime < rhs->completeLookupTime);
      }
    };

    std::vector<CumulativeTraceData*> mCumData;
    void sortByCompleteLookupTime( );

  public:
    OSegCumulativeTraceAnalysis(const char* opt_name, const uint32 nservers, uint64 time_after_seconds =  0);
    ~OSegCumulativeTraceAnalysis();
    void printData(std::ostream &fileOut);
    void printDataHuman(std::ostream &fileOut);
  };




class OSegCacheErrorAnalysis
{

public:

  struct ServerIDTimePair
  {
    ServerID sID;
    Time t;

    ServerIDTimePair()
      : t(Time::null())
    {
    }

  };


  struct Results
  {
    int missesCache;
    int hitsCache;
    int totalCache;
    int totalLookups;
  };


private:

  typedef std::vector<ServerIDTimePair> LocationList;
  typedef std::map<UUID, LocationList>  ObjectLocationMap;

  ObjectLocationMap mObjLoc;

  std::vector< ObjectMigrationRoundTripEvent >                mMigrationVector; //round trip event
  std::vector< ObjectLookupProcessedEvent >                      mLookupVector; //lookup proc'd
  std::vector< OSegCacheResponseEvent >                   mCacheResponseVector;
  std::vector< ObjectLookupNotOnServerEvent >   mObjectLookupNotOnServerVector;

  //basic strategy: load all migration events.
  //write all migrate times in for each object.
  //load all lookup events....augment map with their lookups.  (If receive a lookup before we have any migrations, we can know that these values are real.)

  bool checkHit(const UUID& obj_ider, const Time& ter, const ServerID& sID);
  void analyzeMisses(Results& res, double processedAfterInMicro);
  void buildObjectMap();

  static bool compareCacheResponseEvents(OSegCacheResponseEvent A, OSegCacheResponseEvent B);
  static bool compareLookupProcessedEvents(ObjectLookupProcessedEvent A, ObjectLookupProcessedEvent B);
  static bool compareRoundTripEvents(ObjectMigrationRoundTripEvent A, ObjectMigrationRoundTripEvent B);


  void analyzeMisses(Results& res);

public:
  OSegCacheErrorAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream& fileOut, int processAfter = 0);
  ~OSegCacheErrorAnalysis();
};




/** Computes distance vs. latency of location updates for objects hosted on the same
 *  object host (but possibly connected to different space servers).
 */
void LocationLatencyAnalysis(const char* opt_name, const uint32 nservers);

/** Dump proximity results to a text file for external analysis.  You probably only
 *  want to use this if you have somehow limited the proximity results.
 */
void ProximityDumpAnalysis(const char* opt_name, const uint32 nservers, const String& outfilename);

} // namespace CBR

#endif //_CBR_ANALYSIS_HPP_
