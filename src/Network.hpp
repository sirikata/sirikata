#ifndef _CBR_NETWORK_HPP_
#define _CBR_NETWORK_HPP_
#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "sirikata/util/Platform.hpp"
#include "sirikata/network/Stream.hpp"
#include "PollingService.hpp"

namespace CBR {
class Network;
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
    bool operator!=(const Address4& other) const {
        return ip != other.ip || port != other.port;
    }
    bool operator<(const Address4& other) const {
        return (ip < other.ip) || (ip == other.ip && port < other.port);
    }
    uint16 getPort() const {
        return port;
    }

    static Address4 Null;
};
Sirikata::Network::Address convertAddress4ToSirikata(const Address4&addy);
inline size_t hash_value(const Address4&addy) {
    return std::tr1::hash<unsigned int>()(addy.ip)^std::tr1::hash<unsigned short>()(addy.port);
}
class Network : public PollingService {
public:
    typedef Sirikata::Network::Chunk Chunk;

    virtual ~Network() {}

    virtual void init(void*(*)(void*))=0;
    // Called right before we start the simulation, useful for syncing network timing info to Time(0)
    virtual void start() = 0;

    // Checks if this chunk, when passed to send, would be successfully pushed.
    virtual bool canSend(const Address4&,uint32 size, bool reliable, bool ordered, int priority)=0;
    virtual bool send(const Address4&,const Chunk&, bool reliable, bool ordered, int priority)=0;

    virtual void listen (const Address4&)=0;
    virtual Chunk* front(const Address4& from, uint32 max_size)=0;
    virtual Chunk* receiveOne(const Address4& from, uint32 max_size)=0;

    virtual void reportQueueInfo(const Time& t) const = 0;

protected:
    virtual void service() = 0;

    Network(SpaceContext* ctx);

    SpaceContext* mContext;
private:
    virtual void poll();
    TimeProfiler::Stage* mProfiler;

    Duration mStatsSampleRate;
    Time mLastStatsSample;
};
}
#endif //_CBR_NETWORK_HPP_
