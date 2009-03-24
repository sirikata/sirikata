#include "Utility.hpp"
#include "Network.hpp"
#include "Server.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "Message.hpp"

namespace CBR {

FIFOServerMessageQueue::FIFOServerMessageQueue(Network* net, uint32 bytes_per_second, const ServerID& sid, Trace* trace)
 : ServerMessageQueue(net, sid, trace),
   mRate(bytes_per_second),
   mRemainderBytes(0),
   mLastTime(0)
{
}

bool FIFOServerMessageQueue::addMessage(ServerID destinationServer,const Network::Chunk&msg){
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

    mQueue.push(ServerMessagePair(destinationServer,with_header));

    return true;
}

Network::Chunk* FIFOServerMessageQueue::receive() {
    if (mReceiveQueue.empty()) return NULL;

    Network::Chunk* c = mReceiveQueue.front();
    mReceiveQueue.pop();
    return c;
}

void FIFOServerMessageQueue::service(const Time& t){
    Duration sinceLast = t - mLastTime;
    uint32 free_bytes = mRemainderBytes + (uint32)(sinceLast.seconds() * mRate);

    while(!mQueue.empty() && mQueue.front().data().size() <= free_bytes) {
        bool ok=mNetwork->send(mQueue.front().dest(),mQueue.front().data(),false,true,1);
        free_bytes -= mQueue.front().data().size();
        assert(ok&&"Network Send Failed");
        mTrace->packetSent(t, mQueue.front().dest(), GetMessageUniqueID(mQueue.front().data()), mQueue.front().data().size());
        mQueue.pop();
    }

    mRemainderBytes = mQueue.empty() ? 0 : free_bytes;

    mLastTime = t;

    // no limit on receive bandwidth
    while( Network::Chunk* c = mNetwork->receiveOne() )
        mReceiveQueue.push(c);
}

void FIFOServerMessageQueue::setServerWeight(ServerID sid, float weight) {
}



}
