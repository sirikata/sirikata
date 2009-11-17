#ifndef _CBR_TCP_NETWORK_HPP_
#define _CBR_TCP_NETWORK_HPP_

#include "Network.hpp"
#include <sirikata/network/Stream.hpp>
#include <sirikata/network/StreamListener.hpp>
#include <sirikata/util/PluginManager.hpp>
#include <sirikata/util/SizedThreadSafeQueue.hpp>

namespace CBR {

class TCPNetwork : public Network {
    // Streams for sending data to remote nodes
    struct SendStream {
        Sirikata::Network::Stream* stream;
        bool connected;

        SendStream(Sirikata::Network::Stream* stream);
    };

    typedef std::tr1::unordered_map<Address4, SendStream, Address4::Hasher> TCPStreamMap;

    // Streams for receiving data from the remote nodes
    class TSQueue {
    public:
        Sirikata::Network::Stream *stream;
        Chunk *front;
        Sirikata::SizedThreadSafeQueue<Chunk*,Sirikata::SizedPointerResourceMonitor> buffer;
        bool inserted; // whether this connection got the initial info so it could be inserted into the main map
        bool paused;//this is true if someone paused a stream, and the stream must be unpaused the next time some bozo calls receiveOne
        TSQueue (TCPNetwork*parent, Sirikata::Network::Stream*strm);
    };

    typedef std::tr1::shared_ptr<TSQueue> ptr_queue;
    typedef std::tr1::shared_ptr<ptr_queue> dbl_ptr_queue;
    typedef std::tr1::weak_ptr<ptr_queue> weak_dbl_ptr_queue;
    typedef std::tr1::unordered_map<Address4, dbl_ptr_queue, Address4::Hasher> ReceiveMap;


    // All the real data is handled in the main thread.
    Sirikata::PluginManager mPluginManager;
    String mStreamPlugin;
    Sirikata::OptionSet* mListenOptions;
    Sirikata::OptionSet* mSendOptions;

    IOService *mIOService;
    IOWork* mIOWork;
    Thread* mThread;

    Address4 mListenAddress;

    uint32 mIncomingBufferLength;
    uint32 mIncomingBandwidth;
    uint32 mOutgoingBandwidth;

    Sirikata::Network::StreamListener *mListener;
    TCPStreamMap mSendStreams;

    ReceiveMap mReceiveBuffers;


    // Main Thread/Strand Methods, allowed to access all the core data structures.  These are mainly utility methods
    // posted by the IO thread.

    // Marks a send stream as connected
    void markSendConnected(const Address4& addr);
    // Mark a send stream as disconnected
    void markSendDisconnected(const Address4& addr);

    // Handles the identification of a receive stream as a specific endpoint, adding it to the list of full connections
    void addNewStream(const Address4& remote_endpoint, dbl_ptr_queue source_queue);

    ptr_queue getQueue(const Address4&);


    // IO Thread/Strand.  They must be certain not to call any main thread methods or access any main thread data.
    // These are all callbacks from the network, so mostly they should just be posting the results to the main thread.
    Sirikata::Network::Stream::ReceivedResponse bytesReceivedCallback(const weak_dbl_ptr_queue&queue, Chunk&data);

    void receivedConnectionCallback(const weak_dbl_ptr_queue&queue, const Sirikata::Network::Stream::ConnectionStatus, const std::string&reason);
    void readySendCallback(const Address4&);
    void sendStreamConnectionCallback(const Address4&, const Sirikata::Network::Stream::ConnectionStatus, const std::string&reason);
    void newStreamCallback(Sirikata::Network::Stream*, Sirikata::Network::Stream::SetCallbacks&cb);

    ReceiveMap mPendingReceiveBuffers; // Waiting for initial message specifying remote endpoint id

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
