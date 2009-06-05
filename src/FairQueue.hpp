/*  cbr
 *  FairQueue.hpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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
#include "Queue.hpp"

namespace CBR {

template<class MessageQueue> class Weight {
public:
    template <class MessageType> uint32 operator()(MessageQueue&mq,const MessageType*nil)const {
        return mq.empty()?nil->size():mq.front()->size();
    }
};
template <class Message,class Key,class TQueue, class WeightFunction=Weight<TQueue > > class FairQueue {
public:
    struct ServerQueueInfo {
    private:
        ServerQueueInfo():nextFinishTime(0) {
            messageQueue=NULL;
            weight=1.0;
        }
    public:
        ServerQueueInfo(Key serverName, TQueue* queue, float w)
          : messageQueue(queue), weight(w), nextFinishTime(0),mKey(serverName) {}

        TQueue* messageQueue;
        float weight;
        Time nextFinishTime;
        Key mKey;
    };


    typedef std::map<Key, ServerQueueInfo> ServerQueueInfoMap;
    unsigned int mEmptyQueueMessageLength;
    bool mRenormalizeWeight;
    typedef TQueue MessageQueue;
    FairQueue(unsigned int emptyQueueMessageLength, bool renormalizeWeight)
        :mEmptyQueueMessageLength(emptyQueueMessageLength),
         mRenormalizeWeight(renormalizeWeight),
         mCurrentVirtualTime(0),
         mServerQueues()
    {
        mNullMessage = new Message(mEmptyQueueMessageLength);
    }

    ~FairQueue() {
        typename ServerQueueInfoMap::iterator it = mServerQueues.begin();
        for(; it != mServerQueues.end(); it++) {
            ServerQueueInfo* queue_info = &it->second;
            delete queue_info->messageQueue;
        }
    }

    void addQueue(MessageQueue *mq, Key server, float weight) {
        //assert(mq->empty());

        ServerQueueInfo queue_info (server, mq, weight);

        mServerQueues.insert(std::pair<Key,ServerQueueInfo>(server, queue_info));
    }
    void setQueueWeight(Key server, float weight) {
        typename ServerQueueInfoMap::iterator where=mServerQueues.find(server);
        bool retval=( where != mServerQueues.end() );
        if (where != mServerQueues.end()) {
            where->second.weight = weight;
        }
    }
    float getQueueWeight(Key server) const {
        typename ServerQueueInfoMap::const_iterator where=mServerQueues.find(server);

        if (where != mServerQueues.end()) {
          return where->second.weight;
        }

        return 0;
    }



    bool deprioritize(Key server,float factor, float affine, float minval,float maxval) {
        return changepriority(server,factor,affine,minval,maxval,true);
    }
    bool reprioritize(Key server,float factor, float affine, float minval,float maxval) {
        return changepriority(server,factor,affine,minval,maxval,false);
    }
    bool changepriority(Key server,float factor, float affine, float minval,float maxval, bool passOnDeprioritize) {
        typename ServerQueueInfoMap::iterator where=mServerQueues.find(server);
        if ( where == mServerQueues.end() )
            return false;
        float oldweight=where->second.weight;
        oldweight*=factor;
        oldweight+=affine;
        if (oldweight<minval){
            oldweight=minval;
        }
        this->setQueueWeight(server,oldweight);
        return true;
    }
    bool removeQueue(Key server) {
        typename ServerQueueInfoMap::iterator where=mServerQueues.find(server);
        bool retval=( where != mServerQueues.end() );
        if (retval) {
            delete where->second.messageQueue;
            mServerQueues.erase(where);
        }
        return retval;
    }
    bool hasQueue(Key server) const{
        return ( mServerQueues.find(server) != mServerQueues.end() );
    }
    unsigned int numQueues() const {
        return (unsigned int)mServerQueues.size();
    }
    QueueEnum::PushResult push(Key dest_server, Message *msg){
        typename ServerQueueInfoMap::iterator qi_it = mServerQueues.find(dest_server);

        assert( qi_it != mServerQueues.end() );

        ServerQueueInfo* queue_info = &qi_it->second;

        if (queue_info->messageQueue->empty()) {
            queue_info->nextFinishTime = finishTime(msg->size(), queue_info->weight);
        }
        return queue_info->messageQueue->push(msg);
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately for intermediate null messages when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allocated
    Message* front(uint64* bytes, Key*keyAtFront) {
        Message* result = NULL;
        Time vftime(0);
        ServerQueueInfo* min_queue_info = NULL;

        nextMessage(bytes, &result, &vftime, &min_queue_info);
        if (result != NULL) {
            assert( *bytes >= result->size() );
            *keyAtFront=min_queue_info->mKey;
            return result;
        }

        return NULL;
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allotted
    Message* pop(uint64* bytes) {
        Message* result = NULL;
        Time vftime(0);
        ServerQueueInfo* min_queue_info = NULL;

        nextMessage(bytes, &result, &vftime, &min_queue_info);
        if (result != NULL) {
            mCurrentVirtualTime = vftime;

            assert(min_queue_info != NULL);

            assert( *bytes >= result->size() );
            *bytes -= result->size();

            min_queue_info->messageQueue->pop();

            uint32 serviceEmptyQueue = mEmptyQueueMessageLength;

            // update the next finish time if there's anything in the queue
            if (serviceEmptyQueue || !min_queue_info->messageQueue->empty())
                min_queue_info->nextFinishTime = finishTime(WeightFunction()(*min_queue_info->messageQueue,mNullMessage), min_queue_info->weight);
        }

        return result;
    }

    bool empty() const {
        // FIXME we could track a count ourselves instead of checking all these queues
        for(typename ServerQueueInfoMap::const_iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
            const ServerQueueInfo* queue_info = &it->second;
            if (!queue_info->messageQueue->empty())
                return false;
        }
        return true;
    }

    // Returns the total amount of space that can be allocated for the destination
    uint32 maxSize(Key dest) const {
        typename ServerQueueInfoMap::const_iterator it = mServerQueues.find(dest);
        if (it == mServerQueues.end()) return 0;
        return it->second.messageQueue->maxSize();
    }

    // Returns the total amount of space currently used for the destination
    uint32 size(Key dest) const {
        typename ServerQueueInfoMap::const_iterator it = mServerQueues.find(dest);
        if (it == mServerQueues.end()) return 0;
        return it->second.messageQueue->size();
    }

protected:

    // Retrieves the next message to deliver, along with its virtual finish time, given the number of bytes available
    // for transmission.  May update bytes for null messages, but does not update it to remove bytes to be used for
    // the returned message.  Returns null either if the number of bytes is not sufficient or the queue is empty.
    void nextMessage(uint64* bytes, Message** result_out, Time* vftime_out, ServerQueueInfo** min_queue_info_out) {
        uint32 serviceEmptyQueue = mEmptyQueueMessageLength;

        Message* result = mNullMessage;
        while( result == mNullMessage) {
            // Find the non-empty queue with the earliest finish time
            ServerQueueInfo* min_queue_info = NULL;
            for(typename ServerQueueInfoMap::iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
                ServerQueueInfo* queue_info = &it->second;
                if (queue_info->messageQueue->empty()&&!serviceEmptyQueue) continue;
                if (min_queue_info == NULL || queue_info->nextFinishTime < min_queue_info->nextFinishTime)
                    min_queue_info = queue_info;
            }

            // If we actually have something to deliver, deliver it
            result = NULL;
            if (min_queue_info) {
                *min_queue_info_out = min_queue_info;
                *vftime_out = min_queue_info->nextFinishTime;
                result = mNullMessage;

                if (!min_queue_info->messageQueue->empty()) {
                    if (*bytes < min_queue_info->messageQueue->front()->size())
                        break;
                    result = min_queue_info->messageQueue->front();
                } else {
                    if (*bytes < result->size())
                        break;
                }

                if (result == mNullMessage) {
                    uint32 message_size = result->size();
                    assert (message_size <= *bytes);
                    *bytes -= message_size;

                    mCurrentVirtualTime = min_queue_info->nextFinishTime;

                    // update the next finish time if there's anything in the queue
                    if (serviceEmptyQueue)
                        min_queue_info->nextFinishTime = finishTime(WeightFunction()(*min_queue_info->messageQueue,mNullMessage), min_queue_info->weight);
                }
            }
        }

        if (result == mNullMessage)
            *result_out = NULL;
        else
            *result_out = result;
    }


    Time finishTime(uint32 size, float weight) const{
        float queue_frac = weight;
        Duration transmitTime = Duration::seconds( size / queue_frac );
        if (transmitTime == Duration(0)) transmitTime = Duration(1); // just make sure we take *some* time

        return mCurrentVirtualTime + transmitTime;
    }

protected:
    uint32 mRate;
    Time mCurrentVirtualTime;
    ServerQueueInfoMap mServerQueues;
    Message* mNullMessage;
}; // class FairQueue

} // namespace CBR

#endif //_FAIR_MESSAGE_QUEUE_HPP_
