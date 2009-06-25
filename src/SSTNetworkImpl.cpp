#include "SSTNetworkImpl.h"
#define SERVICE_NAME "cbr"
#define SERVICE_DESC "Constant Bit Rate Service"
#define PROTOCOL_NAME "cbr1.0"
#define PROTOCOL_DESC "Constant Bit Rate Protocol 1.0"

namespace CBR {

SSTStatsListener::SSTStatsListener(Trace* trace, const QTime& start, const Address4& remote)
 : mTrace(trace),
   mStartTime(start),
   mRemote(remote)
{
}

void SSTStatsListener::packetSent(qint32 size) {
    qint32 msecs = mStartTime.elapsed();

    mTrace->packetSent( Time(0) + Duration::milliseconds((uint32)msecs), mRemote, size);
}

void SSTStatsListener::packetReceived(qint32 size) {
    qint32 msecs = mStartTime.elapsed();

    mTrace->packetReceived( Time(0) + Duration::milliseconds((uint32)msecs), mRemote, size);
}

static int argc = 1;
static char* argv[] = { "blah", NULL };

CBRSST::CBRSST(Trace* trace)
 : mTrace(trace)
{
    mApp = new QApplication((int&)argc, (char**)argv);
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

void CBRSST::trySendCurrentChunk(StreamInfo* si) {
    // try to service the most recent
    if (si->currentSendChunk == NULL) return;

    Stream* strm = si->stream;
    uint32 new_bytes_sent = strm->writeMessage((const char*)&si->currentSendChunk->data[si->currentSendChunk->bytes_sent], si->currentSendChunk->data.size() - si->currentSendChunk->bytes_sent);
    si->currentSendChunk->bytes_sent += new_bytes_sent;

    if (si->currentSendChunk->bytes_sent == si->currentSendChunk->data.size()) {
        delete si->currentSendChunk;
        si->currentSendChunk = NULL;
    }
}

bool CBRSST::canSend(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority) {
    StreamInfo* si = lookupOrConnectSend(addy);
    trySendCurrentChunk(si);

    if (si->currentSendChunk != NULL)
        return false;

    return true;
}

bool CBRSST::send(const Address4& addy, const Network::Chunk& data, bool reliable, bool ordered, int priority) {
    StreamInfo* si = lookupOrConnectSend(addy);
    trySendCurrentChunk(si);

    if (si->currentSendChunk != NULL)
        return false;

    si->currentSendChunk = new NetworkChunk();
    si->currentSendChunk->addr = addy;
    si->currentSendChunk->data = data;
    si->currentSendChunk->bytes_sent = 0;

    trySendCurrentChunk(si);

    return true;
}

Network::Chunk* CBRSST::front(const Address4& from, uint32 max_size) {
    assert(from != Address4::Null);

    StreamInfo* si = lookupReceive(from);
    if (si == NULL) return NULL;

    if (si->peek != NULL) return si->peek;

    SST::Stream* strm = si->stream;

    if (strm->hasPendingMessages() && strm->pendingMessageSize() <= max_size) {
        QByteArray msg = strm->peekMessage();
        if (msg.isEmpty())
            return NULL;

        si->peek = new Network::Chunk( (const uint8*)msg.constData(), (const uint8*)msg.constData() + msg.size() );
        return si->peek;
    }

    return NULL;
}

Network::Chunk* CBRSST::receiveOne(const Address4& from, uint32 max_size) {
    assert(from != Address4::Null);

    StreamInfo* si = lookupReceive(from);
    if (si == NULL) return NULL;

    SST::Stream* strm = si->stream;

    if (strm->hasPendingMessages() && strm->pendingMessageSize() <= max_size) {
        QByteArray msg = strm->readMessage();

        Network::Chunk* result = si->peek;

        if (result) {
            assert( (uint32)result->size() == (uint32)msg.size() );
            si->peek = NULL;
            return result;
        }

        if (msg.isEmpty())
            return NULL;

        result = new Network::Chunk( (const uint8*)msg.constData(), (const uint8*)msg.constData() + msg.size() );

        return result;
    }

    return NULL;
}

void CBRSST::service() {
    mApp->processEvents();
}

void CBRSST::reportQueueInfo(const Time& t) const {
    for(StreamMap::const_iterator it = mSendConnections.begin(); it != mSendConnections.end(); it++) {
        uint32 tx_size = it->second.stream->getTransmitBuffer();
        int tx_used_max;
        it->second.stream->getTransmitBufferUsed(NULL, NULL, &tx_used_max);
        uint32 tx_used = tx_used_max;

        uint32 rx_size = 0;
        uint32 rx_used = 0;
        StreamMap::const_iterator rx_it = mReceiveConnections.find(it->first);
        if (rx_it != mReceiveConnections.end()) {
            rx_size = rx_it->second.stream->getReceiveBuffer();
            int rx_used_max;
            rx_it->second.stream->getReceiveBufferUsed(NULL, NULL, &rx_used_max);
            rx_used = rx_used_max;
        }
        mTrace->packetQueueInfo(t, it->first, tx_size, tx_used, 0, rx_size, rx_used, 0);
    }
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
        //remote_ip = ntohl(remote_ip); // FIXME why is this reversed?
        remote_port = ntohs(remote_port); // but this isn't?

        //qDebug() << "remote ip: " << remote_ip << "  remote port: " << remote_port;

        //strm->setChildReceiveBuffer(sizeof(qint32));
        strm->listen(SST::Stream::Reject);

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

        SSTStatsListener* stats_listener = new SSTStatsListener(mTrace, mStartTime, remote_addy);
        strm->setStatListener( stats_listener );

        StreamInfo si;
        si.stream = strm;
        si.stats = stats_listener;
        si.currentSendChunk = NULL;
        si.peek = NULL;

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


CBRSST::StreamInfo* CBRSST::lookupOrConnectSend(const Address4& addy) {
    // If we have one, just return it
    StreamMap::iterator it = mSendConnections.find(addy);
    if (it != mSendConnections.end())
        return &(it->second);

    // Otherwise we need to connect
    SST::Stream* strm = new SST::Stream(mHost, this);
    quint32 addy_ip = htonl(addy.ip);
    strm->connectTo(SST::Ident::fromIpAddress(QHostAddress(addy_ip),addy.port).id(), SERVICE_NAME, PROTOCOL_NAME);

    SSTStatsListener* stats_listener = new SSTStatsListener(mTrace, mStartTime, addy);
    strm->setStatListener( stats_listener );

    StreamInfo si;
    si.stream = strm;
    si.stats = stats_listener;
    si.currentSendChunk = NULL;
    si.peek = NULL;

    mSendConnections[addy] = si;

    it = mSendConnections.find(addy);
    return &(it->second);
}

CBRSST::StreamInfo* CBRSST::lookupReceive(const Address4& addy) {
    // If we have one, just return it
    StreamMap::iterator it = mReceiveConnections.find(addy);
    if (it != mReceiveConnections.end())
        return &(it->second);
    return NULL;
}

} // namespace CBR
