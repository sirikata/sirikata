#include "SSTNetwork.hpp"
#include "SSTNetworkImpl.h"

namespace CBR {

SSTNetwork::SSTNetwork(SpaceContext* ctx)
 : Network(ctx)
{
    mImpl = new CBRSST(ctx->trace());
}

SSTNetwork::~SSTNetwork() {
    delete mImpl;
}

bool SSTNetwork::canSend(const Address4& addy, uint32 size, bool reliable, bool ordered, int priority) {
    return mImpl->canSend(addy, size, reliable, ordered, priority);
}

bool SSTNetwork::send(const Address4& addy, const Network::Chunk& toSend, bool reliable, bool ordered, int priority) {
    return mImpl->send(addy, toSend, reliable, ordered, priority);
}

void SSTNetwork::listen(const Address4& as_server) {
    mImpl->listen(as_server.getPort());
}

Network::Chunk* SSTNetwork::front(const Address4& from, uint32 max_size) {
    return mImpl->front(from, max_size);
}

Network::Chunk* SSTNetwork::receiveOne(const Address4& from, uint32 max_size) {
    return mImpl->receiveOne(from, max_size);
}

void SSTNetwork::service(const Time& t) {
    mProfiler->started();
    mImpl->service();
    mProfiler->finished();
}

void SSTNetwork::init(void* (*x)(void*)){
    mImpl->init(x);
}

void SSTNetwork::start() {
    mImpl->start();
}

void SSTNetwork::reportQueueInfo(const Time& t) const {
    mImpl->reportQueueInfo(t);
}

} // namespace CBR
