#include "sirikata/util/Platform.hpp"
#include "sirikata/network/Stream.hpp"
namespace CBR {
class Network;
class RaknetNetwork;
class Address4 {
public:
    unsigned int ip;
    unsigned short port;
    Address4(const Sirikata::Network::Address&a);
    Address4(unsigned int ip, unsigned short prt) {
        this->ip=ip;
        this->port=port;
    }
    bool operator ==(const Address4&other)const {
        return ip==other.ip&&port==other.port;
    }
};
class Network {

public:
    typedef Sirikata::Network::Chunk Chunk;

    virtual void listen (const std::string&service)=0;
    virtual bool sendTo(const Address4&,const Sirikata::Network::Chunk&, bool reliable, bool ordered, int priority)=0;
    virtual Sirikata::Network::Chunk*receiveOne()=0;
};
}
