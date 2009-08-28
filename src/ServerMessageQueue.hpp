#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "ServerIDMap.hpp"

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
    ServerMessageQueue(SpaceContext* ctx, Network* net, ServerIDMap* sidmap)
     : mContext(ctx),
       mNetwork(net),
       mServerIDMap(sidmap)
    {
        // start the network listening
        Address4* listen_addy = mServerIDMap->lookup(mContext->id);
        assert(listen_addy != NULL);
        net->listen(*listen_addy);
    }

    virtual ~ServerMessageQueue(){}
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out) = 0;

    virtual void service() = 0;

    const SpaceContext* context() const { return mContext; }

    virtual void setServerWeight(ServerID sid, float weight) = 0;

    virtual void reportQueueInfo(const Time& t) const = 0;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const = 0;

protected:
    SpaceContext* mContext;
    Network* mNetwork;
    ServerIDMap* mServerIDMap;
};
}

#endif
