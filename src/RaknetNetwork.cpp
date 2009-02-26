#include "RaknetNetwork.hpp"
namespace CBR {
RaknetNetwork::RaknetNetwork ():mListener(RakNetworkFactory::GetRakPeerInterface()){
}

void RaknetNetwork::listen(const Sirikata::Network::Address&addy) {
    SocketDescriptor socketDescriptor(atoi(addy.getService().c_str()),0);
    mListener->Startup(32767,0,&socketDescriptor,1);
    mListener->SetOccasionalPing(true);
}

bool RaknetNetwork::sendTo(const Sirikata::Network::Address&addy, const Sirikata::Network::Chunk& toSend, bool reliable, bool ordered, int priority) {
    if (toSend.size()==0)
        return true;
    ConnectionMap::iterator where;
    std::tr1::shared_ptr<Connection> found;
   
    if ((where=mOutboundConnectionMap.find(addy))==mOutboundConnectionMap.end()) {
        std::tr1::shared_ptr<Connection> newcnx(new Connection);
        newcnx->mPeer=RakNetworkFactory::GetRakPeerInterface();
        newcnx->mPeer->Connect(addy.getHostName().c_str(),
                               atoi(addy.getService().c_str()),
                               "",
                               0);        
        mOutboundConnectionMap[addy]=found=newcnx;
    }else {
        found=where->second;
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
    Sirikata::Network::Chunk paddedToSend(1);
    paddedToSend[0]=ID_USER_PACKET_ENUM;
    paddedToSend.insert(paddedToSend.end(),toSend.begin(),toSend.end());
    PacketPriority pri=(priority==0?HIGH_PRIORITY:priority==1?MEDIUM_PRIORITY:LOW_PRIORITY);
    if (found->mToBeSent.size()||where==mOutboundConnectionMap.end()) {
        found->mToBeSent.resize(found->mToBeSent.size()+1);
        found->mToBeSent.back().first.swap(paddedToSend);
        found->mToBeSent.back().second.first=pri;
        found->mToBeSent.back().second.second=rel;
        return true;
    }else {    
        return found->mPeer->Send((const char*)&*paddedToSend.begin(),toSend.size()+1,pri,rel,0,UNASSIGNED_SYSTEM_ADDRESS,false);
    }
}
Sirikata::Network::Chunk*RaknetNetwork::makeChunk(RakPeerInterface*i,Packet*p) {
    
    Sirikata::Network::Chunk*retval=new Sirikata::Network::Chunk(0);
    retval->insert(retval->end(),p->data+1,p->data+p->length);
    i->DeallocatePacket(p);
    return retval;
}


Sirikata::Network::Chunk*RaknetNetwork::receiveOne() {
    
    for (SocketList::iterator i=mConnectingSockets.begin();i!=mConnectingSockets.end();) {
        Packet *p;
        while (true) {
            std::tr1::shared_ptr <Connection>conn(i->lock());
            if (conn) {
                p=conn->mPeer->Receive();
                if (p==NULL) break;
                unsigned char packetIdentifier = p->data[0];
                switch (packetIdentifier) {
                  case ID_ALREADY_CONNECTED:
                    fprintf (stderr,"Strange Already connected to target\n");
                    break;
                  case ID_REMOTE_NEW_INCOMING_CONNECTION: 
                    fprintf (stderr,"You can't talk to me\n");
                    break;
                  case ID_DISCONNECTION_NOTIFICATION:
                  case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server tel
                  case ID_REMOTE_CONNECTION_LOST: 
                  case ID_CONNECTION_BANNED:
                  case ID_CONNECTION_ATTEMPT_FAILED:
                  case ID_NO_FREE_INCOMING_CONNECTIONS:
                  case ID_MODIFIED_PACKET:
                    // Cheater!
                  case ID_INVALID_PASSWORD:
                  case ID_CONNECTION_LOST:
                      {
                          SocketList::iterator oldi=i;
                          ++i;                      
                          mConnectingSockets.erase(oldi);
                      }
                    continue;
                  case ID_CONNECTION_REQUEST_ACCEPTED:
                      {
                          for (size_t count=0,counte=conn->mToBeSent.size();count!=counte;++count) {
                              conn->mPeer->Send((const char*)&*conn->mToBeSent[count].first.begin(),
                                                conn->mToBeSent[count].first.size(),
                                                conn->mToBeSent[count].second.first,
                                                conn->mToBeSent[count].second.second,
                                                0,
                                                UNASSIGNED_SYSTEM_ADDRESS,
                                                true);
                          }
                      }
                    conn->mToBeSent.clear();
                    {
                        SocketList::iterator oldi=i;
                        ++i;
                        mConnectingSockets.erase(oldi);
                    }
                    continue;
                  default:
                    return makeChunk(conn->mPeer,p);                    
                }                
                ++i;
            }else{
                SocketList::iterator oldi=i;
                ++i;
                mConnectingSockets.erase(oldi);
            }
        }
    }
    Packet*p;
    while((p=mListener->Receive())) {
        unsigned char packetIdentifier = p->data[0];
        switch (packetIdentifier) {
          case ID_ALREADY_CONNECTED:
            fprintf (stderr,"Strange\n");
            break;
          case ID_REMOTE_NEW_INCOMING_CONNECTION: 
            fprintf (stderr,"New peers\n");
            break;
          case ID_DISCONNECTION_NOTIFICATION:
          case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server tel
          case ID_REMOTE_CONNECTION_LOST: 
          case ID_CONNECTION_BANNED:
          case ID_CONNECTION_ATTEMPT_FAILED:
          case ID_NO_FREE_INCOMING_CONNECTIONS:
          case ID_MODIFIED_PACKET:
            // Cheater!
          case ID_INVALID_PASSWORD:
          case ID_CONNECTION_LOST:
            fprintf (stderr,"Misc error\n");
            break;
          case ID_CONNECTION_REQUEST_ACCEPTED:
            fprintf (stderr,"Impossible connection request accepted\n");
            break;
          default:
            return makeChunk(&*mListener,p);
        }
    }
    return  NULL;
}


}
