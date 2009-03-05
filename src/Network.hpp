#ifndef _CBR_NETWORK_HPP_
#define _CBR_NETWORK_HPP_
#include "Utility.hpp"
#include "sirikata/util/Platform.hpp"
#include "sirikata/network/Stream.hpp"

namespace CBR {
class Network;
class RaknetNetwork;
class Address4 {
public:
    unsigned int ip;
    unsigned short port;
    class Hasher{
    public:
        size_t operator() (const Address4&addy)const {
            return std::tr1::hash<unsigned int>()(addy.ip^addy.port);
        }
    };
    Address4() {
        ip=port=0;
    }
    Address4(const Sirikata::Network::Address&a);
    Address4(unsigned int ip, unsigned short prt) {
        this->ip=ip;
        this->port=prt;
    }
    bool operator ==(const Address4&other)const {
        return ip==other.ip&&port==other.port;
    }

    uint16 getPort() const {
        return port;
    }
};
class ServerIDMap;
class Network {
protected:
    ServerIDMap * mServerIDMap;
public:
    virtual bool sendTo(const Address4&,const Sirikata::Network::Chunk&, bool reliable, bool ordered, int priority)=0;
    typedef Sirikata::Network::Chunk Chunk;
    Network(ServerIDMap*sidm):mServerIDMap(sidm){}
    virtual bool send(const ServerID&,const Network::Chunk&, bool reliable, bool ordered, int priority);
    virtual void listen (const ServerID& as_server)=0;
    virtual Sirikata::Network::Chunk*receiveOne()=0;
};
}
#endif //_CBR_NETWORK_HPP_
