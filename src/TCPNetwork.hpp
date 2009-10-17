#ifndef _CBR_TCP_NETWORK_HPP_
#define _CBR_TCP_NETWORK_HPP_

#include "Network.hpp"
#include <enet/enet.h>
##include "util/PluginManager.hpp"
namespace CBR {
class Trace;
class TCPNetwork :public Network{
    Trace*mTrace;
    typedef std::tr1::unordered_map<Address4, Sirikata::Stream*> TCPStreamMap;
    StreamListener *mListener;
    TCPStreamMap mSendStreams;
    TCPStreamMap mReceiveStreams;
    uint32 mIncomingBufferLength;
    uint32 mIncomingBandwidth;
    uint32 mOutgoingBandwidth;
    Sirikata::PluginManager mPluginManager;
    Sirikata::IOService *mIOService;
    class TSQueue {
    public:
        Sirikata::Network::Stream *stream;
        ///FIXME this queue should be LOCK FREE! you can doo it
        SizedThreadSafeQueue<Chunk*,SizedPointerResourceMonitor> buffer;
        bool paused;//this is true if someone paused a stream, and the stream must be unpaused the next time some bozo calls receiveOne
        TSQueue (TCPNetwork*parent, Sirikata::Network::Stream*strm) :buffer(SizedPointerResourceMonitor(parent->mIncomingBufferLength)) {
            stream=strm;
            paused=false;
        }
    };
    void newStreamCallback(Sirikata::Network::Stream*, Sirikata::Network::Stream::SetCallbacks&cb);
    enum {
        MAX_RECIEVE_STREAMS=8192;
    };
    std::tr1::shared_ptr<TSQueue>mReceiveBuffers[MAX_RECEIVE_STREAMS];    
    unsigned int mCurNumStreams;
    ThreadSafeQueue<std::pair<Address4,unsigned int> > mNewReceiveBuffers;
    ThreadSafeQueue<unsigned int > mFreeReceiveBuffers;
    
    std::tr1::unordered_map<Address4, unsigned int> mReceiveBufferMapping;
    std::tr1::shared_ptr<TSQueue>getSharedQueue(const Address4&);
public:

    TCPNetwork(Trace* trace, uint32 incomingBufferLength, uint32 icomingBandwidth,uint32 outgoingBandwidth);
    virtual ~TCPNetwork();

    virtual void init(void*(*)(void*));
    // Called right before we start the simulation, useful for syncing network timing info to Time(0)
    virtual void start();

    // Checks if this chunk, when passed to send, would be successfully pushed.
    virtual bool canSend(const Address4&,uint32 size, bool reliable, bool ordered, int priority);
    virtual bool send(const Address4&,const Chunk&, bool reliable, bool ordered, int priority);

    virtual void listen (const Address4&);
    virtual Chunk* front(const Address4& from, uint32 max_size);
    virtual Chunk* receiveOne(const Address4& from, uint32 max_size);
    virtual void service(const Time& t);
    virtual void reportQueueInfo(const Time& t) const;
};
}


#endif //_CBR_TCP_NETWORK_HPP_
