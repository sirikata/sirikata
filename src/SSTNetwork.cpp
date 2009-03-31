#include "SSTNetwork.hpp"
#include "SSTNetworkImpl.h"

namespace CBR {

SSTNetwork::SSTNetwork(int argc, char**argv)
 : Network()
{
    mImpl = new CBRSST(argc,argv);
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

Network::Chunk* SSTNetwork::receiveOne() {
    return mImpl->receiveOne();
}

void SSTNetwork::service(const Time& t) {
    mImpl->service();
}
void SSTNetwork::init(void* (*x)(void*)){
    mImpl->init(x);
}
} // namespace CBR
