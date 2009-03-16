#ifndef _CBR_LOSSYFAIRSENDQUEUE_HPP
#define _CBR_LOSSYFAIRSENDQUEUE_HPP
#include "SendQueue.hpp"
#include "LossyFairMessageQueue.hpp"
#include "FairSendQueue.hpp"

namespace CBR {
class LossyFairSendQueue:public FairSendQueue {
    class ServerMessagePair {
    private:
        std::pair<ServerID,Network::Chunk> mPair;
    public:
        ServerMessagePair(const ServerID&sid, const Network::Chunk&data):mPair(sid,data){}
        //destructively modifies the data chunk to quickly place it in the queue
        ServerMessagePair(const ServerID&sid, Network::Chunk&data):mPair(sid,Network::Chunk()){
            mPair.second.swap(data);
        }
        explicit ServerMessagePair(size_t size):mPair(0,Network::Chunk(size)){

        }
        unsigned int size()const {
            return mPair.second.size();
        }
        bool empty() const {
            return size()==0;
        }
        ServerID dest() const {
            return mPair.first;
        }

        const Network::Chunk data() const {
            return mPair.second;
        }
    };

public:
    LossyFairSendQueue(Network* net, uint32 bytes_per_second, bool renormalizeWeights, BandwidthStatistics* bstats) ;

protected:
    virtual void aggregateLocationMessages();

private:
    LocationMessage* extractSoleLocationMessage(Network::Chunk& chunk);

    LossyFairMessageQueue<ServerMessagePair,UUID> mClientQueues;
    LossyFairMessageQueue<ServerMessagePair, ServerID > mServerQueues;


};
}
#endif
