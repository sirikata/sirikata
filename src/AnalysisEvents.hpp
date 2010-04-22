
#ifndef __CBR_ANALYSIS_EVENTS_HPP__
#define __CBR_ANALYSIS_EVENTS_HPP__
#include "Statistics.hpp"
#include "OSegLookupTraceToken.hpp"

namespace CBR {

/** Read a single trace record, storing the type hint in type_hint_out and the result in payload_out.*/
bool read_record(std::istream& is, uint16* type_hint_out, std::string* payload_out);

struct Event {
    static Event* parse(uint16 type_hint, const std::string& record, const ServerID& trace_server_id);

    Event()
     : time(Time::null())
    {}
    virtual ~Event() {}

    Time time;

    virtual Time begin_time() const {
        return time;
    }
    virtual Time end_time() const {
        return time;
    }
};

struct EventTimeComparator {
    bool operator()(const Event* lhs, const Event* rhs) const {
        return (lhs->time < rhs->time);
    }
};

struct ObjectEvent : public Event {
    UUID receiver;
    UUID source;
};

struct ProximityEvent : public ObjectEvent {
    bool entered;
    TimedMotionVector3f loc;
};

struct LocationEvent : public ObjectEvent {
    TimedMotionVector3f loc;
};

// NOTE: This could just reuse PingEvent except we have some backwards
// compatibility problems where analyses would interpret it as an actual ping
// event instead of just a creation, causing double counting.
struct PingCreatedEvent : public ObjectEvent {
    Time sentTime;
    //ping count
    uint64 id;
    double distance;
    PingCreatedEvent():sentTime(Time::null()){}
    virtual Time begin_time() const {
        return sentTime;
    }
    virtual Time end_time() const {
        return time;
    }
};

struct PingEvent : public ObjectEvent {
    Time sentTime;
    //ping count
    uint64 id;
    //unique id for all packets across the board
    uint64 uid;
    double distance;
    PingEvent():sentTime(Time::null()){}
    virtual Time begin_time() const {
        return sentTime;
    }
    virtual Time end_time() const {
        return time;
    }
};

struct GeneratedLocationEvent : public ObjectEvent {
    TimedMotionVector3f loc;
};


struct MessageTimestampEvent : public ObjectEvent {
    uint64 uid;
    Trace::MessagePath path;
};

struct MessageCreationTimestampEvent : public MessageTimestampEvent {
    ObjectMessagePort srcport;
    ObjectMessagePort dstport;
};


struct ServerDatagramQueueInfoEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 send_size;
    uint32 send_queued;
    float send_weight;
};


struct ServerDatagramEvent : public Event {
    ServerID source;
    ServerID dest;
    uint64 id;
    uint32 size;
};

struct ServerDatagramQueuedEvent : public ServerDatagramEvent {
};

struct ServerDatagramSentEvent : public ServerDatagramEvent {
    ServerDatagramSentEvent()
     : ServerDatagramEvent(), _start_time(Time::null()), _end_time(Time::null())
    {}

    virtual Time begin_time() const {
        return _start_time;
    }
    virtual Time end_time() const {
        return _end_time;
    }

    float weight;

    Time _start_time;
    Time _end_time;
};

struct ServerDatagramReceivedEvent : public ServerDatagramEvent {
    ServerDatagramReceivedEvent()
     : ServerDatagramEvent(), _start_time(Time::null()), _end_time(Time::null())
    {}

    virtual Time begin_time() const {
        return _start_time;
    }
    virtual Time end_time() const {
        return _end_time;
    }

    Time _start_time;
    Time _end_time;
};


struct SegmentationChangeEvent : public Event {
  BoundingBox3f bbox;
  ServerID server;
};


struct ObjectBeginMigrateEvent : public Event
{
  UUID mObjID;
  ServerID mMigrateFrom, mMigrateTo;
};

struct ObjectAcknowledgeMigrateEvent : public Event
{
  UUID mObjID;
  ServerID mAcknowledgeFrom, mAcknowledgeTo;
};

struct ObjectCraqLookupEvent: public Event
{
  UUID mObjID;
  ServerID mID_lookup;
};


struct ObjectLookupNotOnServerEvent: public Event
{
  UUID mObjID;
  ServerID mID_lookup;
};

struct ObjectLookupProcessedEvent: public Event
{
  UUID mObjID;
  ServerID mID_processor, mID_objectOn;
  uint32 deltaTime;
  uint32 stillInQueue;
};


struct ServerLocationEvent : public Event {
    ServerID source;
    ServerID dest;
    UUID object;
    TimedMotionVector3f loc;
};

struct ServerObjectEventEvent : public Event {
    ServerID source;
    ServerID dest;
    UUID object;
    bool added;
    TimedMotionVector3f loc;
};

struct ObjectMigrationRoundTripEvent : public Event
{
  UUID obj_id;
  ServerID sID_migratingFrom;
  ServerID sID_migratingTo;
  int numMill;
};

struct OSegTrackedSetResultsEvent : public Event
{
  UUID obj_id;
  ServerID sID_migratingTo;
  int numMill;
};

struct OSegShutdownEvent : public Event
{
  ServerID sID;
  int numLookups;
  int numOnThisServer;
  int numCacheHits;
  int numCraqLookups;
  int numTimeElapsedCacheEviction;
  int numMigrationNotCompleteYet;

};

struct OSegCacheResponseEvent : public Event
{
  ServerID cacheResponseID;
  UUID obj_id;
};

  struct OSegCumulativeEvent : public Event
  {
    OSegLookupTraceToken traceToken;
  };



}

#endif
