/*  cobra
 *  LossyFairMessageQueue.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of cobra nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LOSSY_FAIR_MESSAGE_QUEUE_HPP_
#define _LOSSY_FAIR_MESSAGE_QUEUE_HPP_

#include "Time.hpp"
#include "Message.hpp"
#include "LossyFairSendQueue.hpp"

#include "FairMessageQueue.hpp"

namespace CBR {

class LossyFairSendQueue;

class Message;
class Server;

template <class Message,class Key,class WeightFunction=Weight<Queue<Message*> > > class LossyFairMessageQueue {
public:
    struct ServerQueueInfo {
        ServerQueueInfo():nextFinishTime(0) {
            messageQueue=NULL;
            weight=1.0;
        }
        ServerQueueInfo(Queue<Message*>* queue, float w)
         : messageQueue(queue), weight(w), nextFinishTime(0) {}

        Queue<Message*>* messageQueue;
        float weight;
        Time nextFinishTime;
    };

    typedef std::map<Key, ServerQueueInfo> ServerQueueInfoMap;
    unsigned int mEmptyQueueMessageLength;
    bool mRenormalizeWeight;
    int64 mLeftoverBytes;
    typedef Queue<Message*> MessageQueue;
    LossyFairMessageQueue(uint32 bytes_per_second, unsigned int emptyQueueMessageLength, bool renormalizeWeight)
        :mEmptyQueueMessageLength(emptyQueueMessageLength),
         mRenormalizeWeight(renormalizeWeight),
         mLeftoverBytes(0),
         mRate(bytes_per_second),
         mTotalWeight(0),
         mCurrentTime(0),
         mCurrentVirtualTime(0),
         mMessageBeingSent(NULL),
         mMessageSendFinishTime(0),
         mServerQueues(),
         bytes_sent(0){}

    ~LossyFairMessageQueue(){
        typename ServerQueueInfoMap::iterator it = mServerQueues.begin();
        for(; it != mServerQueues.end(); it++) {
            ServerQueueInfo* queue_info = &it->second;
            delete queue_info->messageQueue;
        }
    }

    void getQueues(std::vector<MessageQueue*>& queue) {
        for(typename ServerQueueInfoMap::iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
	  ServerQueueInfo* queue_info = &it->second;
	  
	  queue.push_back(queue_info->messageQueue);
	}
    }

    void addQueue(MessageQueue *mq, Key server, float weight){
        assert(mq->empty());

        ServerQueueInfo queue_info (mq, weight);

        mTotalWeight += weight;
        mServerQueues[server] = queue_info;
    }
    bool removeQueue(Key server) {
        typename ServerQueueInfoMap::iterator where=mServerQueues.find(server);
        bool retval=( where != mServerQueues.end() );
        if (retval) {
            mTotalWeight -= where->second.weight;
            delete where->second.messageQueue;
            mServerQueues.erase(where);
        }
        return retval;
    }
    bool hasQueue(Key server) const{
        return ( mServerQueues.find(server) != mServerQueues.end() );
    }

    QueueEnum::PushResult queueMessage(Key dest_server, Message *msg){
        typename ServerQueueInfoMap::iterator qi_it = mServerQueues.find(dest_server);

        assert( qi_it != mServerQueues.end() );

        ServerQueueInfo* queue_info = &qi_it->second;

        if (queue_info->messageQueue->empty()) {
            if (!mEmptyQueueMessageLength) {
                mTotalWeight+=queue_info->weight;
            }
            queue_info->nextFinishTime = finishTime(msg->size(), queue_info->weight);
        }
        return queue_info->messageQueue->push(msg);
    }    

    // returns a list of messages which should be delivered immediately
    std::vector<Message*> tick(const Time& t) {
        Duration since_last = t - mCurrentTime;
        mCurrentTime = t;
        double renormalized_rate=mRate;
        if (mRenormalizeWeight)
            renormalized_rate*=mTotalWeight;
        mLeftoverBytes+=(int64)(((double)since_last.seconds())*renormalized_rate);
        std::vector<Message*> msgs;
        static Message*sNullMessage=new Message(mEmptyQueueMessageLength);
        static unsigned int serviceEmptyQueue=mEmptyQueueMessageLength;
        bool processed_message = true;
        while( processed_message == true ) {
            processed_message = false;

            // If we are currently working on delivering a message, check if it can now be delivered
            if (mMessageBeingSent != NULL) {
                if ( mCurrentTime > mMessageSendFinishTime ) {
                    if (mMessageBeingSent!=sNullMessage) {
                        msgs.push_back(mMessageBeingSent);
                    }
                    mMessageBeingSent = NULL;
                    processed_message = true;
                }
            } else {
                // with no processing, update the last finish time to the current time so we don't use
                // past time for processing a new message
                mMessageSendFinishTime = mCurrentTime;
            }

            // If no message is currently being processed (or we just finished processing one), check for
            // another message to process
            if (mMessageBeingSent == NULL) {
                // Find the non-empty queue with the earliest finish time
                ServerQueueInfo* min_queue_info = NULL;
                for(typename ServerQueueInfoMap::iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
  		    ServerQueueInfo* queue_info = &it->second;  		  
                    if (queue_info->messageQueue->empty()&&!serviceEmptyQueue) continue;
                    if (min_queue_info == NULL || queue_info->nextFinishTime < min_queue_info->nextFinishTime)
                        min_queue_info = queue_info;
                }

                // If we actually have something to deliver, deliver it
                if (min_queue_info) {
                    mCurrentVirtualTime = min_queue_info->nextFinishTime;
                    mMessageBeingSent = sNullMessage;
                    if (!min_queue_info->messageQueue->empty()) {
                        if (mLeftoverBytes<min_queue_info->messageQueue->front()->size())
                            break;
                        mMessageBeingSent=min_queue_info->messageQueue->pop();
                        if (min_queue_info->messageQueue->empty()&&!serviceEmptyQueue) {
                            mTotalWeight-=min_queue_info->weight;
                        }
                    }else {
                        if (mLeftoverBytes<mMessageBeingSent->size())
                            break;                        
                    }
                    uint32 message_size = mMessageBeingSent->size();
                    assert(message_size<=mLeftoverBytes);
                    mLeftoverBytes-=message_size;
                    //Duration duration_to_finish_send = Duration::milliseconds(message_size / (mRate/1000.f));
                    //mMessageSendFinishTime = mMessageSendFinishTime + duration_to_finish_send;

                    // update the next finish time if there's anything in the queue
                    if (serviceEmptyQueue||!min_queue_info->messageQueue->empty()) {
                        min_queue_info->nextFinishTime = finishTime(WeightFunction()(*min_queue_info->messageQueue,sNullMessage), min_queue_info->weight);
                    }
                }
            }
        }

        if (msgs.size() > 0) {
            for(uint32 k = 0; k < msgs.size(); k++) {
                bytes_sent += msgs[k]->size();
            }
            Duration since_start = mCurrentTime - Time(0);
            //printf("%d / %f = %f bytes per second\n", bytes_sent, since_start.seconds(), bytes_sent / since_start.seconds());
        }

        return msgs;
    }
private:
    // because I *CAN*
    Time FinnishTime(uint32 size, float weight) const{
        return finishTime(size,weight);
    }
    Time finishTime(uint32 size, float weight) const{
        float queue_frac = weight;
        Duration transmitTime = Duration::seconds( size / (mRate * queue_frac) );
        if (transmitTime == Duration(0)) transmitTime = Duration(1); // just make sure we take *some* time

        return mCurrentVirtualTime + transmitTime;
    }

    uint32 mRate;
    uint32 mTotalWeight;
    Time mCurrentTime;
    Time mCurrentVirtualTime;
    Message *mMessageBeingSent;
    Time mMessageSendFinishTime;
    ServerQueueInfoMap mServerQueues;

    uint32 bytes_sent;

}; // class LossyFairMessageQueue

} // namespace Cobra

#endif //_LOSSY_FAIR_MESSAGE_QUEUE_HPP_
