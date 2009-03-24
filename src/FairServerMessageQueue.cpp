#include "Network.hpp"
#include "Server.hpp"
#include "FairServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR{
FairServerMessageQueue::FairServerMessageQueue(Network* net, uint32 bytes_per_second, bool renormalizeWeights, const ServerID& sid, Trace* trace)
 : ServerMessageQueue(net, sid, trace),
   mServerQueues(bytes_per_second,0,renormalizeWeights)
{
}

bool FairServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
    ServerMessageHeader server_header(mSourceServer, destinationServer);

    uint32 offset = 0;
    Network::Chunk with_header;
    offset = server_header.serialize(with_header, offset);
    with_header.insert( with_header.end(), msg.begin(), msg.end() );
    offset += msg.size();

    if (mSourceServer == destinationServer) {
        mReceiveQueue.push(new Network::Chunk(with_header));
        return true;
    }

    return mServerQueues.queueMessage(destinationServer,new ServerMessagePair(destinationServer,with_header))==QueueEnum::PushSucceeded;
}

Network::Chunk* FairServerMessageQueue::receive() {
    if (mReceiveQueue.empty()) return NULL;

    Network::Chunk* c = mReceiveQueue.front();
    mReceiveQueue.pop();
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

    // no limit on receive bandwidth
    while( Network::Chunk* c = mNetwork->receiveOne() )
        mReceiveQueue.push(c);
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
