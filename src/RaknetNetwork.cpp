#include "RaknetNetwork.hpp"
#include "ServerIDMap.hpp"

       #include <sys/socket.h>
       #include <netinet/in.h>
       #include <arpa/inet.h>

namespace CBR {
RaknetNetwork::RaknetNetwork ()
 : Network(),
   mListener(RakNetworkFactory::GetRakPeerInterface()),
   mOutstandingPacket(NULL)
{
}
void RaknetNetwork::init(void*(*x)(void*)){
        (*x)(NULL);
}
void RaknetNetwork::start() {
    // do nothing
}
void RaknetNetwork::listen(const Address4& as_server) {
    SocketDescriptor socketDescriptor(as_server.getPort(),0);
    bool starting=mListener->Startup(16383,30,&socketDescriptor,1);
       mListener->SetMaximumIncomingConnections(16383);
    assert(starting);
    mListener->SetOccasionalPing(true);
}
Address4 MakeAddress(const SystemAddress &address) {
    return Address4(address.binaryAddress,address.port);
}
SystemAddress MakeAddress(const Address4 &address) {
    SystemAddress sa;
    sa.binaryAddress=address.ip;
    sa.port=address.port;
    return sa;
}
void ConnectTo(RakPeerInterface*ri,const SystemAddress &sa) {
    struct in_addr temp;
    temp.s_addr=sa.binaryAddress;
    std::string hostname(inet_ntoa(temp));
    bool evenTried=ri->Connect(hostname.c_str(),
                                      sa.port,
                                      0,
                                      0);
    assert(evenTried);
}
bool RaknetNetwork::send(const Address4&addy, const Sirikata::Network::Chunk& toSend, bool reliable, bool ordered, int priority) {
    if (toSend.size()==0)
        return false;
    SystemAddress sa=MakeAddress(addy);
    if (mConnectingSockets.find(addy)==mConnectingSockets.end()&&!mListener->IsConnected(sa,true,false)) {
        ConnectTo(mListener,sa);
    }
    PacketReliability rel;
    if (ordered==true&&reliable==true) {
        rel=RELIABLE_ORDERED;
    }
    if (ordered==false&&reliable==true){
        rel=RELIABLE;
    }
    if (ordered==false&&reliable==false){
        rel=UNRELIABLE;
    }
    if (ordered==true&&reliable==false){
        rel=UNRELIABLE_SEQUENCED;
    }
    PacketPriority pri=(priority==0?HIGH_PRIORITY:(priority==1?MEDIUM_PRIORITY:LOW_PRIORITY));
    Sirikata::Network::Chunk paddedToSend(1);
    paddedToSend[0]=ID_USER_PACKET_ENUM;
    paddedToSend.insert(paddedToSend.end(),toSend.begin(),toSend.end());
    if (!mListener->IsConnected(sa,false,false)) {
        SentQueue::SendQueue* tosend=&mConnectingSockets[addy].mToBeSent;
        tosend->resize(tosend->size()+1);
        tosend->back().first=paddedToSend;
        tosend->back().second.first=pri;
        tosend->back().second.second=rel;
        return true;
    }else {
        assert(MakeAddress(sa)==addy);
        sendRemainingItems(sa);
        return mListener->Send((const char*)&*paddedToSend.begin(),toSend.size()+1,pri,rel,0,sa,false);
    }
}
Sirikata::Network::Chunk*RaknetNetwork::makeChunk(RakPeerInterface*i,Packet*p) {
    Sirikata::Network::Chunk*retval=new Sirikata::Network::Chunk(0);
    retval->insert(retval->end(),p->data+1,p->data+p->length);
    return retval;
}
bool RaknetNetwork::sendRemainingItems(SystemAddress address) {
    Address4 a=MakeAddress(address);
    ConnectingMap::iterator where=mConnectingSockets.find(a);
    if (where!=mConnectingSockets.end()) {
        SentQueue::SendQueue v;
        v.swap(where->second.mToBeSent);
        mConnectingSockets.erase(where);
        for (SentQueue::SendQueue::iterator i=v.begin(),ie=v.end();i!=ie;++i) {
            bool retval=mListener->Send((const char*)&*i->first.begin(),i->first.size(),i->second.first,i->second.second,0,address,false);
            assert(retval);
        }
        return true;
    }
    return false;
}

Network::Chunk* RaknetNetwork::front(const Address4& from, uint32 max_size) {
    assert(from != Address4::Null);

    Queue<Chunk*>* q = getReceiveQueue(from);
    if (q == NULL || q->empty() || q->front()->size() > max_size) return NULL;
    return q->front();
}

Sirikata::Network::Chunk*RaknetNetwork::receiveOne(const Address4& from, uint32 max_size) {
    assert(from != Address4::Null);

    Queue<Chunk*>* q = getReceiveQueue(from);
    if (q == NULL || q->empty() || q->front()->size() > max_size) return NULL;
    return q->pop();
}

Packet* RaknetNetwork::nextPacket() {
    if (mOutstandingPacket != NULL) {
        Packet* result = mOutstandingPacket;
        mOutstandingPacket = NULL;
        return result;
    }

    return mListener->Receive();
}

Queue<Network::Chunk*>* RaknetNetwork::getReceiveQueue(const Address4& addr) {
    ReceiveQueueMap::iterator it = mReceiveQueues.find(addr);

    if (it == mReceiveQueues.end()) {
        Queue<Chunk*>* new_queue = new Queue<Chunk*>(64*1024); //FIXME constant
        mReceiveQueues[addr] = new_queue;
        return new_queue;
    }

    return it->second;
}

void RaknetNetwork::service(const Time& t) {
    Packet* p = NULL;
    while ((p = nextPacket())) {
        unsigned char packetIdentifier = p->data[0];
        //fprintf (stderr,"GOT SOMETHING!!!%d\n",packetIdentifier);
        switch (packetIdentifier) {
          case ID_ALREADY_CONNECTED:
            sendRemainingItems(p->systemAddress);
            break;
          case ID_NEW_INCOMING_CONNECTION:
          case ID_REMOTE_NEW_INCOMING_CONNECTION:
            sendRemainingItems(p->systemAddress);
            break;
          case ID_NO_FREE_INCOMING_CONNECTIONS:
            fprintf (stderr,"NO FREE INCOMING CONNECTIONS:Retrying\n");
          case ID_DISCONNECTION_NOTIFICATION:
          case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server tel
          case ID_REMOTE_CONNECTION_LOST:
          case ID_CONNECTION_BANNED:
          case ID_CONNECTION_ATTEMPT_FAILED:
          case ID_MODIFIED_PACKET:
            // Cheater!

          case ID_INVALID_PASSWORD:
          case ID_CONNECTION_LOST:
            if (mConnectingSockets.find(MakeAddress(p->systemAddress))!=mConnectingSockets.end()) {
                ConnectTo(mListener,p->systemAddress);
            }
            break;
          case ID_CONNECTION_REQUEST_ACCEPTED:
            sendRemainingItems(p->systemAddress);
            break;
          case ID_USER_PACKET_ENUM:
              {
                  Address4 sourceAddr = MakeAddress(p->systemAddress);
                  Chunk* rchunk = makeChunk(mListener,p);
                  // try to insert the chunk
                  Queue<Chunk*>* rqueue = getReceiveQueue(sourceAddr);
                  if (rqueue->push(rchunk) == QueueEnum::PushSucceeded) { // push succeeded, so we can just proceed normally
                  }
                  else { // we couldn't handle this packet, so we need to stop processing and try again later
                      delete rchunk;
                      mOutstandingPacket = p;
                      return;
                  }
              }
              break;
          default:
            fprintf(stderr,"New unknown error message: %d\n",ID_USER_PACKET_ENUM);
            break;
        }

        mListener->DeallocatePacket(p);
    }
}

}
