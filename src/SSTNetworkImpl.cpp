#include "SSTNetworkImpl.h"

#define SERVICE_NAME "cbr"
#define SERVICE_DESC "Constant Bit Rate Service"
#define PROTOCOL_NAME "cbr1.0"
#define PROTOCOL_DESC "Constant Bit Rate Protocol 1.0"

namespace CBR {

SSTStatsListener::SSTStatsListener(const QTime& start)
 : mStartTime(start)
{
}

void SSTStatsListener::packetSent(qint32 size) {
    qint32 msecs = mStartTime.elapsed();
    //printf("packet sent: %d at %d\n", size, msecs);
}

void SSTStatsListener::packetReceived(qint32 size) {
    qint32 msecs = mStartTime.elapsed();
    //printf("packet received: %d at %d\n", size, msecs);
}


CBRSST::CBRSST()
{
    mApp = new QApplication(0, 0);
    mCurrentSendChunk = NULL;
}

CBRSST::~CBRSST() {
}
void CBRSST::handleInit() {
    this->mMainCallback(NULL);
    mApp->exit();
}

void CBRSST::init(void* (*x)(void*)){
    mMainCallback=x;
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(handleInit()));
    timer->start(0);
    mApp->exec();
}

void CBRSST::start() {
    mStartTime.start();
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

void CBRSST::trySendCurrentChunk() {
    // try to service the most recent
    if (mCurrentSendChunk == NULL) return;

    StreamInfo si = lookupOrConnect(mCurrentSendChunk->addr);
    Stream* strm = si.stream;
    uint32 new_bytes_sent = strm->writeMessage((const char*)&mCurrentSendChunk->data[mCurrentSendChunk->bytes_sent], mCurrentSendChunk->data.size() - mCurrentSendChunk->bytes_sent);
    mCurrentSendChunk->bytes_sent += new_bytes_sent;

    if (mCurrentSendChunk->bytes_sent == mCurrentSendChunk->data.size()) {
        delete mCurrentSendChunk;
        mCurrentSendChunk = NULL;
    }
}

bool CBRSST::send(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority) {
    trySendCurrentChunk();

    if (mCurrentSendChunk != NULL)
        return false;

    mCurrentSendChunk = new NetworkChunk();
    mCurrentSendChunk->addr = addy;
    mCurrentSendChunk->data = data;
    mCurrentSendChunk->bytes_sent = 0;

    trySendCurrentChunk();

    return true;
}

Network::Chunk* CBRSST::receiveOne(const Address4& from, uint32 max_size) {
    SST::Stream* strm = NULL;

    if (from == Address4::Null) {
        // FIXME this should do something smarter than just choosing the first one with data
        for(StreamMap::iterator it = mReceiveConnections.begin(); it != mReceiveConnections.end(); it++) {
            Stream* it_strm = it->second.stream;
            if (it_strm->hasPendingMessages() && it_strm->pendingMessageSize() <= max_size) {
                strm = it_strm;
                break;
            }
        }
        if (strm == NULL) return NULL;
    }
    else {
        strm = lookupOrConnect(from).stream;
    }

    if (strm->hasPendingMessages() && strm->pendingMessageSize() <= max_size) {
        QByteArray msg = strm->readMessage();
        if (msg.isEmpty())
            return NULL;

        return new Network::Chunk( (const uint8*)msg.constData(), (const uint8*)msg.constData() + msg.size() );
    }

    return NULL;
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

        SSTStatsListener* stats_listener = new SSTStatsListener(mStartTime);
        strm->setStatListener( stats_listener );

        StreamInfo si;
        si.stream = strm;
        si.stats = stats_listener;

        assert( mReceiveConnections.find(remote_addy) == mReceiveConnections.end() );
        mReceiveConnections[remote_addy] = si;
    }
}

void CBRSST::handleReadyReadMessage() {
    // don't do anything, they'll be serviced on demand in the receive method
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


CBRSST::StreamInfo CBRSST::lookupOrConnect(const Address4& addy) {
    // If we have one, just return it
    StreamMap::iterator it = mSendConnections.find(addy);
    if (it != mSendConnections.end())
        return it->second;

    // Otherwise we need to connect
    SST::Stream* strm = new SST::Stream(mHost, this);
    quint32 addy_ip = htonl(addy.ip);
    strm->connectTo(SST::Ident::fromIpAddress(QHostAddress(addy_ip),addy.port).id(), SERVICE_NAME, PROTOCOL_NAME);

    SSTStatsListener* stats_listener = new SSTStatsListener(mStartTime);
    strm->setStatListener( stats_listener );

    StreamInfo si;
    si.stream = strm;
    si.stats = stats_listener;

    mSendConnections[addy] = si;

    return si;
}

} // namespace CBR
