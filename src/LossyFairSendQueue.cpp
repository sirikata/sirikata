#include "Network.hpp"
#include "Server.hpp"
#include "LossyFairSendQueue.hpp"
#include "Message.hpp"

namespace CBR{

LossyFairSendQueue::LossyFairSendQueue(Network* net, uint32 bytes_per_second, bool renormalizeWeights, Trace* trace)
:  FairSendQueue(net, bytes_per_second, renormalizeWeights, trace),
   mClientQueues(bytes_per_second,0,false),
   mServerQueues(bytes_per_second,0,renormalizeWeights)
{
}


LocationMessage* LossyFairSendQueue::extractSoleLocationMessage(Network::Chunk& chunk) {
  Message* result;
  unsigned int offset=0;

  LocationMessage* locMsg = NULL;

  do {
    ServerMessageHeader hdr=ServerMessageHeader::deserialize(chunk,offset);

    offset=Message::deserialize(chunk,offset,&result);

    if (result->type() == MESSAGE_TYPE_LOCATION) {
      locMsg = (LocationMessage*) result;
    }
    else {
      locMsg = NULL;
      return locMsg;
    }

  } while ( offset<chunk.size() );

  return locMsg;
}

void LossyFairSendQueue::aggregateLocationMessages() {
    std::vector< Queue<ServerMessagePair*>* > queues;
    mClientQueues.getQueues(queues);

    for (uint32 i=0; i < queues.size(); i++) {
      Queue<ServerMessagePair*>* q = queues[i];

      if (q->empty()) continue;

      ServerMessagePair* frontMessage = q->front();

      Network::Chunk chunk = frontMessage->data();

      LocationMessage* locMsg = extractSoleLocationMessage(chunk);

      if ( !locMsg ) continue;

      std::vector<uint32> markedForDeletion;
      Time latestTime = locMsg->location().time();

      std::deque<ServerMessagePair*> messageList = q->messages();
      for (uint32 j=1; j < messageList.size(); j++) {
	ServerMessagePair* smp = messageList[i];

	Network::Chunk chunk = smp->data();

	LocationMessage* locMsg2 = extractSoleLocationMessage(chunk);

	if (locMsg2 && locMsg2->sourceObject() == locMsg->sourceObject() &&
	    locMsg2->destObject() == locMsg->destObject() && locMsg2->location().time() <= latestTime)
	  {
	    markedForDeletion.push_back(j);
	    //printf("Marked for deletion; j=%d, time=%d, latestTime=%d\n", j, locMsg2->location().time().raw(), latestTime.raw());
	  }
      }

      for (int j=markedForDeletion.size() - 1; j>=0; j--) {
	q->messages().erase( q->messages().begin() + markedForDeletion[j]);
      }
    }

    queues.clear();
    mServerQueues.getQueues(queues);

    for (uint32 i=0; i < queues.size(); i++) {
      Queue<ServerMessagePair*>* q = queues[i];

      if (q->empty()) continue;

      ServerMessagePair* frontMessage = q->front();

      Network::Chunk chunk = frontMessage->data();

      LocationMessage* locMsg = extractSoleLocationMessage(chunk);

      if ( !locMsg ) continue;

      std::vector<uint32> markedForDeletion;
      Time latestTime = locMsg->location().time();

      std::deque<ServerMessagePair*> messageList = q->messages();
      for (uint32 j=1; j < messageList.size(); j++) {
	ServerMessagePair* smp = messageList[i];

	Network::Chunk chunk = smp->data();

	LocationMessage* locMsg2 = extractSoleLocationMessage(chunk);

	if (locMsg2 && locMsg2->sourceObject() == locMsg->sourceObject() &&
	    locMsg2->destObject() == locMsg->destObject() && locMsg2->location().time() <= latestTime)
	  {
	    markedForDeletion.push_back(j);
	    //printf("Marked for deletion; locMsg=%x, locMsg2=%x, j=%d, time=%d, latestTime=%d\n", locMsg, locMsg2, j, locMsg2->location().time().raw(), latestTime.raw());
	  }
      }

      for (int j=markedForDeletion.size() - 1; j>=0; j--) {
	q->messages().erase( q->messages().begin() + markedForDeletion[j]);
      }
    }
}


}
