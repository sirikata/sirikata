/*  Sirikata Kernel -- Task scheduling system
 *  Scheduler.hpp
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

#ifndef SIRIKATA_Scheduler_HPP__
#define SIRIKATA_Scheduler_HPP__

#include "Time.hpp"

namespace Sirikata {
namespace Task {

/**
 * TaskFunction represents a runnable task that will is a bound
 * std::tr1::function to some class that needs to be run whenever it
 * needs processing time.  NOTE: Because this interface needs to be fast
 * and should be kept simple,
 *
 * @param LocalTime  When the task should aim to finish.
 * @returns        'true' if the task should remain on the ready queue
 *                 (if it needs more time), or false if it should sleep.
 */
typedef std::tr1::function1<bool(LocalTime)> TaskFunction;

/// Scheduler interface
class Scheduler {
public:
	/** Create a task. */
	SubscriptionId createTask(TaskFunction func) = 0;

	/** Put the task in the ready queue to be run at regular intervals. */
	void readyTask(SubscriptionId taskId) = 0;

	/** A task has nothing to do (or is waiting for some event). */
	void sleepTask(SubscriptionId taskId) = 0;

	/** Destroy the task associated with 'taskId'. */
	void destroyTask(SubscriptionId taskId) = 0;

	//void setPriority(SubscriptionId taskId, int prio) = 0;
};


/** Scheduler runs through the queue in the order that tasks were made
 * ready. */
class RoundRobinScheduler : Scheduler {
	struct TaskInfo;

	typedef std::list<TaskInfo*> FunctionList;
	typedef std::map<SubscriptionId, TaskInfo*> TaskIdMap;

	struct TaskInfo {
		SubscriptionId id;

		TaskFunction func;
		FunctionList *activeQueue;
		FunctionList::iterator removeIter;

		/* Help scheduler decide how much time to allocate btwn frames */
		//DeltaTime lastTimes[NUM_AVERAGES];
		//DeltaTime expectedTime;
	};

	TaskIdMap mTaskIdMap;
	FunctionList mReadyQueue;
	SubscriptionId mRunningTask;
public:

	SubscriptionId createTask(TaskFunction func) {
		SubscriptionId myId = SubscriptionIdClass::alloc();

		TaskInfo* ti = new TaskInfo;
		ti->id = myId;
		ti->func = func;
		ti->activeQueue = NULL;

		mTaskIdMap.insert(TaskIdMap::value_type(myId, ti));
	}

	void readyTask(SubscriptionId taskId) {
		TaskIdMap::iterator iter = mTaskIdMap.find(taskId);
		if (iter == mTaskIdMap.end()) {
			return;
		}
		TaskInfo* ti = (*iter).second;

		if (ti->activeQueue == NULL) {
			ti->activeQueue = &mReadyQueue;
			mReadyQueue.push_back(ti);
			FunctionList::iterator iter = mReadyQueue.end();
			--iter;
			ti->removeIter = iter;
		}
	}

	void sleepTask(SubscriptionId taskId) {
		TaskIdMap::iterator iter = mTaskIdMap.find(taskId);
		if (iter == mTaskIdMap.end()) {
			return;
		}
		TaskInfo* ti = (*iter).second;

		if (ti->activeQueue != NULL) {
			ti->activeQueue->remove(ti->removeIter);
			ti->activeQueue = NULL;
		}
	}

	void destroyTask(SubscriptionId taskId) {
		TaskIdMap::iterator iter = mTaskIdMap.find(taskId);
		if (iter == mTaskIdMap.end()) {
			return;
		}
		TaskInfo* ti = (*iter).second;

		if (ti->activeQueue != NULL) {
			ti->activeQueue->remove(ti->removeIter);
			ti->activeQueue = NULL;
		}

		delete ti;
		mTaskIdMap.remove(iter);
	}

};

}
}

#endif
