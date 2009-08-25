#include "ENetNetwork.hpp"
#include "Statistics.hpp"
namespace CBR {

#define MAGIC_PORT_OFFSET 8192
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
    mSendHost=NULL;
    mRecvHost=NULL;
}
ENetNetwork::~ENetNetwork(){
    for (PeerFrontMap::iterator i=mPeerFront.begin(),ie=mPeerFront.end();i!=ie;++i) {
        delete i->second;
    }
    if (mRecvHost) {
        for (PeerMap::iterator i=mSendPeers.begin(),ie=mSendPeers.end();i!=ie;++i) {
            enet_peer_disconnect(i->second,0);
        }
        for (PeerMap::iterator i=mRecvPeers.begin(),ie=mRecvPeers.end();i!=ie;++i) {
            enet_peer_disconnect(i->second,0);
        }
        enet_host_flush(mSendHost);
        enet_host_destroy(mSendHost);
        enet_host_flush(mRecvHost);
        enet_host_destroy(mRecvHost);
        mSendHost=NULL;
        mRecvHost=NULL;
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
        return true;
    }
    size_t totalSize=0;
    PeerMap::iterator where=mSendPeers.find(addy);
    if (where!=mSendPeers.end()) {
        totalSize=enet_peer_send_buffer_size(where->second)+dat.size();
        if (totalSize<=mSendBufferSize) {
            return true;
        }
    }else {//no peer initialized--init communication buffer can hold one packet
        return true;
    }
    //printf ("Cant send to %d [%d < %d]\n",addy.port,(int)totalSize,(int)mSendBufferSize);
    return false;
}

bool ENetNetwork::connect(const Address4&addy) {
    ENetAddress address=toENetAddress(addy);
    ENetPeer*peer=enet_host_connect(mSendHost,&address,31);
    if (peer) {
        mSendPeers[addy]=peer;
        return true;
    }
    return false;
}

bool ENetNetwork::internalSend(const Address4&addy,const Chunk&dat, bool reliable, bool ordered, int priority, bool force){
    if (mPeerInit.find(addy)!=mPeerInit.end()) {
        mPeerInit[addy].push_back(dat);
        return true;
    }
    PeerMap::iterator where=mSendPeers.find(addy);
    if (where!=mSendPeers.end()) {
        size_t esend_buffer_size=enet_peer_send_buffer_size(where->second);
        size_t totalSize=esend_buffer_size+dat.size();
        if (totalSize<=mSendBufferSize||esend_buffer_size==0||force) {
            ENetPacket *pkt=enet_packet_create(dat.empty()?NULL:&dat[0],dat.size(),((reliable?ENET_PACKET_FLAG_RELIABLE:0)|(ordered?0:ENET_PACKET_FLAG_UNSEQUENCED)));
            int retval=(enet_peer_send(where->second,1,pkt));
            if (retval!=0) {
                printf ("Failure\n");
            }
            return retval==0;
        }else {
            printf ("Failed to have buffer room %d %d \n",(int)esend_buffer_size,(int)totalSize);
        }
    }else {
        //connect
        mPeerInit[addy].push_back(dat);
        return connect(addy);
    }
    return false;
}
bool ENetNetwork::send(const Address4&addy,const Chunk&dat, bool reliable, bool ordered, int priority){
    return internalSend(addy,dat,reliable,ordered,priority,false);
}
void ENetNetwork::listen (const Address4&addy){
    ENetAddress address=toENetAddress(addy);
    mRecvHost=enet_host_create(&address,254, mIncomingBandwidth, mOutgoingBandwidth);
    address.port+=MAGIC_PORT_OFFSET;
    mSendHost=enet_host_create(&address,254, mIncomingBandwidth, mOutgoingBandwidth);

}
Network::Chunk* ENetNetwork::front(const Address4& from, uint32 max_size){
    Network::Chunk **retval=&mPeerFront[from];
    if (*retval)
        return *retval;
    PeerMap::iterator where=mRecvPeers.find(from);
    if (where!=mRecvPeers.end()) {
        ENetEvent event;
        if (enet_peer_check_events(mRecvHost,where->second,&event)) {
            switch (event.type) {
              case ENET_EVENT_TYPE_NONE:
                printf("None event\n");
                break;
              case ENET_EVENT_TYPE_RECEIVE:
                  {
                      ENetPacket* pkt=event.packet;
                      if (pkt) {
                          (*retval)=new Network::Chunk ((unsigned char*)pkt->data,((unsigned char*)pkt->data)+pkt->dataLength);
                          enet_packet_destroy(pkt);
                          if ((*retval)->size()<=max_size)
                              return *retval;
                          return NULL;
                      }
                  }
                  break;
              case ENET_EVENT_TYPE_CONNECT:
              case ENET_EVENT_TYPE_DISCONNECT:
                processOutboundEvent(event);
                break;
            }
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
void ENetNetwork::processOutboundEvent(ENetEvent&event) {
    switch (event.type) {
      case ENET_EVENT_TYPE_NONE:
        printf("None event\n");
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        assert(0);
        break;
      case ENET_EVENT_TYPE_CONNECT:
        printf ("Connect event %d\n",event.peer->address.port);
          {
              Address4 addy=fromENetAddress(event.peer->address);
              PeerMap::iterator where=mSendPeers.find(addy);
              if (where!=mSendPeers.end()&&where->second==event.peer) {
                  PeerInitMap::iterator datawhere=mPeerInit.find(addy);
                  if (datawhere!=mPeerInit.end()) {
                      std::vector<Network::Chunk> datachunks;
                      datachunks.swap(datawhere->second);
                      mPeerInit.erase(datawhere);
                      for (std::vector<Network::Chunk>::iterator i=datachunks.begin(),ie=datachunks.end();i!=ie;++i) {
                          internalSend(addy,*i,true,true,1,true);
                      }
                      printf("send connect event %d\n",(int)datachunks.size());
                  }
              }else {
                  printf("recv connect event\n");
                  addy.port-=MAGIC_PORT_OFFSET;
                  mRecvPeers[addy]=event.peer;
              }
              break;
          }
          break;
      case ENET_EVENT_TYPE_DISCONNECT:
        printf ("DisConnect event %d\n",event.peer->address.port);
          {
              PeerMap::iterator where=mRecvPeers.find(fromENetAddress(event.peer->address));
              if (where!=mRecvPeers.end()) {
                  mRecvPeers.erase(where);
              }
          }
          break;
    }
}
void ENetNetwork::service(const Time& t){
    PeerMap::iterator senditer,recviter;
    std::vector<std::pair<Address4,size_t> > sendBufferSizes(mSendPeers.size());
    std::vector<std::pair<Address4,size_t> > recvBufferSizes(mRecvPeers.size());
    senditer=mSendPeers.begin();
    recviter=mRecvPeers.begin();
    for (size_t i=0,ie=mSendPeers.size();i!=ie;++i,++senditer) {
        sendBufferSizes[i].first=senditer->first;
        sendBufferSizes[i].second=senditer->second->outgoingReliableCommands.byte_size+senditer->second->outgoingUnreliableCommands.byte_size;
    }
    for (size_t i=0,ie=mRecvPeers.size();i!=ie;++i,++recviter) {
        recvBufferSizes[i].first=recviter->first;
        recvBufferSizes[i].second=0;
        for (unsigned int j=0;j<recviter->second->channelCount;++j) {
            recvBufferSizes[i].second+=recviter->second->channels[j].incomingReliableCommands.byte_size+recviter->second->channels[j].incomingUnreliableCommands.byte_size;
        }
    }
    do {
        ENetEvent event;
        if (enet_host_service_one_outbound (mRecvHost, & event))
            processOutboundEvent(event);

        if (enet_host_service (mSendHost, & event,0))
            processOutboundEvent(event);
    }while (Time::now()<t);


    senditer=mSendPeers.begin();
    recviter=mRecvPeers.begin();

    for (size_t i=0,ie=sendBufferSizes.size();i!=ie;++i) {
        senditer=mSendPeers.find(sendBufferSizes[i].first);
        if (senditer!=mSendPeers.end()) {
            ptrdiff_t diff=sendBufferSizes[i].second-(senditer->second->outgoingReliableCommands.byte_size+senditer->second->outgoingUnreliableCommands.byte_size);
            if (diff>0) {
                mTrace->packetSent(t,sendBufferSizes[i].first,diff);
            }
        }
    }
    for (size_t i=0,ie=recvBufferSizes.size();i!=ie;++i) {
        size_t curincoming=0;
        recviter=mRecvPeers.find(recvBufferSizes[i].first);
        if (recviter!=mRecvPeers.end()) {
            for (unsigned int j=0;j<recviter->second->channelCount;++j) {
                curincoming+=recviter->second->channels[j].incomingReliableCommands.byte_size+recviter->second->channels[j].incomingUnreliableCommands.byte_size;
            }
            ptrdiff_t diff=curincoming-recvBufferSizes[i].second;
            if (diff>0) {
                mTrace->packetReceived(t,recvBufferSizes[i].first,diff);
            }
        }

    }
}

void ENetNetwork::reportQueueInfo(const Time& t) const{

}
}
