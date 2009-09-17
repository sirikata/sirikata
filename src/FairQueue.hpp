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

#include "Queue.hpp"

namespace CBR {

/** Predicate for FairQueue which never rejects the item being considered. */
struct AlwaysUsePredicate {
    template<typename Key, typename Message>
    bool operator()(const Key& key, const Message* msg) const {
        return true;
    }
};

/** Fair Queue with one input queue of Messages per Key, backed by a TQueue. Each
 *  input queue can be assigned a weight and selection happens according to FairQueuing,
 *  with the additional constraint that each potential front element will be checked
 *  against a Predicate to give the user a chance to block certain queues from being used
 *  (for instance, if the downstream queue couldn't handle it).
 */
template <class Message,class Key,class TQueue,class Predicate=AlwaysUsePredicate> class FairQueue {
public:
    struct ServerQueueInfo {
    private:
        ServerQueueInfo():nextFinishTime(0) {
            messageQueue=NULL;
            weight=1.0;
        }
    public:
        ServerQueueInfo(Key serverName, TQueue* queue, float w)
         : messageQueue(queue),
           weight(w),
           nextFinishTime(Time::null()),
           mKey(serverName) {}

        TQueue* messageQueue;
        float weight;
        Time nextFinishTime;
        Key mKey;

        struct FinishTimeOrder {
            bool operator()(const ServerQueueInfo* lhs, const ServerQueueInfo* rhs) {
                return (lhs->nextFinishTime < rhs->nextFinishTime);
            }
        };
    };


    typedef std::map<Key, ServerQueueInfo> ServerQueueInfoMap;
    typedef TQueue MessageQueue;

    FairQueue(const Predicate pred = Predicate())
     :mCurrentVirtualTime(Time::null()),
      mServerQueues(),
      mPredicate(pred),
      mFrontQueue(NULL)
    {
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

        // If the queue was empty then the new packet is first and its finish time needs to be computed.
        if (queue_info->messageQueue->empty())
            queue_info->nextFinishTime = finishTime(msg->size(), queue_info->weight);
        return queue_info->messageQueue->push(msg);
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately for intermediate null messages when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allocated
    Message* front(uint64* bytes, Key*keyAtFront) {
        Message* result = NULL;
        Time vftime(Time::null());
        mFrontQueue = NULL;

        nextMessage(bytes, &result, &vftime, &mFrontQueue);
        if (result != NULL) {
            assert( *bytes >= result->size() );
            *keyAtFront = mFrontQueue->mKey;
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
        Time vftime(Time::null());

        // If we haven't marked any queue as holding the front item, do so now
        if (mFrontQueue == NULL)
            nextMessage(bytes, &result, &vftime, &mFrontQueue);
        else { // Otherwise, just fill in the information we need from the marked queue
            assert(!mFrontQueue->messageQueue->empty());
            result = mFrontQueue->messageQueue->front();
            vftime = mFrontQueue->nextFinishTime;
        }

        if (result != NULL) {
            // Note: we may have skipped a msg using the predicate, so we use max here to make sure
            // the virtual time increases monotonically.
            mCurrentVirtualTime = std::max(vftime, mCurrentVirtualTime);

            assert(mFrontQueue != NULL);

            assert( *bytes >= result->size() );
            *bytes -= result->size();

            mFrontQueue->messageQueue->pop();

            // update the next finish time if there's anything in the queue
            if (!mFrontQueue->messageQueue->empty())
                mFrontQueue->nextFinishTime = finishTime(mFrontQueue->messageQueue->front()->size(), mFrontQueue->weight, mFrontQueue->nextFinishTime);

            // Unmark the queue as being in front
            mFrontQueue = NULL;
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
        // Create a list of queues, sorted by nextFinishTime.  FIXME we should probably track this instead of regenerating it every time
        std::vector<ServerQueueInfo*> queues_by_finish_time;
        queues_by_finish_time.reserve(mServerQueues.size());
        for(typename ServerQueueInfoMap::iterator it = mServerQueues.begin(); it != mServerQueues.end(); it++) {
            ServerQueueInfo* queue_info = &it->second;
            if (queue_info->messageQueue->empty()) continue;
            queues_by_finish_time.push_back(queue_info);
        }

        // If this list ends up empty then there are no enqueued messages
        if (queues_by_finish_time.empty()) {
            *result_out = NULL;
            return;
        }

        std::sort(queues_by_finish_time.begin(), queues_by_finish_time.end(), typename ServerQueueInfo::FinishTimeOrder());

        // Loop through until we find one that has data and can be handled.
        for(uint32 i = 0; i < queues_by_finish_time.size(); i++) {
            ServerQueueInfo* min_queue_info = queues_by_finish_time[i];

            // Check that we have enough bytes to deliver.  If not stop the search and return since doing otherwise
            // would violate the ordering.
            if (*bytes < min_queue_info->messageQueue->front()->size()) {
                *result_out = NULL;
                return;
            }

            // Now give the user a chance to veto this packet.  If they can use it, set output and return.
            // Otherwise just continue trying the rest of the options.
            if (mPredicate(min_queue_info->mKey, min_queue_info->messageQueue->front())) {
                *min_queue_info_out = min_queue_info;
                *vftime_out = min_queue_info->nextFinishTime;
                *result_out = min_queue_info->messageQueue->front();
                return;
            }
        }

        // If we get here then we've tried everything and nothing has satisfied our constraints. Give up.
        *result_out = NULL;
        return;
    }

    /** Finish time for a packet that was inserted into a non-empty queue, i.e. based on the previous packet's
     *  finish time. */
    Time finishTime(uint32 size, float weight, const Time& last_finish_time) const {
        float queue_frac = weight;
        Duration transmitTime = queue_frac == 0 ? Duration::seconds((float)1000) : Duration::seconds( size / queue_frac );
        if (transmitTime == Duration::zero()) transmitTime = Duration::microseconds(1); // just make sure we take *some* time

        return last_finish_time + transmitTime;
    }

    /** Finish time for a packet inserted into an empty queue, i.e. based on the most recent virtual time. */
    Time finishTime(uint32 size, float weight) const {
        float queue_frac = weight;
        Duration transmitTime = queue_frac == 0 ? Duration::seconds((float)1000) : Duration::seconds( size / queue_frac );
        if (transmitTime == Duration::zero()) transmitTime = Duration::microseconds(1); // just make sure we take *some* time

        return mCurrentVirtualTime + transmitTime;
    }

protected:
    uint32 mRate;
    Time mCurrentVirtualTime;
    ServerQueueInfoMap mServerQueues;
    Predicate mPredicate;
    ServerQueueInfo* mFrontQueue; // The queue we've marked as the one containing the front item
}; // class FairQueue

} // namespace CBR

#endif //_FAIR_MESSAGE_QUEUE_HPP_
