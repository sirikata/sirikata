#ifndef _CBR_SENDQUEUE_HPP
#define _CBR_SENDQUEUE_HPP

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"
#include "ServerNetwork.hpp"
#include "ServerIDMap.hpp"
#include "ServerProtocolMessagePair.hpp"
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
        Address4* listen_addy = mServerIDMap->lookupInternal(mContext->id());
        assert(listen_addy != NULL);
        net->listen(*listen_addy);
    }

    virtual ~ServerMessageQueue(){}
    virtual bool canAddMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool addMessage(ServerID destinationServer,const Network::Chunk&msg)=0;
    virtual bool receive(Network::Chunk** chunk_out, ServerID* source_server_out) = 0;
    virtual void service() = 0;

    virtual void setServerWeight(ServerID sid, float weight) = 0;

    virtual void reportQueueInfo(const Time& t) const = 0;

    virtual void getQueueInfo(std::vector<QueueInfo>& queue_info) const = 0;
    virtual bool canSend(const ServerProtocolMessagePair&msg){
        static Network::Chunk throwaway1(1);
        static Network::Chunk throwaway4(4);
        static Network::Chunk throwaway16(16);
        static Network::Chunk throwaway64(64);
        static Network::Chunk throwaway128(128);
        static Network::Chunk throwaway256(256);
        static Network::Chunk throwaway512(512);
        static Network::Chunk throwaway1024(1024);
        static Network::Chunk throwaway2048(2048);
        static Network::Chunk throwaway4096(4096);
        static Network::Chunk throwaway8192(8192);
        static Network::Chunk throwaway16384(16384);
        static const Network::Chunk*examples[15]={&throwaway1,
                                     &throwaway4,
                                     &throwaway4,
                                     &throwaway16,
                                     &throwaway16,
                                     &throwaway64,
                                     &throwaway64,
                                     &throwaway128,
                                     &throwaway256,
                                     &throwaway512,
                                     &throwaway1024,
                                     &throwaway2048,
                                     &throwaway4096,
                                     &throwaway8192,
                                     &throwaway16384};
        static const char LogTable256[] = 
            {
                0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
            };
        
        uint32 v=msg.size();
        unsigned int t, tt; // temporaries
        unsigned int r;

        if ((tt = (v >> 16)))
        {
            r = (t = (tt >> 8)) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
        }
        else 
        {
            r = (t = (v >> 8)) ? 8 + LogTable256[t] : LogTable256[v];
        }
        if (v!=(uint32)(1<<r)) ++r;
        if (r>=15) r=14;
        bool retval=canAddMessage(msg.dest(),*examples[r]);
        printf ("Can Add %d really(%d) %d\n",msg.size(),(int)examples[r]->size(),(int)retval);
        return retval;
    }
protected:
    SpaceContext* mContext;
    Network* mNetwork;
    ServerIDMap* mServerIDMap;
};
}

#endif
