#ifndef _CBR_ENET_NETWORK_HPP_
#define _CBR_ENET_NETWORK_HPP_

#include "Network.hpp"
#include <enet/enet.h>

namespace CBR {
class Trace;
class ENetNetwork :public Network{
    Trace*mTrace;
    ENetHost *mSendHost;
    ENetHost *mRecvHost;
    struct BufferSizeStats{
        size_t mSendSize;
        size_t mRecvSize;
        double mSendWeight;
        double mRecvWeight;
        BufferSizeStats(){mSendSize=mRecvSize=0;mSendWeight=mRecvWeight=0.0;}
    };
    std::tr1::unordered_map<Address4,BufferSizeStats > mBufferSizes;
    typedef std::tr1::unordered_map<Address4,Chunk*,Address4::Hasher> PeerFrontMap;
    PeerFrontMap mPeerFront;
    typedef std::tr1::unordered_map<Address4,std::vector<Chunk>,Address4::Hasher> PeerInitMap;
    PeerInitMap mPeerInit;
    typedef std::tr1::unordered_map<Address4,ENetPeer*,Address4::Hasher> PeerMap;
    PeerMap mSendPeers;
    PeerMap mRecvPeers;
    size_t mSendBufferSize;
    uint32 mIncomingBandwidth;
    uint32 mOutgoingBandwidth;
    bool connect(const Address4&addy);
    static ENetAddress toENetAddress(const Address4&addy);
    static Address4 fromENetAddress(const ENetAddress&addy);
    void processOutboundEvent(ENetEvent&event);
    bool internalSend(const Address4&,const Chunk&, bool reliable, bool ordered, int priority, bool force);
public:

    ENetNetwork(Trace* trace, size_t mPeerSendBufferSize, uint32 icomingBandwidth,uint32 outgoingBandwidth);
    virtual ~ENetNetwork();

    virtual void init(void*(*)(void*));
    // Called right before we start the simulation, useful for syncing network timing info to Time(0)
    virtual void start();

    // Checks if this chunk, when passed to send, would be successfully pushed.
    virtual bool canSend(const Address4&,const Chunk&, bool reliable, bool ordered, int priority);
    virtual bool send(const Address4&,const Chunk&, bool reliable, bool ordered, int priority);

    virtual void listen (const Address4&);
    virtual Chunk* front(const Address4& from, uint32 max_size);
    virtual Chunk* receiveOne(const Address4& from, uint32 max_size);
    virtual void service(const Time& t);

    virtual void reportQueueInfo(const Time& t) const;
};
}


#endif //_CBR_ENET_NETWORK_HPP_
