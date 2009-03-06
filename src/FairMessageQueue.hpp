/*  cobra
 *  FairMessageQueue.hpp
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

#ifndef _FAIR_MESSAGE_QUEUE_HPP_
#define _FAIR_MESSAGE_QUEUE_HPP_

#include "Time.hpp"
#include "MessageQueue.hpp"

namespace Cobra {

class Message;
class Server;
template <typename Message> class Queue {
    std::dequeue<Message> mMessages;
    uint32 mMaxSize;
public:
    MessageQueue(uint32 max_size){
        mMaxSize=max_size;
    }
    ~MessageQueue(){}

    enum PushResult {
        PushSucceeded,
        PushExceededMaximumSize
    };
    PushResult push(const Message &msg){
        if (mMessages.size()>=mMaxSize)
            return PushExceededMaximumSize;
        mMessages.push_back(msg);
        return PushSucceeded;
    }

    const Message& front() const{
        return mMessages.front();
    }
    Message& front() {
        return mMessages.front();
    }
    Message pop(){
        Message m;
        std::swap(m,mMessages.front());
        mMessages.pop_front();
        return m;
    }
    bool empty() const{
        return mMessages.empty();
    }
};

template<class MessageQueue> class Weight {
public:
    uint32 operator()(const MessageQueue&mq)const {
        return mq.front().size();
    }
};
class <Message,Key,WeightFunction=Weight<MessageQueue>> class FairMessageQueue {
public:
    typedef Queue<Message*> MessageQueue;
    FairMessageQueue(uint32 bytes_per_second)
        : mRate(bytes_per_second),
         mTotalWeight(0),
         mCurrentTime(0),
         mCurrentVirtualTime(0),
         mMessageBeingSent(NULL),
         mMessageSendFinishTime(0),
         mServerQueues(),
         bytes_sent(0){}   

    ~FairMessageQueue(){
        for(ServerQueueInfoMap::iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
            ServerQueueInfo* queue_info = it->second;
            delete queue_info->messageQueue;
            delete queue_info;
        }
    }

    void addQueue(MessageQueue *mq, Key server, float weight){
        assert(mq->empty());
        
        ServerQueueInfo* queue_info = new ServerQueueInfo(mq, weight);
        
        mTotalWeight += weight;
        mServerQueues[server] = queue_info;
    }
    bool hasQueue(Key server) const{
        return ( mServerQueues.find(server) != mServerQueues.end() );
    }

    MessageQueue::PushResult queueMessage(Key dest_server, Message msg){
        ServerQueueInfoMap::iterator qi_it = mServerQueues.find(dest_server);
        assert( qi_it != mServerQueues.end() );
        
        ServerQueueInfo* queue_info = qi_it->second;

        if (queue_info->messageQueue->empty())
            queue_info->nextFinishTime = finishTime(msg->size(), queue_info->weight);
        
        return queue_info->messageQueue->push(msg);
    }

    // returns a list of messages which should be delivered immediately
    std::vector<Message> tick(const Time& t){
        Duration since_last = t - mCurrentTime;
        mCurrentTime = t;
        
        std::vector<Message*> msgs;
        
        bool processed_message = true;
        while( processed_message == true ) {
            processed_message = false;
            
            // If we are currently working on delivering a message, check if it can now be delivered
            if (mMessageBeingSent != NULL) {
                if ( mCurrentTime > mMessageSendFinishTime ) {
                    msgs.push_back(mMessageBeingSent);
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
                for(ServerQueueInfoMap::iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
                    ServerQueueInfo* queue_info = &it->second;
                    if (queue_info->messageQueue->empty()) continue;
                    if (min_queue_info == NULL || queue_info->nextFinishTime < min_queue_info->nextFinishTime)
                        min_queue_info = queue_info;
                }
                
                // If we actually have something to deliver, deliver it
                if (min_queue_info) {
                    mCurrentVirtualTime = min_queue_info->nextFinishTime;
                    mMessageBeingSent = min_queue_info->messageQueue->pop();
                    
                    uint32 message_size = mMessageBeingSent->size();
                    Duration duration_to_finish_send = Duration::milliseconds(message_size / (mRate/1000.f));
                    mMessageSendFinishTime = mMessageSendFinishTime + duration_to_finish_send;
                    
                    // update the next finish time if there's anything in the queue
                    if (!min_queue_info->messageQueue->empty())
                        min_queue_info->nextFinishTime = finishTime(min_queue_info->messageQueue->frontSize(), min_queue_info->weight);
                }
            }
        }
        
        if (msgs.size() > 0) {
            for(uint32 k = 0; k < msgs.size(); k++) {
                bytes_sent += msgs[k]->size();
            }
            Duration since_start = mCurrentTime - Time(0);
            printf("%d / %f = %f bytes per second\n", bytes_sent, since_start.seconds(), bytes_sent / since_start.seconds());
        }
        
        return msgs;
        
    }
private:
    // because I *CAN*
    Time FinnishTime(uint32 size, float weight) const{
        return finishTime(size,weight);
    }
    Time finishTime(uint32 size, float weight) const{
        float queue_frac = weight / mTotalWeight;
        Duration transmitTime = Duration::seconds( size / (mRate * queue_frac) );
        if (transmitTime == Duration(0)) transmitTime = Duration(1); // just make sure we take *some* time
        
        return mCurrentVirtualTime + transmitTime;        
    }

    struct ServerQueueInfo {
        ServerQueueInfo(MessageQueue* queue, float w)
         : messageQueue(queue), weight(w), nextFinishTime(0) {}

        MessageQueue* messageQueue;
        float weight;
        Time nextFinishTime;
    };
    typedef std::map<Key, ServerQueueInfo> ServerQueueInfoMap;

    uint32 mRate;
    uint32 mTotalWeight;
    Time mCurrentTime;
    Time mCurrentVirtualTime;
    Message *mMessageBeingSent;
    Time mMessageSendFinishTime;
    ServerQueueInfoMap mServerQueues;

    uint32 bytes_sent;
}; // class FairMessageQueue

} // namespace Cobra

#endif //_FAIR_MESSAGE_QUEUE_HPP_
