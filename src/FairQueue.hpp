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
template <class Message,class Key,class TQueue,class Predicate=AlwaysUsePredicate,bool DoubleCheckFront=false> class FairQueue {
private:
    typedef TQueue MessageQueue;

    struct QueueInfo {
    private:
        QueueInfo()
         : key(),
           messageQueue(NULL),
           weight(1.f),
           nextFinishMessage(NULL),
           nextFinishStartTime(Time::null()),
           nextFinishTime(Time::null())
        {
        }
    public:
        QueueInfo(Key _key, TQueue* queue, float w)
         : key(_key),
           messageQueue(queue),
           weight(w),
           nextFinishMessage(NULL),
           nextFinishStartTime(Time::null()),
           nextFinishTime(Time::null())
        {}

        ~QueueInfo() {
            delete messageQueue;
        }

        Key key;
        TQueue* messageQueue;
        float weight;
        Message* nextFinishMessage; // Need to verify this matches when we pop it off
        Time nextFinishStartTime; // The time the next message to finish started at, used to recompute if front() changed
        Time nextFinishTime;
    };

    typedef std::map<Key, QueueInfo*> QueueInfoByKey; // NOTE: this could be unordered, but must be unique associative container
    typedef std::multimap<Time, QueueInfo*> QueueInfoByFinishTime; // NOTE: this must be ordered multiple associative container

    typedef typename QueueInfoByKey::iterator ByKeyIterator;
    typedef typename QueueInfoByKey::const_iterator ConstByKeyIterator;

    typedef typename QueueInfoByFinishTime::iterator ByTimeIterator;
    typedef typename QueueInfoByFinishTime::const_iterator ConstByTimeIterator;

    typedef std::set<Key> KeySet;
public:
    FairQueue(const Predicate pred = Predicate())
     :mCurrentVirtualTime(Time::null()),
      mQueuesByKey(),
      mQueuesByTime(),
      mEmptyQueues(),
      mPredicate(pred),
      mFrontQueue(NULL)
    {
    }

    ~FairQueue() {
        while(!mQueuesByKey.empty())
            removeQueue( mQueuesByKey.begin()->first );
    }

    void addQueue(MessageQueue *mq, Key key, float weight) {
        QueueInfo* queue_info = new QueueInfo(key, mq, weight);
        mQueuesByKey[key] = queue_info;
        if (mq->empty()) {
            mEmptyQueues.insert(key);
        }
        else {
            computeNextFinishTime(queue_info);
            mFrontQueue = NULL; // Force recomputation of front
        }
    }

    void setQueueWeight(Key key, float weight) {
        ConstByKeyIterator it = mQueuesByKey.find(key);
        if (it != mQueuesByKey.end()) {
            QueueInfo* qi = it->second;
            qi->weight = weight;
            // FIXME should we update the finish time here, or just wait until the next packet?
            // Updating here requires either starting from current time or keeping track of
            // the last dequeued time
            //updateNextFinishTime(qi);
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

        // Remove from the empty queue list
        mEmptyQueues.erase(key);

        // Clean up queue and main entry
        mQueuesByKey.erase(it);
        delete qi;

        return true;
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
        bool wasEmpty = queue_info->messageQueue->empty();

        QueueEnum::PushResult pushResult = queue_info->messageQueue->push(msg);

        if (wasEmpty)
            computeNextFinishTime(queue_info);

        return pushResult;
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately for intermediate null messages when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allocated
    Message* front(uint64* bytes, Key* keyAtFront) {
        Message* result = NULL;
        Time vftime(Time::null());
        mFrontQueue = NULL;

        nextMessage(bytes, &result, &vftime, &mFrontQueue);
        if (result != NULL) {
            assert( *bytes >= result->size() );
            *keyAtFront = mFrontQueue->key;
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

    // FIXME This shouldn't exist, but is needed to handle queues which get
    // data pushed to them without going through this class. Currently this
    // is only caused by NetworkQueueWrapper and should be fixed.
    void service() {
        // FIXME It sucks that we have to do this, but we need to iterate through queues we think
        // are empty and double check them.  NetworkQueueWrapper doesn't behave properly (pushes
        // occur without our knowledge), so we have no choice here.
        KeySet empty_keys;
        empty_keys.swap(mEmptyQueues);
        for(typename KeySet::iterator it = empty_keys.begin(); it != empty_keys.end(); it++) {
            Key key = *it;
            computeNextFinishTime( mQueuesByKey.find(key)->second );
        }
        empty_keys.clear();
    }
protected:

    /** Verifies that all inputs, ensuring their front elements are what we expect.  Doesn't modify
     *  any data, only returns whether this assumption is valid.
     *  Returns true if the the front of the input queues were all valid, false if any were invalid.
     */
    bool verifyInputFront() {
        // This is the same as updating inputs, it just doesn't do any real updating
        return checkInputFront(false);
    }

    /** Checks all input queues to ensure that their front items are the same as the ones used
     *  to calculate virtual finish times, and recalculates them if they are incorrect.
     *  Returns true if the the front of the input queues were all valid, false if any were invalid.
     */
    bool checkInputFront(bool update) {
        /** First scan through and get a list of elements that need to be updated.  This avoids duplicate work
         *  when rearranging elements as well as guaranteeing no broken iterators.
         */
        QueueInfoByKey invalid_front_keys;
        for(ByTimeIterator it = mQueuesByTime.begin(); it != mQueuesByTime.end(); it++) {
            QueueInfo* queue_info = it->second;

            // Verify that the front is still really the front, can be violated by "queues" which don't adhere to
            // strict queue semantics, e.g. the FairQueue itself
            Message* queue_front = queue_info->messageQueue->front();
            if (queue_front != queue_info->nextFinishMessage) {
                invalid_front_keys[queue_info->key] = queue_info;
            }
        }

        if (!update)
            return (invalid_front_keys.empty());

        for(ByKeyIterator it = invalid_front_keys.begin(); it != invalid_front_keys.end(); it++) {
            QueueInfo* queue_info = it->second;

            // We need to:
            // 1. Recompute the finish time based on the new message
            // 2. Fix up the ByTimeIndex

            // Remove from queue time list, note that this will fubar the iterators
            removeFromTimeIndex(queue_info);

            // Recompute finish time, using the start time from the last element we thought would finish next
            // This will also add back to the ByTimeIndex, again, fubaring iterators
            computeNextFinishTime(queue_info, queue_info->nextFinishStartTime);
        }

        return (invalid_front_keys.empty());
    }

    // Retrieves the next message to deliver, along with its virtual finish time, given the number of bytes available
    // for transmission.  May update bytes for null messages, but does not update it to remove bytes to be used for
    // the returned message.  Returns null either if the number of bytes is not sufficient or the queue is empty.
    void nextMessage(uint64* bytes, Message** result_out, Time* vftime_out, QueueInfo** min_queue_info_out) {
        // If there's nothing in the queue, there is no next message
        if (mQueuesByTime.empty()) {
            *result_out = NULL;
            return;
        }

        // Verify front elements haven't changed out from under us
        if (DoubleCheckFront) {
            checkInputFront(true);
        }
#if SIRIKATA_DEBUG
        else {
            bool valid = verifyInputFront();
            if (!valid)
                SILOG(fairqueue,fatal,"[FAIRQUEUE] Invalid front elements on queue marked DoubleCheckFront=false.");
        }
#endif

        // Loop through until we find one that has data and can be handled.
        for(ByTimeIterator it = mQueuesByTime.begin(); it != mQueuesByTime.end(); it++) {
            QueueInfo* min_queue_info = it->second;

            // Verify that the front is still really the front, can be violated by "queues" which don't adhere to
            // strict queue semantics, e.g. the FairQueue itself
            Message* min_queue_front = min_queue_info->messageQueue->front();

            // Check that we have enough bytes to deliver.  If not stop the search and return since doing otherwise
            // would violate the ordering.
            if (*bytes < min_queue_front->size()) {
                *result_out = NULL;
                return;
            }

            // Now give the user a chance to veto this packet.  If they can use it, set output and return.
            // Otherwise just continue trying the rest of the options.
            if (mPredicate(min_queue_info->key, min_queue_front)) {
                *min_queue_info_out = min_queue_info;
                *vftime_out = min_queue_info->nextFinishTime;
                *result_out = min_queue_front;
                return;
            }
        }

        // If we get here then we've tried everything and nothing has satisfied our constraints. Give up.
        *result_out = NULL;
        return;
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
    void computeNextFinishTime(QueueInfo* qi, const Time& last_finish_time) {
        if ( qi->messageQueue->empty() ) {
            mEmptyQueues.insert(qi->key);
            return;
        }

        // If we don't restrict to strict queues, front() may return NULL even though the queue is not empty.
        // For example, if the input queue is a FairQueue itself, nothing may be able to send due to the
        // canSend predicate.
        Message* front_msg = qi->messageQueue->front();
        if ( front_msg == NULL ) {
            mEmptyQueues.insert(qi->key);
            return;
        }

        qi->nextFinishMessage = front_msg;
        qi->nextFinishTime = finishTime( front_msg->size(), qi->weight, last_finish_time);
        qi->nextFinishStartTime = last_finish_time;

        mQueuesByTime.insert( typename QueueInfoByFinishTime::value_type(qi->nextFinishTime, qi) );
        // In case it was an empty queue, remove it
        mEmptyQueues.erase(qi->key);
    }

    void computeNextFinishTime(QueueInfo* qi) {
        computeNextFinishTime(qi, mCurrentVirtualTime);
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
        return finishTime(size, weight, mCurrentVirtualTime);
    }

protected:
    uint32 mRate;
    Time mCurrentVirtualTime;
    // FIXME if I could get the templates to work, using multi_index_container instead of 2 containers would be preferable
    QueueInfoByKey mQueuesByKey;
    QueueInfoByFinishTime mQueuesByTime;
    KeySet mEmptyQueues; // FIXME everything but NetworkQueueWrapper works without tracking these, it only requires
                         // it because "pushing" to the NetworkQueue doesn't go through this FairQueue
    Predicate mPredicate;
    QueueInfo* mFrontQueue; // Queue holding the front item
}; // class FairQueue

} // namespace CBR

#endif //_FAIR_MESSAGE_QUEUE_HPP_
