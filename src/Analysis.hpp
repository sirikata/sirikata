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
#include "ServerNetwork.hpp"
#include "AnalysisEvents.hpp"

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

class ObjectLatencyAnalysis {
    std::map<double, Duration> mLatency;
public:
    int mNumberOfServers;
    ObjectLatencyAnalysis(const char* opt_name, const uint32 nservers);
    ~ObjectLatencyAnalysis();
    struct Average{
        Duration time;
        int numSamples;
        Average(const Duration&t) :time(t){numSamples=0;}
    };
    void histogramDistanceData(double bucketWidth, std::map<int, Average> &retval);
    void printHistogramDistanceData(std::ostream&out, double bucketWidth);
};

class MessageLatencyAnalysis {public:
    class DTime:public Time {
    public:
        Trace::MessagePath mPath;
        bool isNull() const {
            const Time * t=this;
            return *t==Time::null();
        }
        uint32 mServerId;
        DTime():Time(Time::null()) {mServerId=0;mPath=Trace::NUM_PATHS;}
        DTime(const Time&t, Trace::MessagePath path):Time(t){mServerId=0;mPath=path;}
        bool operator < (const Time&other)const {
            return *static_cast<const Time*>(this)<other;
        }
        bool operator == (const Time&other)const {
            return *static_cast<const Time*>(this)==other;
        }
    };
    class Average {
    public:
        double average;
        unsigned int numSamples;
        double variance;
        Average(): average(0){
            variance=0;
            numSamples=0;
        }
        void addAverageSample(Duration t){
            double st=t.toSeconds();
            st=fabs(st);
            average+=st;

            ++numSamples;
        }
        void averageOut(){ 
            if (numSamples) {
                average=average/(double)numSamples;
            }
        }
        void addVarianceSample(Duration t) {
            double diff=(t.toSeconds()-average);
            variance+=diff*diff/(double)numSamples;
        }
    };
    class PacketData {public:
        uint64 mId;
        unsigned char mType;
        unsigned char mSrcPort;        
        unsigned short mDstPort;
        std::vector<DTime> mStamps;
        PacketData();
    };
    class Filters {
    public:
        const uint32*mFilterByCreationServer;
        const uint32*mFilterByDestructionServer;
        const uint32*mFilterByForwardingServer;
        const uint32 *mFilterByDeliveryServer;
        const uint16 *mDestPort;
        Filters(uint16 *destPort=NULL, const uint32*filterByCreationServer=NULL,const uint32 *filterByDestructionServer=NULL, const uint32*filterByForwardingServer=NULL, const uint32 *filterByDeliveryServer=NULL) {
            mDestPort=destPort;
            mFilterByCreationServer=filterByCreationServer;
            mFilterByDestructionServer=filterByDestructionServer;
            mFilterByForwardingServer=filterByForwardingServer;
            mFilterByDeliveryServer=filterByDeliveryServer;
        }
        bool verify(const uint32*server,const PacketData &pd, Trace::MessagePath path) const{
            if (server==NULL) return true;
            for (std::vector<DTime>::const_iterator i=pd.mStamps.begin(),ie=pd.mStamps.end();i!=ie;++i) {
                
                if (i->mPath==path)
                    return i->mServerId==*server;
            }
            return false;
            
        }
        bool operator() (const PacketData&pd)const{
            return (mDestPort==NULL||pd.mDstPort==*mDestPort)&&
                verify(mFilterByCreationServer,pd,Trace::CREATED)&&
                verify(mFilterByDestructionServer,pd,Trace::DESTROYED)&&
                verify(mFilterByForwardingServer,pd,Trace::FORWARDED)&&
                verify(mFilterByDeliveryServer,pd,Trace::DELIVERED);
        }
    };
    MessageLatencyAnalysis(const char* opt_name, const uint32 nservers, Filters f);
    ~MessageLatencyAnalysis();
    Filters mFilter;
    uint32_t mNumberOfServers;
};

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




class ObjectSegmentationLookupRequestsAnalysis
{
private:
  std::vector<Time> times;
  std::vector<UUID> obj_ids;
  std::vector<ServerID> sID_lookup;

  void convertToEvtsAndSort(std::vector<ObjectLookupEvent>&);
  static bool compareEvts(ObjectLookupEvent A, ObjectLookupEvent B);

public:
  ObjectSegmentationLookupRequestsAnalysis(const char* opt_name, const uint32 nservers);
  void printData(std::ostream &fileOut, bool sortByTime=true);
  ~ObjectSegmentationLookupRequestsAnalysis();
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

  std::vector< ObjectMigrationRoundTripEvent > mMigrationVector; //round trip event
  std::vector< ObjectLookupProcessedEvent >       mLookupVector; //lookup proc'd
  std::vector< OSegCacheResponseEvent >    mCacheResponseVector;
  
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

} // namespace CBR

#endif //_CBR_ANALYSIS_HPP_
