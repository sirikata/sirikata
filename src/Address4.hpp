#ifndef _SIRIKATA_ADDRESS4_HPP_
#define _SIRIKATA_ADDRESS4_HPP_

#include "Utility.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Address.hpp>

namespace Sirikata {

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

}
#endif //_SIRIKATA_ADDRESS4_HPP_
