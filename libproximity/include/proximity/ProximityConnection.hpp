
namespace Sirikata { namespace Network {
class Stream;
class Address;
class IOService;
} }
namespace Sirikata { namespace Protocol {
class IMessage;

} }

namespace Sirikata { namespace Proximity {
class ProximitySystem;

class ProximityConnection {
    ProximitySystem *mParent;
    std::tr1::shared_ptr<Network::Stream> mConnectionStream;
    typedef std::tr1::unordered_map<ObjectReference,Network::Stream*,ObjectReference::Hasher> ObjectStreamMap;
    ObjectStreamMap mObjectStreams;
    
public:
    void streamDisconnected();
    ProximityConnection(const Network::Address&addy, ProximitySystem*, Network::IOService&);
    ~ProximityConnection();
    void constructObjectStream(const ObjectReference&obc);
    void deleteObjectStream(const ObjectReference&obc);
    void send(const ObjectReference&,
              const Protocol::IMessage&,
              const void*optionalSerializedMessage,
              const size_t optionalSerializedMessageSize);
};

} }
