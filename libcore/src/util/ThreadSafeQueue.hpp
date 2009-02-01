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
void lock(Lock*lok);
/// acquires lok, calls check, and while check returns false wait on a condition
void wait(Lock*lok,
          Condition* cond,
          bool(*check)(void*, void *),
          void* arg1, void* arg2);
/// Notifies the condition once
void notify(Condition* cond);
/// release the lock
void unlock(Lock*lok);
/// creates a Lock class
Lock* lockCreate();
/// creates a condition
Condition* condCreate();
/// destroys the lock class
void lockDestroy(Lock*oldlock);
/// destroys the condition
void condDestroy(Condition*oldcond);
}
/// A queue of any type that has thread-safe push() and pop() functions.
template <typename T> class ThreadSafeQueue {
private:
	typedef std::deque<T> ListType;
	ListType mList;
    ThreadSafeQueueNS::Lock* mLock;
    ThreadSafeQueueNS::Condition* mCond;
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

    void swap(std::deque<T> &swapWith) {
        ThreadSafeQueueNS::lock(mLock);
		mList.swap(swapWith);
        ThreadSafeQueueNS::unlock(mLock);
    }

	/**
	 * Pushes value onto the queue
	 *
	 * @param value  is a new'ed value (you must not keep a reference)
	 */
	void push(T value) {
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

	/**
	 * Pops the front value from the queue and places it in value.
	 *
	 * @returns  the T at the front of the queue, or NULL if the queue
	 *           was empty (or if another thread grabbed the item).
	 */
	bool pop(T&ret) {
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
	/**
	 * Waits until an item is available on the list.
	 *
	 * @returns  the next item in the queue.  Will not return NULL.
	 */
	void blockingPop(T&retval) {
        ThreadSafeQueueNS::wait(mLock,mCond, &ThreadSafeQueue<T>::waitCheck,this,&retval);
    }
};

}

#endif
