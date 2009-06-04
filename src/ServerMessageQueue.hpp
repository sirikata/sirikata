#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "ServerIDMap.hpp"
#include "Statistics.hpp"

namespace CBR{

typedef struct QueueInfo{
  uint32 mTXSize;
  uint32 mTXUsed;
  float mTXWeight;
  
  uint32 mRXSize;
  uint32 mRXUsed;
  float mRXWeight;

  QueueInfo(uint32 tx_size, uint32 tx_used, float tx_weight, 
	    uint32 rx_size, uint32 rx_used, float rx_weight
	   )
  {
    mTXSize = tx_size;
    mTXUsed = tx_used;
    mTXWeight = tx_weight;

    mRXSize = rx_size;
    mRXUsed = rx_used;
    mRXWeight = rx_weight;
  }
  

} QueueInfo;

class ServerMessageQueue {
public:
    ServerMessageQueue(Network* net, const ServerID& sid, ServerIDMap* sidmap, Trace* trace)
     : mNetwork(net),
       mSourceServer(sid),
       mServerIDMap(sidmap),
       mTrace(trace)
    {
        // start the network listening
        Address4* listen_addy = mServerIDMap->lookup(mSourceServer);
        assert(listen_addy != NULL);
        net->listen(*listen_addy);
    }

    virtual ~ServerMessageQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out) = 0;
    virtual void service(const Time& t)=0;

    virtual void setServerWeight(ServerID sid, float weight) = 0;
    ServerID getSourceServer()const{return mSourceServer;}

    virtual void reportQueueInfo(const Time& t) const = 0;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const = 0;

protected:
    Network* mNetwork;
    ServerID mSourceServer;
    ServerIDMap* mServerIDMap;
    Trace* mTrace;
};
}

#endif
