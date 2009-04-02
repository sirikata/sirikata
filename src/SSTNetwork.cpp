#include "SSTNetwork.hpp"
#include "SSTNetworkImpl.h"

namespace CBR {

SSTNetwork::SSTNetwork()
 : Network()
{
    mImpl = new CBRSST();
}

SSTNetwork::~SSTNetwork() {
    delete mImpl;
}

bool SSTNetwork::send(const Address4& addy, const Network::Chunk& toSend, bool reliable, bool ordered, int priority) {
    return mImpl->send(addy, toSend, reliable, ordered, priority);
}

void SSTNetwork::listen(const Address4& as_server) {
    mImpl->listen(as_server.getPort());
}

Network::Chunk* SSTNetwork::receiveOne(const Address4& from, uint32 max_size) {
    return mImpl->receiveOne(from, max_size);
}

void SSTNetwork::service(const Time& t) {
    mImpl->service();
}
void SSTNetwork::init(void* (*x)(void*)){
    mImpl->init(x);
}
} // namespace CBR
