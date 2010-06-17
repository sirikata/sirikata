/*  Sirikata
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
 *  * Neither the name of Sirikata nor the names of its contributors may
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
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

/** Fair Queue with one input queue of Messages per Key, backed by a TQueue. Each
 *  input queue can be assigned a weight and selection happens according to FairQueuing.
 */
template <class Message,class Key,class TQueue> class FairQueue {
private:
    typedef TQueue MessageQueue;

    struct QueueInfo {
    private:
        QueueInfo()
         : key(),
           messageQueue(NULL),
           weight(1.f),
           weight_inv(1.f),
           nextFinishMessage(NULL),
           nextFinishStartTime(Time::null()),
           nextFinishTime(Time::null()),
           enabled(true)
        {
        }
    public:
        QueueInfo(Key _key, TQueue* queue, float w)
         : key(_key),
           messageQueue(queue),
           weight(w),
           weight_inv( w == 0.f ? 0.f : (1.f / w) ),
           nextFinishMessage(NULL),
           nextFinishStartTime(Time::null()),
           nextFinishTime(Time::null()),
           enabled(true)
        {}

        ~QueueInfo() {
            delete messageQueue;
        }

        Key key;
        TQueue* messageQueue;
        float weight;
        float weight_inv;
        Message* nextFinishMessage; // Need to verify this matches when we pop it off
        Time nextFinishStartTime; // The time the next message to finish started at, used to recompute if front() changed
        Time nextFinishTime;
        bool enabled;
    };

    typedef std::map<Key, QueueInfo*> QueueInfoByKey; // NOTE: this could be unordered, but must be unique associative container
    typedef std::multimap<Time, QueueInfo*> QueueInfoByFinishTime; // NOTE: this must be ordered multiple associative container

    typedef typename QueueInfoByKey::iterator ByKeyIterator;
    typedef typename QueueInfoByKey::const_iterator ConstByKeyIterator;

    typedef typename QueueInfoByFinishTime::iterator ByTimeIterator;
    typedef typename QueueInfoByFinishTime::const_iterator ConstByTimeIterator;

    typedef std::set<Key> KeySet;
    typedef std::set<QueueInfo*> QueueInfoSet;
public:
    FairQueue()
     :zero_time(Duration::zero()),
      min_tx_time(Duration::microseconds(1)),
      default_tx_time(Duration::seconds((float)1000)),
      mCurrentVirtualTime(Time::null()),
      mQueuesByKey(),
      mQueuesByTime(),
      mFrontQueue(NULL)
    {
        warn_count = 0;
    }

    ~FairQueue() {
        while(!mQueuesByKey.empty())
            removeQueue( mQueuesByKey.begin()->first );
    }

    void addQueue(MessageQueue *mq, Key key, float weight) {
        QueueInfo* queue_info = new QueueInfo(key, mq, weight);
        mQueuesByKey[key] = queue_info;
        computeNextFinishTime(queue_info);
        mFrontQueue = NULL; // Force recomputation of front
    }

    void setQueueWeight(Key key, float weight) {
        ConstByKeyIterator it = mQueuesByKey.find(key);
        if (it != mQueuesByKey.end()) {
            QueueInfo* qi = it->second;
            float old_weight = qi->weight;
            qi->weight = weight;
            qi->weight_inv = (weight == 0.f ? 0.f : (1.f/weight));
            // FIXME should we update the finish time here, or just wait until the next packet?
            // Updating here requires either starting from current time or keeping track of
            // the last dequeued time
            //updateNextFinishTime(qi);

            // Currently, we just special case queues going from zero
            // to non-zero weight so they don't get stuck.  Computing
            // based on the current virtual time is pretty much always
            // better than waiting the default maximum amount of time
            // for the current head packet to pass through.
            if (old_weight == 0.0) {
                removeFromTimeIndex(qi);
                computeNextFinishTime(qi);
            }
        }
    }

    float getQueueWeight(Key key) const {
        ConstByKeyIterator it = mQueuesByKey.find(key);
        if (it != mQueuesByKey.end())
            return it->second->weight;
        return 0.f;
    }

    bool removeQueue(Key key) {
        // Find the queue
        ByKeyIterator it = mQueuesByKey.find(key);
        bool havequeue = (it != mQueuesByKey.end());
        if (!havequeue) return false;

        QueueInfo* qi = it->second;

        // Remove from the time index
        removeFromTimeIndex(qi);

        // If its the front queue, reset it
        if (mFrontQueue == qi)
            mFrontQueue = NULL;

        // Clean up queue and main entry
        mQueuesByKey.erase(it);
        delete qi;

        return true;
    }

    // NOTE: Enabling and disabling only affects the computation of front() and
    // pop() by setting a flag. Otherwise disabled queues are treated just like
    // enabled queues, just ignored when looking for the next item.  Both
    // operations, under certain conditions, need to reset the previously
    // computed front queue because they can affect this computation by adding
    // or removing options.
    void enableQueue(Key key) {
        ByKeyIterator it = mQueuesByKey.find(key);
        if (it == mQueuesByKey.end())
            return;
        QueueInfo* qi = it->second;
        qi->enabled = true;
        // Enabling a queue *might* affect the choice of the front queue if
        //  a. another queue is currently selected as the front
        //  b. the enabled queue is non-empty
        //  c. the next finish time of the enabled queue is less than that of
        //     the currently selected front item
        if (mFrontQueue != NULL && // a
            !qi->messageQueue->empty() && // b
            qi->nextFinishTime < mFrontQueue->nextFinishTime) // c
            mFrontQueue = NULL;
    }

    void disableQueue(Key key) {
        ByKeyIterator it = mQueuesByKey.find(key);
        assert(it != mQueuesByKey.end());
        QueueInfo* qi = it->second;
        qi->enabled = false;

        // Disabling a queue will only affect the choice of front queue if the
        // one disabled *was* the front queue.
        if (mFrontQueue == qi)
            mFrontQueue = NULL;
    }

    bool hasQueue(Key key) const{
        return ( mQueuesByKey.find(key) != mQueuesByKey.end() );
    }

    uint32 numQueues() const {
        return (uint32)mQueuesByKey.size();
    }

    QueueEnum::PushResult push(Key key, Message *msg) {
        ByKeyIterator qi_it = mQueuesByKey.find(key);
        assert( qi_it != mQueuesByKey.end() );

        QueueInfo* queue_info = qi_it->second;
        bool wasEmpty = queue_info->messageQueue->empty() ||
            queue_info->nextFinishMessage == NULL;

        QueueEnum::PushResult pushResult = queue_info->messageQueue->push(msg);

        if (wasEmpty)
            computeNextFinishTime(queue_info);

        return pushResult;
    }

    // An alternative to push(key,msg) for input queue types that may not allow
    // manual pushing of data, e.g. a network queue.  This instead allows the
    // user to notify the queue that data was pushed onto the queue.  Note that
    // the name is notifyPushFront, not notifyPush: it is only necessary to call
    // this when the item at the front of the queue has changed (either by going
    // from empty -> one or more elements, or because the "queue" got
    // rearranged).  However, it is safe to call this method when any new
    // elements are pushed.  In this case of the network example, we only need
    // to know when data becomes available when none was left in the network
    // buffer, but it is safe to call notifyPushFront on every data received
    // callback.
    void notifyPushFront(Key key) {
        ByKeyIterator qi_it = mQueuesByKey.find(key);
        assert( qi_it != mQueuesByKey.end() );

        // We just need to (re)compute the next finish time.
        QueueInfo* queue_info = qi_it->second;
        removeFromTimeIndex(queue_info);
        computeNextFinishTime(queue_info);

        // Reevaluate front queue
        mFrontQueue = NULL;
    }

    // Returns the next message to deliver
    // \returns the next message, or NULL if the queue is empty
    Message* front(Key* keyAtFront) {
        Message* result = NULL;

        if (mFrontQueue == NULL) {
            Time vftime(Time::null());
            nextMessage(&result, &vftime, &mFrontQueue);
        }
        else { // Otherwise, just fill in the information we need from the marked queue
            assert(!mFrontQueue->messageQueue->empty());
            result = mFrontQueue->nextFinishMessage;
            assert(result == mFrontQueue->messageQueue->front());
        }

        if (result != NULL) {
            *keyAtFront = mFrontQueue->key;
            assert(mFrontQueue->enabled);
            return result;
        }

        return NULL;
    }

    // Returns the next message to deliver
    // \returns the next message, or NULL if the queue is empty
    Message* pop(Key* keyAtFront = NULL) {
        Message* result = NULL;
        Time vftime(Time::null());

        // If we haven't marked any queue as holding the front item, do so now
        if (mFrontQueue == NULL)
            nextMessage(&result, &vftime, &mFrontQueue);
        else { // Otherwise, just fill in the information we need from the marked queue
            assert(!mFrontQueue->messageQueue->empty());
            result = mFrontQueue->nextFinishMessage;
            assert(result == mFrontQueue->messageQueue->front());
            vftime = mFrontQueue->nextFinishTime;
        }

        if (result != NULL) {
            // Note: we may have skipped a msg using the predicate, so we use max here to make sure
            // the virtual time increases monotonically.
            mCurrentVirtualTime = std::max(vftime, mCurrentVirtualTime);

            assert(mFrontQueue != NULL);
            assert(mFrontQueue->enabled);

            if (keyAtFront != NULL)
                *keyAtFront = mFrontQueue->key;

            Message* popped_val = mFrontQueue->messageQueue->pop();
            assert(popped_val == mFrontQueue->nextFinishMessage);
            assert(popped_val == result);

            // Remove from queue time list
            removeFromTimeIndex(mFrontQueue);
            // Update finish time and add back to time index if necessary
            computeNextFinishTime(mFrontQueue, vftime);

            // Unmark the queue as being in front
            mFrontQueue = NULL;
        }

        return result;
    }

    bool empty() const {
        // Queues won't be in mQueuesByTime unless they have something in them
        // This allows us to efficiently answer false if we know we have pending
        // items
        return mQueuesByTime.empty();
    }

    // Returns the total amount of space that can be allocated for the destination
    uint32 maxSize(Key key) const {
        // FIXME we could go through fewer using the ByTime index
        ConstByKeyIterator it = mQueuesByKey.find(key);
        if (it == mQueuesByKey.end()) return 0;
        return it->second->messageQueue->maxSize();
    }

    // Returns the total amount of space currently used for the destination
    uint32 size(Key key) const {
        ConstByKeyIterator it = mQueuesByKey.find(key);
        if (it == mQueuesByKey.end()) return 0;
        return it->second->messageQueue->size();
    }

    // FIXME we really shouldn't have to expose this
    float avg_weight() const {
        if (mQueuesByKey.size() == 0) return 1.f;
        float w_sum = 0.f;
        for(ConstByKeyIterator it = mQueuesByKey.begin(); it != mQueuesByKey.end(); it++)
            w_sum += it->second->weight;
        return w_sum / mQueuesByKey.size();
    }

    // Key iteration support
    class const_iterator {
      public:
        Key operator*() const {
            return internal_it->first;
        }

        void operator++() {
            ++internal_it;
        }
        void operator++(int) {
            internal_it++;
        }

        bool operator==(const const_iterator& rhs) const {
            return internal_it == rhs.internal_it;
        }
        bool operator!=(const const_iterator& rhs) const {
            return internal_it != rhs.internal_it;
        }
      private:
        friend class FairQueue;

        const_iterator(const typename QueueInfoByKey::const_iterator& it)
                : internal_it(it)
        {
        }

        const_iterator();

        typename QueueInfoByKey::const_iterator internal_it;
    };

    const_iterator keyBegin() const {
        return const_iterator(mQueuesByKey.begin());
    }
    const_iterator keyEnd() const {
        return const_iterator(mQueuesByKey.end());
    }

protected:
    // Retrieves the next message to deliver, along with its virtual finish time
    // for transmission. Returns null if the queue is empty.
    void nextMessage(Message** result_out, Time* vftime_out, QueueInfo** min_queue_info_out) {
        *result_out = NULL;

        // If there's nothing in the queue, there is no next message
        if (mQueuesByTime.empty())
            return;

        // Loop through until we find one that has data and can be handled.
        bool advance = true; // Indicates whether the loop needs to advance, set to false when an unexpected front has already advanced to the next item in order to remove the current item
        for(ByTimeIterator it = mQueuesByTime.begin(); it != mQueuesByTime.end(); advance ? it++ : it) {
            Time min_queue_finish_time = it->first;
            QueueInfo* min_queue_info = it->second;

            advance = true;

            // First, check if this queue is even enabled
            if (!min_queue_info->enabled)
                continue;

            // These just assert that this queue is just sane.
            assert(min_queue_info->nextFinishMessage != NULL);
            assert(min_queue_info->nextFinishMessage == min_queue_info->messageQueue->front());

            *min_queue_info_out = min_queue_info;
            *vftime_out = min_queue_info->nextFinishTime;
            *result_out = min_queue_info->nextFinishMessage;
            break;
        }
    }

    // Finds and removes this queue from the time index (mQueuesByTime).
    void removeFromTimeIndex(QueueInfo* qi) {
        std::pair<ByTimeIterator, ByTimeIterator> eq_range = mQueuesByTime.equal_range(qi->nextFinishTime);
        ByTimeIterator start_q = eq_range.first;
        ByTimeIterator end_q = eq_range.second;

        for(ByTimeIterator it = start_q; it != end_q; it++) {
            if (it->second == qi) {
                mQueuesByTime.erase(it);
                return;
            }
        }
    }

    // Computes the next finish time for this queue and, if it has one, inserts it into the time index
    ByTimeIterator computeNextFinishTime(QueueInfo* qi, const Time& last_finish_time) {
        if ( qi->messageQueue->empty() ) {
            qi->nextFinishMessage = NULL;
            return mQueuesByTime.end();
        }

        // If we don't restrict to strict queues, front() may return NULL even though the queue is not empty.
        // For example, if the input queue is a FairQueue itself, nothing may be able to send due to the
        // canSend predicate.
        Message* front_msg = qi->messageQueue->front();
        if ( front_msg == NULL ) {
            qi->nextFinishMessage = NULL;
            return mQueuesByTime.end();
        }

        qi->nextFinishMessage = front_msg;
        qi->nextFinishTime = finishTime( front_msg->size(), qi, last_finish_time);
        qi->nextFinishStartTime = last_finish_time;

        ByTimeIterator new_it = mQueuesByTime.insert( typename QueueInfoByFinishTime::value_type(qi->nextFinishTime, qi) );

        return new_it;
    }

    void computeNextFinishTime(QueueInfo* qi) {
        computeNextFinishTime(qi, mCurrentVirtualTime);
    }

    /** Finish time for a packet that was inserted into a non-empty queue, i.e. based on the previous packet's
     *  finish time. */
    Time finishTime(uint32 size, QueueInfo* qi, const Time& last_finish_time) const {
        if (qi->weight == 0) {
            if (!(warn_count++))
                SILOG(fairqueue,fatal,"[FQ] Encountered 0 weight.");
            return last_finish_time + default_tx_time;
        }

        Duration transmitTime = Duration::seconds( size * qi->weight_inv );
        if (transmitTime == zero_time) {
            SILOG(fairqueue,fatal,"[FQ] Encountered 0 duration transmission");
            transmitTime = min_tx_time; // just make sure we take *some* time
        }
        return last_finish_time + transmitTime;
    }

protected:
    const Duration zero_time;
    const Duration min_tx_time;
    const Duration default_tx_time;
    mutable uint32 warn_count;

    uint32 mRate;
    Time mCurrentVirtualTime;
    // FIXME if I could get the templates to work, using multi_index_container instead of 2 containers would be preferable
    QueueInfoByKey mQueuesByKey;
    QueueInfoByFinishTime mQueuesByTime;
    QueueInfo* mFrontQueue; // Queue holding the front item
}; // class FairQueue

} // namespace Sirikata

#endif //_FAIR_MESSAGE_QUEUE_HPP_
