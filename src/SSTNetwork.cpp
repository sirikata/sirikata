#include "SSTNetwork.hpp"
#include "SSTNetworkImpl.h"

namespace CBR {

SSTNetwork::SSTNetwork(SpaceContext* ctx)
 : Network(ctx)
{
    mImpl = new CBRSST(ctx->trace());
    mServicePoller = new Poller(
        ctx->mainStrand,
        std::tr1::bind(&SSTNetwork::service, this)
    );
}

SSTNetwork::~SSTNetwork() {
    delete mServicePoller;
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

void SSTNetwork::start() {
    Network::start();
    mServicePoller->start();
}

void SSTNetwork::stop() {
    mServicePoller->stop();
    Network::stop();
}

void SSTNetwork::service() {
    mImpl->service();
}

void SSTNetwork::init(void* (*x)(void*)){
    mImpl->init(x);
}

void SSTNetwork::begin() {
    mImpl->start();
}

void SSTNetwork::reportQueueInfo() const {
    mImpl->reportQueueInfo(mContext->time);
}

} // namespace CBR
