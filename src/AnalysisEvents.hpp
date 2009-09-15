
#ifndef __CBR_ANALYSIS_EVENTS_HPP__
#define __CBR_ANALYSIS_EVENTS_HPP__


namespace CBR {
struct Event {
    static Event* read(std::istream& is, const ServerID& trace_server_id);
  
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

struct SubscriptionEvent : public ObjectEvent {
    bool started;
};
struct PingEvent : public ObjectEvent {
    Time sentTime;
    uint64 id;
    double distance;
    PingEvent():sentTime(Time::null()){}
    virtual Time begin_time() const {
        return sentTime;
    }
    virtual Time end_time() const {
        return time;
    }
};


struct ServerDatagramQueueInfoEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 send_size;
    uint32 send_queued;
    float send_weight;
    uint32 receive_size;
    uint32 receive_queued;
    float receive_weight;
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


struct PacketQueueInfoEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 send_size;
    uint32 send_queued;
    float send_weight;
    uint32 receive_size;
    uint32 receive_queued;
    float receive_weight;
};


struct PacketEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 size;
};

struct PacketSentEvent : public PacketEvent {
};

struct PacketReceivedEvent : public PacketEvent {
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

struct ObjectLookupEvent: public Event
{
  UUID mObjID;
  ServerID mID_lookup;
};

struct ObjectLookupProcessedEvent: public Event
{
  UUID mObjID;
  ServerID mID_processor, mID_objectOn;
  uint32 deltaTime;
};


// struct SingleServerOsegFinalDumpEvent : public Event
// {
//   std::vector<UUID> objectsHosted;
//   std::map<UUID,ServerID> inTransitOrLookup;
//   ServerID sID;
// };



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

}

#endif

