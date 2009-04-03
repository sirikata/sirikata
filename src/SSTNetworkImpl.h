#ifndef _CBR_SST_NETWORK_IMPL_H_
#define _CBR_SST_NETWORK_IMPL_H_

#include "Network.hpp"
#include "stream.h"
#include "host.h"
#include <QApplication>
#include <map>
#include <queue>

using namespace SST;

namespace CBR {

class CBRSST : public QObject {
    Q_OBJECT
public:
    CBRSST();
    ~CBRSST();
    void listen(uint32 port);
    bool send(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority);
    Network::Chunk* receiveOne(const Address4& from, uint32 max_size);
    void service();
    void init(void* (*x)(void*));
private slots:
    void handleConnection();

    void handleReadyReadMessage();
    void handleReadyReadDatagram();

    void handleNewSubstream();

    void handleReset();
    void handleInit();
private:
    void trySendCurrentChunk();

    typedef std::map<Address4, SST::Stream*> StreamMap;

    SST::Stream* lookupOrConnect(const Address4& addy);
    void *(*mMainCallback)(void*);
    QApplication* mApp;
    SST::Host* mHost;
    SST::StreamServer* mAcceptor;

    struct NetworkChunk {
        Address4 addr;
        Network::Chunk data;
        uint32 bytes_sent;
    };
    NetworkChunk* mCurrentSendChunk;
    StreamMap mSendConnections;

    StreamMap mReceiveConnections;
}; // class CBRSST

} // namespace CBR

#endif //_CBR_SST_NETWORK_IMPL_H_
