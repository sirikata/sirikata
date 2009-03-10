#include "Network.hpp"
#include "Server.hpp"
#include "FairSendQueue.hpp"


namespace CBR{
FairSendQueue::FairSendQueue(Network* net, uint32 bytes_per_second)
 : mClientQueues(bytes_per_second),
   mServerQueues(bytes_per_second),
   mNetwork(net)
{
}

bool FairSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    return mServerQueues.queueMessage(destinationServer,new ServerMessagePair(destinationServer,msg))==QueueEnum::PushSucceeded;
}
bool FairSendQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg,const UUID &src_obj){
    return mClientQueues.queueMessage(src_obj,new ServerMessagePair(destinationServer,msg))==QueueEnum::PushSucceeded;
}
void FairSendQueue::service(const Time&t){
    bool freeClientTicks=true;
    if (mClientServerBuffer.empty()) {
        freeClientTicks=false;
        mClientServerBuffer=mClientQueues.tick(t);
    }
    size_t count=0;
    for (count=0;count<mClientServerBuffer.size();++count) {
        if (mServerQueues.queueMessage(mClientServerBuffer[count]->mPair.first,
                                       mClientServerBuffer[count])!=QueueEnum::PushSucceeded) {
        }else {
            break;
        }
    }
    if (freeClientTicks&&count==mClientServerBuffer.size()) {
        mClientServerBuffer=mClientQueues.tick(t);
        freeClientTicks=false;
        count=0;
        for (count=0;count<mClientServerBuffer.size();++count) {
            if (mServerQueues.queueMessage(mClientServerBuffer[count]->mPair.first,
                                           mClientServerBuffer[count])!=QueueEnum::PushSucceeded) {
            }else {
                break;
            }
        }
    }

    std::vector<ServerMessagePair*> finalSendMessages=mServerQueues.tick(t);
    for (std::vector<ServerMessagePair*>::iterator i=finalSendMessages.begin(),ie=finalSendMessages.end();
         i!=ie;
         ++i) {
        mNetwork->send((*i)->mPair.first,(*i)->mPair.second,false,true,1);
    }
    finalSendMessages.resize(0);
    for (;count<mClientServerBuffer.size();++count) {
        if (mServerQueues.queueMessage(mClientServerBuffer[count]->mPair.first,mClientServerBuffer[count])!=QueueEnum::PushSucceeded) {
            break;
        }
    }
    if (count!=0&&count!=mClientServerBuffer.size()) {
        std::vector<ServerMessagePair*> rest;
        rest.insert(rest.end(),mClientServerBuffer.begin()+count,mClientServerBuffer.end());
        rest.swap(mClientServerBuffer);
    }else if (count==mClientServerBuffer.size()) {
        mClientServerBuffer.resize(0);
    }
}

    
void FairSendQueue::registerServer(ServerID sid, float weight) {
    if (!mServerQueues.hasQueue(sid)) {
        mServerQueues.addQueue(new Queue<ServerMessagePair*>(65536),sid,weight);
    }
}
void FairSendQueue::removeServer(ServerID sid) {
    mServerQueues.removeQueue(sid);
}
void FairSendQueue::registerClient(UUID sid, float weight) {
   if (!mClientQueues.hasQueue(sid)) { 
       mClientQueues.addQueue(new Queue<ServerMessagePair*>(65536),sid,weight);
   }
}
void FairSendQueue::removeClient(UUID sid) {
    mClientQueues.removeQueue(sid);
}

}
