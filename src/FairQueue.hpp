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
     :mCurrentVirtualTime(Time::null()),
      mQueuesByKey(),
      mQueuesByTime(),
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
        computeNextFinishTime(queue_info);
        mFrontQueue = NULL; // Force recomputation of front
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
        assert(it != mQueuesByKey.end());
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
            assert(mFrontQueue->enabled);
            return result;
        }

        return NULL;
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allotted
    Message* pop(uint64* bytes, Key* keyAtFront = NULL) {
        Message* result = NULL;
        Time vftime(Time::null());

        // If we haven't marked any queue as holding the front item, do so now
        if (mFrontQueue == NULL)
            nextMessage(bytes, &result, &vftime, &mFrontQueue);
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

            assert( *bytes >= result->size() );
            *bytes -= result->size();

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
        // This allows us to efficiently answer false if we know we have pending items
        if (!mQueuesByTime.empty())
            return false;

        // Otherwise we must be careful and check all the queues since some queues may have been
        // pushed to behind our back, so we can't be certain they would be in mQueuesByTime
        for(ConstByKeyIterator it = mQueuesByKey.begin(); it != mQueuesByKey.end(); it++) {
            QueueInfo* qi = it->second;
            if (!qi->messageQueue->empty()) return false;
        }
        return true;
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
    // Checks if the given queue is satisfactory.  Returns false if the queue would violate a constraint, and the
    // consideration of any later queues should stop.  Otherwise it returns true.  The other outputs indicate whether
    // the queue was actually satisfactory, i.e. if *result_out != NULL then there was the queue was truly satisfactory.
    // Note that, despite passing in a QueueInfo*, we have additional parameters which are actually used.  This is because
    // this method may be used when the info in a QueueInfo is out of date.
    bool satisfies(QueueInfo* qi, uint64* bytes, QueueInfo** qiout, Message** result_out, Time* vftime_out) {
        *result_out = NULL;

        assert(qi->nextFinishMessage != NULL);
        // Check that we have enough bytes to deliver.  If not stop the search and return since doing otherwise
        // would violate the ordering.
        if (*bytes < qi->nextFinishMessage->size())
            return false;

        // Otherwsie, we have enough space and can use this queue.
        *qiout = qi;
        *vftime_out = qi->nextFinishTime;
        *result_out = qi->nextFinishMessage;
        return true;
    }

    // Retrieves the next message to deliver, along with its virtual finish time, given the number of bytes available
    // for transmission.  May update bytes for null messages, but does not update it to remove bytes to be used for
    // the returned message.  Returns null either if the number of bytes is not sufficient or the queue is empty.
    void nextMessage(uint64* bytes, Message** result_out, Time* vftime_out, QueueInfo** min_queue_info_out) {
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

            // Verify that the front is still really the front, can be violated by "queues" which don't adhere to
            // strict queue semantics, e.g. the FairQueue itself
            Message* min_queue_front = min_queue_info->messageQueue->front();

            // NOTE: We would like to assert this:
            // assert(min_queue_front == min_queue_info->nextFinishMessage);
            // but doing so is not safe.  During operations, a queue may have pushed downstream
            // causing predicates to change for any number of queues, including this one.  Since
            // we can't assume they are the same, we'll check and try to degrade gracefully.
            if (min_queue_front != min_queue_info->nextFinishMessage) {
                // Our solution is to recompute the information and try to work with it, despite the
                // fact that its out of the by-time-list.  There are three possibilities:
                // 1) The new front is NULL.  This is simple, just ignore and continue.
                // 2) The new front will have a finish time later in the by-time-list.  We can save
                //    this information and consider it again later when it is appropriate.
                // 3) The new front will have a finish time earlier in the by-time-list.  In this case
                //    we've screwed up -- we should have considered this option earlier and may even
                //    have missed it on earlier iterations.  NOTE: We are okay with this because it
                //    appears to be fairly rare, and requires a number of conditions to be true to
                //    occur (observation and theory imply that it will be relatively rare).  In this
                //    case we'll consider the option immediately, and be sure to fix up the by-time-list.

                // Log to insane just for debugging purposes.
                SILOG(fairqueue,error,"[FAIRQUEUE] nextMessage: Unexpected change in input queue front");

                // Advance iterator to make it safe, update entry for this queue (includes fixing up empty queues)
                advance = false;
                it++;
                removeFromTimeIndex(min_queue_info);
                ByTimeIterator new_it = computeNextFinishTime(min_queue_info, min_queue_info->nextFinishStartTime);

                // Case 1
                if (min_queue_front == NULL) {
                    // No need to reconsider, just continue with the next item.
                    continue;
                }

                Time new_next_finish_time = min_queue_info->nextFinishTime;

                if (new_next_finish_time > min_queue_finish_time) {
                    // Case 2
                    // Has been inserted in later, will be considered again when it is reached.

                    // But since we advanced the main iterator, we need to double check that the updated version
                    // wasn't inserted before the new main iterator -- either because the new main iterator is
                    // past the last item, or because the jump increased the current time beyond the new time, even
                    // though it was larger than the time we were at.
                    // Just stepping back to the new_it is sufficient to take care of this case since it will be
                    // considered at the next iteration.
                    if (it == mQueuesByTime.end() || new_next_finish_time <= it->first)
                        it = new_it;
                }
                else {
                    // Case 3 -- log to debug, since this is the only case that actually causes problems
                    SILOG(fairqueue,error,"[FAIRQUEUE] nextMessage: Unexpected change in input queue front to earlier virtual finish time.");

                    // It should have been moved earlier, check if its satisfactory
                    bool satis = satisfies(
                        min_queue_info, bytes,
                        min_queue_info_out, result_out, vftime_out
                    );
                    if (!satis || (satis && *result_out != NULL))
                        break;

                    // And if not, just continue
                    continue;
                }
            }

            bool satis = satisfies(
                min_queue_info, bytes,
                min_queue_info_out, result_out, vftime_out
            );
            if (!satis || (satis && *result_out != NULL))
                break;
        }

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
        qi->nextFinishTime = finishTime( front_msg->size(), qi->weight, last_finish_time);
        qi->nextFinishStartTime = last_finish_time;

        ByTimeIterator new_it = mQueuesByTime.insert( typename QueueInfoByFinishTime::value_type(qi->nextFinishTime, qi) );

        return new_it;
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
    QueueInfo* mFrontQueue; // Queue holding the front item
}; // class FairQueue

} // namespace CBR

#endif //_FAIR_MESSAGE_QUEUE_HPP_
