#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairServerMessageQueue::FairServerMessageQueue(Network* net, uint32 bytes_per_second, bool renormalizeWeights, Trace* trace)
    : ServerMessageQueue(trace),
   mServerQueues(bytes_per_second,0,renormalizeWeights),
   mNetwork(net)
{
}

bool FairServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    return mServerQueues.queueMessage(destinationServer,new ServerMessagePair(destinationServer,msg))==QueueEnum::PushSucceeded;
}

Network::Chunk* FairServerMessageQueue::receive() {
    Network::Chunk* c = mNetwork->receiveOne();
    return c;
}

void FairServerMessageQueue::service(const Time&t){


    std::vector<ServerMessagePair*> finalSendMessages=mServerQueues.tick(t);
    for (std::vector<ServerMessagePair*>::iterator i=finalSendMessages.begin(),ie=finalSendMessages.end();
         i!=ie;
         ++i) {
        mNetwork->send((*i)->dest(),(*i)->data(),false,true,1);
        mTrace->packetSent(t, (*i)->dest(), GetMessageUniqueID((*i)->data()), (*i)->data().size());
    }
    finalSendMessages.resize(0);
}

void FairServerMessageQueue::setServerWeight(ServerID sid, float weight) {
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<ServerMessagePair*>(65536),sid,weight);
    }
    else {
        mServerQueues.setQueueWeight(sid, weight);
    }
}
void FairServerMessageQueue::removeServer(ServerID sid) {
    mServerQueues.removeQueue(sid);
}

}
