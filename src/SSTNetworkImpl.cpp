#include "SSTNetworkImpl.h"

#define SERVICE_NAME "cbr"
#define SERVICE_DESC "Constant Bit Rate Service"
#define PROTOCOL_NAME "cbr1.0"
#define PROTOCOL_DESC "Constant Bit Rate Protocol 1.0"

namespace CBR {

CBRSST::CBRSST()
{
    mApp = new QApplication(0, 0);
}

CBRSST::~CBRSST() {
}
void CBRSST::handleInit() {
    this->mMainCallback(NULL);
}

void CBRSST::init(void* (*x)(void*)){
    mMainCallback=x;
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(handleInit()));
    timer->start(0);
    mApp->exec();
}

void CBRSST::listen(uint32 port) {
    mHost = new SST::Host(NULL, port);
    mAcceptor = new SST::StreamServer(mHost);

    mAcceptor->listen(
        SERVICE_NAME, SERVICE_DESC,
        PROTOCOL_NAME, PROTOCOL_DESC
    );

    connect(mAcceptor, SIGNAL(newConnection()),
        this, SLOT(handleConnection()));
}

bool CBRSST::send(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority) {
    Stream* strm = lookupOrConnect(addy);

    strm->writeMessage((const char*)&data[0], data.size());

    return true;
}

Network::Chunk* CBRSST::receiveOne() {
    if (mReceiveQueue.empty())
        return NULL;
    Network::Chunk* chunk = mReceiveQueue.front();
    mReceiveQueue.pop();
    return chunk;
}

void CBRSST::service() {
    mApp->processEvents();
}


void CBRSST::handleConnection() {
    while(true) {
        SST::Stream *strm = mAcceptor->accept();

        if (strm == NULL)
            return;

        QByteArray remote_id = strm->remoteHostId();
        assert( remote_id.size() == 7 && (uint32)remote_id[0] == 8 );
        uint32 remote_ip;
        uint16 remote_port;
        memcpy(&remote_ip, remote_id.constData() + 1, 4);
        memcpy(&remote_port, remote_id.constData() + 5, 2);
        remote_ip = ntohl(remote_ip);
        remote_port = ntohs(remote_port);

        qDebug() << "remote ip: " << remote_ip << "  remote port: " << remote_port;

        //strm->setChildReceiveBuffer(sizeof(qint32));
        strm->listen(SST::Stream::BufLimit);

        strm->setParent(this);
        connect(strm, SIGNAL(readyReadMessage()),
            this, SLOT(handleReadyReadMessage()));
        connect(strm, SIGNAL(readyReadDatagram()),
            this, SLOT(handleReadyReadDatagram()));
        connect(strm, SIGNAL(reset(const QString &)),
            this, SLOT(handleReset()));
        connect(strm, SIGNAL(newSubstream()),
            this, SLOT(handleNewSubstream()), Qt::QueuedConnection);

        Address4 remote_addy(remote_ip, remote_port);
        assert( mReceiveConnections.find(remote_addy) == mReceiveConnections.end() );
        mReceiveConnections[remote_addy] = strm;
    }
}

void CBRSST::handleReadyReadMessage() {
    SST::Stream* strm = (SST::Stream*)sender();

    while(true) {
        QByteArray msg = strm->readMessage();
        if (msg.isEmpty())
            return;

        Network::Chunk* chunk = new Network::Chunk( (const uint8*)msg.constData(), (const uint8*)msg.constData() + msg.size() );
        mReceiveQueue.push(chunk);
    }
}

void CBRSST::handleReadyReadDatagram() {
    assert(false);
}

void CBRSST::handleNewSubstream() {
    assert(false);
}

void CBRSST::handleReset() {
    SST::Stream* strm = (SST::Stream*)sender();
    strm->deleteLater();
}


SST::Stream* CBRSST::lookupOrConnect(const Address4& addy) {
    // If we have one, just return it
    StreamMap::iterator it = mSendConnections.find(addy);
    if (it != mSendConnections.end())
        return it->second;

    // Otherwise we need to connect
    SST::Stream* strm = new SST::Stream(mHost, this);
    quint32 addy_ip = htonl(addy.ip);
    strm->connectTo(SST::Ident::fromIpAddress(QHostAddress(addy_ip),addy.port).id(), SERVICE_NAME, PROTOCOL_NAME);
    mSendConnections[addy] = strm;
    return strm;
}

} // namespace CBR
