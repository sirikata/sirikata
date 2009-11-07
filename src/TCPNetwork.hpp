#ifndef _CBR_TCP_NETWORK_HPP_
#define _CBR_TCP_NETWORK_HPP_

#include "Network.hpp"
#include "sirikata/network/Stream.hpp"
#include "sirikata/util/PluginManager.hpp"
#include "sirikata/util/SizedThreadSafeQueue.hpp"
namespace Sirikata { namespace Network {
class StreamListener;
} }
namespace CBR {
class Trace;
class TCPNetwork :public Network{
    struct SendStream {
        Sirikata::Network::Stream* stream;
        bool connected;
        SendStream(Sirikata::Network::Stream* stream) {
            this->stream=stream;
            connected=false;
        }
    };
    typedef std::tr1::unordered_map<Address4, SendStream,Address4::Hasher> TCPStreamMap;
    Sirikata::Network::StreamListener *mListener;
    TCPStreamMap mSendStreams;
    uint32 mIncomingBufferLength;
    uint32 mIncomingBandwidth;
    uint32 mOutgoingBandwidth;
    Sirikata::PluginManager mPluginManager;
    Sirikata::Network::IOService *mIOService;
    Thread* mThread;
    void processNewConnectionsOnMainThread();
    class TSQueue {
    public:
        Sirikata::Network::Stream *stream;
        Chunk *front;
        ///FIXME this queue should be LOCK FREE! you can doo it
        Sirikata::SizedThreadSafeQueue<Chunk*,Sirikata::SizedPointerResourceMonitor> buffer;
        bool paused;//this is true if someone paused a stream, and the stream must be unpaused the next time some bozo calls receiveOne
        TSQueue (TCPNetwork*parent, Sirikata::Network::Stream*strm) :buffer(Sirikata::SizedPointerResourceMonitor(parent->mIncomingBufferLength)) {
            front=NULL;
            stream=strm;
            paused=false;
        }
    };
    typedef std::tr1::shared_ptr<std::tr1::shared_ptr<TSQueue> > dbl_ptr_queue;
    typedef std::tr1::weak_ptr<std::tr1::shared_ptr<TSQueue> > weak_dbl_ptr_queue;
    Sirikata::ThreadSafeQueue<Address4> mDisconnectedStreams;
    Sirikata::ThreadSafeQueue<Address4> mNewConnectedStreams;
    Sirikata::Network::Stream::ReceivedResponse bytesReceivedCallback(const weak_dbl_ptr_queue&queue, Chunk&data);
    void receivedConnectionCallback(const weak_dbl_ptr_queue&queue, const Sirikata::Network::Stream::ConnectionStatus, const std::string&reason);
    void readySendCallback(const Address4&);
    void sendStreamConnectionCallback(const Address4&, const Sirikata::Network::Stream::ConnectionStatus, const std::string&reason);
    void newStreamCallback(Sirikata::Network::Stream*, Sirikata::Network::Stream::SetCallbacks&cb);
    std::tr1::unordered_map<Address4,dbl_ptr_queue,Address4::Hasher>mReceiveBuffers;
    Sirikata::ThreadSafeQueue<std::pair<Address4,dbl_ptr_queue> > mNewReceiveBuffers;
    std::tr1::shared_ptr<TSQueue> getQueue(const Address4&);
    Sirikata::OptionSet*mListenOptions;
    Sirikata::OptionSet*mSendOptions;
    String mStreamPlugin;
public:

    TCPNetwork(SpaceContext* ctx, uint32 incomingBufferLength, uint32 icomingBandwidth,uint32 outgoingBandwidth);
    virtual ~TCPNetwork();

    virtual void init(void*(*)(void*));
    // Called right before we start the simulation, useful for syncing network timing info to Time(0)
    virtual void begin();

    // Checks if this chunk, when passed to send, would be successfully pushed.
    virtual bool canSend(const Address4&,uint32 size, bool reliable, bool ordered, int priority);
    virtual bool send(const Address4&,const Chunk&, bool reliable, bool ordered, int priority);

    virtual void listen (const Address4&);
    virtual Chunk* front(const Address4& from, uint32 max_size);
    virtual Chunk* receiveOne(const Address4& from, uint32 max_size);
    virtual void reportQueueInfo() const;
};
}


#endif //_CBR_TCP_NETWORK_HPP_
