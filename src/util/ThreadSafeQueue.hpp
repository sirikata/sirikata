/*     Iridium Utilities -- Iridium Synchronization Utilities
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
 *  * Neither the name of Iridium nor the names of its contributors may
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

#ifndef IRIDIUM_ThreadSafeQueue_HPP__
#define IRIDIUM_ThreadSafeQueue_HPP__

#include <boost/thread.hpp>

/// ThreadSafeQueue.hpp
namespace Iridium {

/// A queue of any type that has thread-safe push() and pop() functions.
template <typename T> class ThreadSafeQueue {
private:
	std::list<T> mList;
	boost::mutex mLock;
	boost::condition_variable mCV;

	inline ThreadSafeQueue& operator= (const ThreadSafeQueue &other) {}
	inline ThreadSafeQueue(const ThreadSafeQueue&other) {}

public:
	ThreadSafeQueue() {}

	/**
	 * Pushes value onto the queue
	 *
	 * @param value  is a new'ed value (you must not keep a reference)
	 */
	void push(const T &value) {
		boost::lock_guard<boost::mutex> push_to_list(mLock);

		mList.push_back(value);
		mCV.notify_one();
	}

	/**
	 * Pops the front value from the queue and places it in value.
	 *
	 * @returns  the T at the front of the queue, or NULL if the queue
	 *           was empty (or if another thread grabbed the item).
	 */
	T pop() {
		boost::lock_guard<boost::mutex> pop_from_list(mLock);
		if (mList.empty()) {
			return T();
		} else {
			T ret = mList.front();
			mList.pop_front();
			return ret;
		}
	}

	/**
	 * Waits until an item is available on the list.
	 *
	 * @returns  the next item in the queue.  Will not return NULL.
	 */
	inline T blockingPop() {
		boost::unique_lock<boost::mutex> pop_and_sleep(mLock);
		while (mList.empty()) {
			mCV.wait(pop_and_sleep);
		}
		T item = mList.front();
		mList.pop_front();
		return item;
	}
};

}

#endif
