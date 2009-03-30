#ifndef _CBR_SST_NETWORK_HPP_
#define _CBR_SST_NETWORK_HPP_

#include "Network.hpp"

namespace CBR {

class CBRSST;

class SSTNetwork : public Network {
public:
    SSTNetwork();
    virtual ~SSTNetwork();
    virtual bool send(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority);
    virtual void listen (const Address4&);
    virtual Network::Chunk* receiveOne();
    virtual void service(const Time& t);
private:
    CBRSST* mImpl;
};

} // namespace CBR

#endif //_CBR_SST_NETWORK_HPP_
