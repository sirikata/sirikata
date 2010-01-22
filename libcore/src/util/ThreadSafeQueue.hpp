/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  ThreadSafeQueue.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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

#ifndef SIRIKATA_ThreadSafeQueue_HPP__
#define SIRIKATA_ThreadSafeQueue_HPP__


namespace Sirikata {
namespace ThreadSafeQueueNS{
class Lock;
class Condition;
/// acquires lok Locks a lock
SIRIKATA_EXPORT void lock(Lock*lok);
/// acquires lok, calls check, and while check returns false wait on a condition
SIRIKATA_EXPORT void wait(Lock*lok,
          Condition* cond,
          bool(*check)(void*, void *),
          void* arg1, void* arg2);
/// Notifies the condition once
SIRIKATA_EXPORT void notify(Condition* cond);
/// release the lock
SIRIKATA_EXPORT void unlock(Lock*lok);
/// creates a Lock class
SIRIKATA_EXPORT Lock* lockCreate();
/// creates a condition
SIRIKATA_EXPORT Condition* condCreate();
/// destroys the lock class
SIRIKATA_EXPORT void lockDestroy(Lock*oldlock);
/// destroys the condition
SIRIKATA_EXPORT void condDestroy(Condition*oldcond);
}

/** ThreadSafeQueue provides a queue interface whose operations -- push, pop,
 *  empty -- are thread-safe.
 */
template <typename T>
class ThreadSafeQueue {
  protected:
    typedef std::deque<T> ListType;
    ThreadSafeQueueNS::Lock* mLock;
    ListType mList;
    ThreadSafeQueueNS::Condition* mCond;
  private:
    /**
     * Private function to copy a ThreadSafeQueue to another
     * Must pick a particular order, in this case pointer order, to acquire the locks
     * Must copy the list, and unlock if an exception is thrown
     *
     * @returns a reference to this class
     */
	ThreadSafeQueue& operator= (const ThreadSafeQueue &other){
        if (this<&other) {
            ThreadSafeQueueNS::lock(mLock);
            ThreadSafeQueueNS::lock(other.mLock);
        }else {
            ThreadSafeQueueNS::lock(other.mLock);
            ThreadSafeQueueNS::lock(mLock);
        }
        try {
            mList=other.mList;
        }catch (...) {
            if (this<&other) {
                ThreadSafeQueueNS::unlock(other.mLock);
                ThreadSafeQueueNS::unlock(mLock);
            }else {
                ThreadSafeQueueNS::unlock(mLock);
                ThreadSafeQueueNS::unlock(other.mLock);
            }
            throw;
        }
        if (this<&other) {
            ThreadSafeQueueNS::unlock(other.mLock);
            ThreadSafeQueueNS::unlock(mLock);
        }else {
            ThreadSafeQueueNS::unlock(mLock);
            ThreadSafeQueueNS::unlock(other.mLock);
        }

        return *this;
    }
	ThreadSafeQueue(const ThreadSafeQueue &other){
        mLock=ThreadSafeQueueNS::lockCreate();
        mCond=ThreadSafeQueueNS::condCreate();
        ThreadSafeQueueNS::lock(other.mLock);
        try {
            mList=other.mList;
            ThreadSafeQueueNS::unlock(other.mLock);
        }catch (...) {
            ThreadSafeQueueNS::unlock(other.mLock);
            throw;
        }
    }

  /**
   * This function is a helper function for a condition wait on empty data
   * Assumes the mLock is taken. Check if the list is empty--if so return true to wait longer. If not empty pop the front value and store the value in the value there
   * @param thus is a ThreadSafeQueue pointer: the queue to be operated upon
   * @param vretval is the item that should be filled with data to be popped from the queue
   * @returns true if there is no data in the ThreadSafeQueue and data must be waited for
   */
    static bool waitCheck(void * thus, void*vretval) {
        T* retval=reinterpret_cast<T*>(vretval);

        if (reinterpret_cast <ThreadSafeQueue* >(thus)->mList.empty())
            return true;
        *retval=reinterpret_cast <ThreadSafeQueue* >(thus)->mList.front();
        reinterpret_cast <ThreadSafeQueue* >(thus)->mList.pop_front();
        return false;
    }

public:
    class NodeIterator {
    private:
    	// Noncopyable
    	NodeIterator(const NodeIterator &other);
    	void operator=(const NodeIterator &other);

    	T *mNext;
    	ListType mSwappedList;

    public:
    	NodeIterator(ThreadSafeQueue<T> &queue) : mNext(NULL) {
    		queue.swap(mSwappedList);
    	}

    	T *next() {
    		if (mNext) {
    			mSwappedList.pop_front();
    		}
    		if (mSwappedList.empty()) {
    			return NULL;
    		}
    		mNext = &(mSwappedList.front());
    		return mNext;
    	}
    };
    friend class NodeIterator;

    ThreadSafeQueue() {
        mLock=ThreadSafeQueueNS::lockCreate();
        mCond=ThreadSafeQueueNS::condCreate();
    }
    ~ThreadSafeQueue(){
        ThreadSafeQueueNS::lockDestroy(mLock);
        ThreadSafeQueueNS::condDestroy(mCond);
    }

    /** Swap the contents of this queue with the one specified. Note that this
     *  differs from popAll -- any elements currently in the queue being swapped
     *  will be in the ThreadSafeQueue when the operation completes.
     *  \param swapWith a deque to swap elements with
     */
    void swap(std::deque<T> &swapWith) {
        ThreadSafeQueueNS::lock(mLock);
		mList.swap(swapWith);
        ThreadSafeQueueNS::unlock(mLock);
    }

    /** Pops all elements currently in the ThreadSafeQueue into popResults.  Any
     *  elements currently in popResults will be discarded.
     *  \param popResults a deque to place popped elements in
     */
    void popAll(std::deque<T> *popResults) {
        popResults->resize(0);
        swap(*popResults);
    }

    /** Push a value onto the queue.
     *  \param value the value to push
     */
    void push(const T &value) {
        ThreadSafeQueueNS::lock(mLock);
        try {
            mList.push_back(value);
            ThreadSafeQueueNS::notify(mCond);
        } catch (...) {
            ThreadSafeQueueNS::unlock(mLock);
            throw;
        }
        ThreadSafeQueueNS::unlock(mLock);
    }

    /** Pops the front element from the queue and places it in ret.
     *  \param ret storage for the popped element
     *  \returns true if an element was popped, false if the queue was empty
     */
    bool pop(T& ret) {
        ThreadSafeQueueNS::lock(mLock);
        if (mList.empty()) {
            ThreadSafeQueueNS::unlock(mLock);
            return false;
        } else {
            try {
                ret = mList.front();
                mList.pop_front();
            }catch (...) {
                ThreadSafeQueueNS::unlock(mLock);
                throw;
            }
            ThreadSafeQueueNS::unlock(mLock);
            return true;
        }
    }

    /** Pop an element from the queue, blocking until an element is available if
     *  the queue is currently empty.
     *  \param retval storage for the popped element
     */
    void blockingPop(T& retval) {
        ThreadSafeQueueNS::wait(mLock, mCond, &ThreadSafeQueue<T>::waitCheck, this, &retval);
    }

    /** Checks if the queue is probably empty, without any locking. Most of the
     *  time this is as good as checking empty() since the value of empty()
     *  could change immediately after the call returns since the queue is then
     *  unlocked.
     *  \returns true if the efficient check indicates the queue is empty
     */
    bool probablyEmpty() {
        return mList.empty();
    }
};

}

#endif
