/*  Sirikata Kernel -- Task scheduling system
 *  WorkQueue.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
/*  Created on: May 2, 2009 */

#ifndef SIRIKATA_WorkQueue_HPP__
#define SIRIKATA_WorkQueue_HPP__

#include <queue>
#include "Time.hpp"

namespace Sirikata {

template <class T> class LockFreeQueue;
template <class T> class ThreadSafeQueue;

namespace Task {

class SIRIKATA_EXPORT WorkItem {
protected:
	// Allow for simple deletion
	typedef std::auto_ptr<WorkItem> AutoPtr;
	friend class std::auto_ptr<WorkItem>;

	virtual ~WorkItem() {} // Virtual to avoid accidental deletion. Children can override this if necessary.
public:
	virtual void operator()() = 0;
};

class SIRIKATA_EXPORT WorkQueue {
public:
	/**
	 * Enqueues this WorkItem and wakes up any threads that may be blocking for new work.
	 *
	 * If NULL is passed, it acts as a WorkItem with an empty operator().
	 *
	 * Note that it is the responsibility of the WorkItem to delete itself when finished,
	 * usually done by creating a AutoPtr at the beginning of operator().
	 */
	virtual void enqueue(WorkItem *element)=0;

	/**
	 * Sleeps on a condition variable or semaphore until a job is added.
	 *
	 * In order to wake a sleeping thread for purposes of destruction, it is
	 * usually done by setting a volatile bool, then calling enqueue(NULL) for
	 * each thread, and joining on each individual one.
	 *
	 * It is legal to allow multiple threads to wait on dequeueBlocking().
	 * It is also legal to have one thread poll while another blocks, however
	 * you must be careful to wake the blocking thread correctly.
	 *
	 * \fixme: Does this belong in the definition? It may not be supported.
	 */
	virtual void dequeueBlocking()=0;

	/**
	 * This function simply checks if the head of the queue is non-NULL. If so,
	 * it runs the job at the head and returns true. If no job exists, returns false.
	 */
	virtual bool dequeuePoll()=0;

	/**
	 * Dequeues all WorkItem's that exist at the time of the call (by atomically
	 * splitting the list at that point.)
	 *
	 * This is implemented using a NodeIterator interface.
	 *
	 * Note that calling dequeuePoll() or dequeueBlocking() from within
	 * a work item dequeued by dequeueAll() will cause items to be
	 * processed out of order. If you intend to dequeue more items from
	 * a callback, you are probably better off using dequeuePoll().
	 */
	virtual unsigned int dequeueAll()=0;

	/** Calls dequeuePoll in a loop until either the time runs out, or no more
	 * jobs are left. Calling this with AbsTime::null() executes dequeueAll(). */
	virtual unsigned int dequeueUntil(AbsTime deadline) {
		if (deadline == AbsTime::null()) {
			return dequeueAll();
		}
		unsigned int count = 0;
		// Do at least one item no matter what.
		while (dequeuePoll() && AbsTime::now() < deadline) {
			count += 1;
		}
		return count;
	}

	virtual bool probablyEmpty() = 0;

	/// Virtual destructor.
	virtual ~WorkQueue() {}
};

template <class QueueType>
class SIRIKATA_EXPORT WorkQueueImpl : public WorkQueue {
	typedef QueueType Queue;
	Queue mQueue;
public:
	virtual void enqueue(WorkItem *element);
	virtual void dequeueBlocking();
	virtual bool dequeuePoll();
	virtual unsigned int dequeueAll();
	virtual ~WorkQueueImpl();

	virtual bool probablyEmpty();
};

typedef WorkQueueImpl<LockFreeQueue<WorkItem*> > LockFreeWorkQueue;
typedef WorkQueueImpl<ThreadSafeQueue<WorkItem*> > ThreadSafeWorkQueue;

template <class QueueType>
class SIRIKATA_EXPORT UnsafeWorkQueueImpl : public WorkQueue {
	typedef QueueType Queue;
	Queue mQueue[2];
	int mWhichQueue;
public:
	UnsafeWorkQueueImpl() : mWhichQueue(0) {};
	virtual void enqueue(WorkItem *element);
	virtual void dequeueBlocking();
	virtual bool dequeuePoll();
	virtual unsigned int dequeueAll();
	virtual ~UnsafeWorkQueueImpl();

	virtual bool probablyEmpty();
};

typedef UnsafeWorkQueueImpl<std::queue<WorkItem*> > ListWorkQueue;

}
}

#endif /* SIRIKATA_WorkQueue_HPP__ */
