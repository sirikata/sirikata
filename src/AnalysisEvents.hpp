namespace CBR {
struct Event {
    static Event* read(std::istream& is, const ServerID& trace_server_id);

    Event()
     : time(0)
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


struct ServerDatagramQueueInfoEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 send_size;
    uint32 send_queued;
    uint32 receive_size;
    uint32 receive_queued;
};


struct ServerDatagramEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 id;
    uint32 size;
};

struct ServerDatagramQueuedEvent : public ServerDatagramEvent {
};

struct ServerDatagramSentEvent : public ServerDatagramEvent {
    ServerDatagramSentEvent()
     : ServerDatagramEvent(), _start_time(0), _end_time(0)
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
     : ServerDatagramEvent(), _start_time(0), _end_time(0)
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
    uint32 receive_size;
    uint32 receive_queued;
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


}
