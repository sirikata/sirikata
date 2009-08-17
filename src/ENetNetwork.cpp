#include "Network.hpp"
#include "ENetNetwork.hpp"
namespace CBR {
ENetAddress ENetNetwork::toENetAddress(const Address4&addy){
    ENetAddress retval;
    retval.host=addy.ip;
    retval.port=addy.port;
    return retval;
}
Address4 ENetNetwork::fromENetAddress(const ENetAddress&addy){
    Address4 retval;
    retval.ip=addy.host;
    retval.port=addy.port;
    return retval;
}
ENetNetwork::ENetNetwork(Trace* trace, size_t peerSendBufferSize, uint32 incomingBandwidth, uint32 outgoingBandwidth){
    mIncomingBandwidth=incomingBandwidth;
    mOutgoingBandwidth=outgoingBandwidth;
    mSendBufferSize=peerSendBufferSize;
    mTrace=trace;
    mHost=NULL;
}
ENetNetwork::~ENetNetwork(){
    for (PeerFrontMap::iterator i=mPeerFront.begin(),ie=mPeerFront.end();i!=ie;++i) {
        delete i->second;
    }
    if (mHost) {
        for (PeerMap::iterator i=mSendPeers.begin(),ie=mSendPeers.end();i!=ie;++i) {
            enet_peer_disconnect(i->second,0);
        }
        for (PeerMap::iterator i=mRecvPeers.begin(),ie=mRecvPeers.end();i!=ie;++i) {
            enet_peer_disconnect(i->second,0);
        }
        enet_host_flush(mHost); 
        enet_host_destroy(mHost);
        mHost=NULL;
    }
}

void ENetNetwork::init(void*(*fucn)(void*)){
    void *retval=(*fucn)(NULL);
}
    // Called right before we start the simulation, useful for syncing network timing info to Time(0)
void ENetNetwork::start(){

}

    // Checks if this chunk, when passed to send, would be successfully pushed.
bool ENetNetwork::canSend(const Address4&addy,const Chunk&dat, bool reliable, bool ordered, int priority){
    if (mPeerInit.find(addy)!=mPeerInit.end()) {
        return false;
    }
    PeerMap::iterator where=mSendPeers.find(addy);
    if (where!=mSendPeers.end()) {
        size_t totalSize=enet_peer_send_buffer_size(where->second)+dat.size();
        if (totalSize<=mSendBufferSize) {
            return true;
        }
    }else {//no peer initialized--init communication buffer can hold one packet
        return true;
    }
    return false;
}

bool ENetNetwork::connect(const Address4&addy) {
    ENetAddress address=toENetAddress(addy);
    ENetPeer*peer=enet_host_connect(mHost,&address,2);
    if (peer) {
        mSendPeers[addy]=peer;
        return true;
    }
    return false;
}

bool ENetNetwork::send(const Address4&addy,const Chunk&dat, bool reliable, bool ordered, int priority){
    if (mPeerInit.find(addy)!=mPeerInit.end()) {
        return false;
    }
    PeerMap::iterator where=mSendPeers.find(addy);
    if (where!=mSendPeers.end()) {
        size_t totalSize=enet_peer_send_buffer_size(where->second)+dat.size();
        if (totalSize<=mSendBufferSize) {
            ENetPacket *pkt=enet_packet_create(dat.empty()?NULL:&dat[0],dat.size(),((reliable?ENET_PACKET_FLAG_RELIABLE:0)|(ordered?0:ENET_PACKET_FLAG_UNSEQUENCED)));
            return (enet_peer_send(where->second,1,pkt))==0;
        }
    }else {
        //connect
        mPeerInit[addy]=dat;
        connect(addy);
    }
    return false;
}
void ENetNetwork::listen (const Address4&addy){
    ENetAddress address=toENetAddress(addy);   
    mHost=enet_host_create(&address,16383, mIncomingBandwidth, mOutgoingBandwidth);

}
Network::Chunk* ENetNetwork::front(const Address4& from, uint32 max_size){
    Network::Chunk **retval=&mPeerFront[from];
    if (*retval)
        return *retval;
    PeerMap::iterator where=mRecvPeers.find(from);
    if (where!=mRecvPeers.end()) {
        ENetPacket* pkt=enet_peer_receive (where->second, 1);
        if (pkt) {
            (*retval)=new Network::Chunk ((unsigned char*)pkt->data,((unsigned char*)pkt->data)+pkt->dataLength);
            enet_packet_destroy(pkt);
            if ((*retval)->size()<=max_size)
                return *retval;
            return NULL;
        }
    }
    return NULL;
}
Network::Chunk* ENetNetwork::receiveOne(const Address4& from, uint32 max_size){
    Network::Chunk*tmp=front(from,max_size);
    if (tmp) {
        mPeerFront.find(from)->second=NULL;
    }
    return tmp;    
}
void ENetNetwork::service(const Time& t){
    do {
        ENetEvent event;
        enet_host_service_one_outbound (mHost, & event);
        switch (event.type) {
          case ENET_EVENT_TYPE_NONE:
            break;
          case ENET_EVENT_TYPE_RECEIVE:
            assert(0);
            break;
          case ENET_EVENT_TYPE_CONNECT:
            {
                Address4 addy=fromENetAddress(event.peer->address);
                PeerMap::iterator where=mSendPeers.find(addy);
                if (where!=mSendPeers.end()&&where->second==event.peer) {
                    PeerInitMap::iterator datawhere=mPeerInit.find(addy);
                    if (datawhere!=mPeerInit.end()) {
                        Network::Chunk datachunk;
                        datachunk.swap(datawhere->second);
                        mPeerInit.erase(datawhere);
                        send(addy,datachunk,true,true,1);
                    }
                }else {
                    mRecvPeers[addy]=event.peer;
                }
                break;
            }
            break;
          case ENET_EVENT_TYPE_DISCONNECT:
            {
                PeerMap::iterator where=mRecvPeers.find(fromENetAddress(event.peer->address));
                if (where!=mRecvPeers.end()) {
                    mRecvPeers.erase(where);
                }
            }
            break;
        }
    }while (Time::now()<t);
}

void ENetNetwork::reportQueueInfo(const Time& t) const{

}
}

