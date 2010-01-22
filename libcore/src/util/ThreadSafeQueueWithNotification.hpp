/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  ThreadSafeQueueWithNotification.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_THREAD_SAFE_QUEUE_WITH_NOTIFICATION_HPP_
#define _SIRIKATA_THREAD_SAFE_QUEUE_WITH_NOTIFICATION_HPP_

#include "ThreadSafeQueue.hpp"

namespace Sirikata {

/** ThreadSafeQueueWithNotification is a ThreadSafeQueue that also allows the
 *  user to specify a notification callback which is invoked when an item is
 *  pushed on the queue, causing it to go from empty to non-empty (note that a
 *  notification is not (necessarily) generated for every single push).  This
 *  mechanism allows the "receiving" thread to "wait" on this queue efficiently,
 *  aggregating handling of elements if more are inserted before the
 *  notification is handled.  This also allows a single receiving thread to
 *  "wait" on multiple ThreadSafeQueues.
 *
 *  Note that notifications are generated from the thread or strand in which the
 *  push occurs.  If you want to receive them in another thread, you probably
 *  want to use a callback wrapped by a strand or which performs a post to an
 *  IOService, depending on your setup.
 *
 *  Finally, note that the notification mechanism is conservative: it is
 *  possible that, due to concurrent pushing and popping, it is possible an
 *  extra notification will be generated, resulting in a notification callback
 *  being invoked when nothing remains in the queue.  Therefore the user should
 *  not assume the queue is non-empty when a notification callback is invoked.
 */
template <typename T>
class ThreadSafeQueueWithNotification {
  public:
    typedef std::function<void()> Notification;

    /** Create a new ThreadSafeQueueWithNotification which will invoke the given
     *  notification callback when a push() causes the queue to become
     *  non-empty.
     */
    ThreadSafeQueueWithNotification(const Notification& cb)
            : mQueue(),
              mCallback(cb)
    {
    }

    ~ThreadSafeQueueWithNotification() {
    }

    /** \see ThreadSafeQueue::push. */
    void push(const T &value) {
        bool was_empty = empty();
        mQueue.push(value);
        if (was_empty)
            mCallback();
    }

    /** \see ThreadSafeQueue::pop. */
    bool pop(T& ret) {
        mQueue.pop(ret);
    }

    /** \see ThreadSafeQueue::blockingPop. */
    void blockingPop(T& retval) {
        mQueue.blockingPop(retval);
    }

    /** \see ThreadSafeQueue::popAll. */
    void popAll(std::deque<T> *popResults) {
        mQueue.popAll();
    }

    /** \see ThreadSafeQueue::probablyEmpty. */
    bool probablyEmpty() {
        return mQueue.probablyEmpty();
    }

    /** Returns true if the queue is empty. */
    bool empty() {
        return probablyEmpty();
    }

  private:
    // Disable default constructor -- if you're not specifying a callback you
    // shouldn't be using this class.
    ThreadSafeQueueWithNotification();
    // Disable assignment and copy
    ThreadSafeQueueWithNotification& operator=(const ThreadSafeQueueWithNotification& other);
    ThreadSafeQueueWithNotification(const ThreadSafeQueueWithNotification& other);

    ThreadSafeQueue mQueue;
    Notification mCallback;
};

} // namespace Sirikata

#endif //_SIRIKATA_THREAD_SAFE_QUEUE_WITH_NOTIFICATION_HPP_
