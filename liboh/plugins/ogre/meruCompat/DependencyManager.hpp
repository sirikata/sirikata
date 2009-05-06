/*  Meru
 *  DependencyManager.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _DEPENDENCY_MANAGER_HPP_
#define _DEPENDENCY_MANAGER_HPP_

#include "MeruDefs.hpp"
#include "Singleton.hpp"
#include "Event.hpp"
#include "DependencyTask.hpp"
#include <queue>
#include <vector>
#include <set>

namespace Meru
{
/**
 * Manager class that handles task processing of dependency DAGs.
 *
 * Author: Michael Chung
 */
class DependencyManager
{
public:
  /** DependencyManager constructor.
   *  \param destroy_on_completion if true, the Manager will destroy itself 
   *         (and all tasks as a result) when all tasks are complete.
   */
  DependencyManager(bool destroy_on_completion = false);
  virtual ~DependencyManager();

	/**
	 * Queues given task, which should be the root of its dependency DAG. The given task
	 * and all of its direct and indirect dependencies will be run in an appropriate
	 * order.
	 */
	virtual void queueDependencyRoot(DependencyTask *task);

	/**
	 * Decides which tasks to queue and run in response to the completion of given task.
	 * The given task which has just completed is deleted.
	 */
        virtual void handleTaskCompletion(DependencyTask *task, bool successful = true);

	/**
	 * Establishes a double-link dependent/dependency relationship between two tasks.
	 */
	virtual void establishDependencyRelationship(DependencyTask *dependentTask, DependencyTask *dependencyTask);

	/**
	 * Removes an existing double-link dependent/dependency relationship between two tasks.
	 */
	virtual void disestablishDependencyRelationship(DependencyTask *dependentTask, DependencyTask *dependencyTask);

	/**
	 * Removes an existing double-link dependent/dependency relationship between two tasks.
	 */
	virtual void destroyDisestablishedDependencyRelationship(DependencyTask *dependentTask, DependencyTask *dependencyTask);

        virtual bool runQueuedTasksInstance();

protected:
	static bool runQueuedTasksStatic(DependencyManager *manager);

	virtual void queueTask(DependencyTask *task, bool canRunTasks);
	virtual void runTask(DependencyTask *task);

        bool mDestroyOnCompletion;
        bool mWithinRunQueuedTasks;
	unsigned int numConcurrentTasks;
	unsigned int maxConcurrentTasks;

  // Queue containing tasks that have yet to be run.
  //std::queue<DependencyTask *> *startQueue;
	std::priority_queue<DependencyTask *, std::vector<DependencyTask *>, DependencyTask::DependencyTaskPriorityLessThanFunctor> *startQueue;

  // Set of tasks that are either currently waiting in the startQueue or have been run already.
  std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *processedTasks;

	static unsigned int DEFAULT_MAX_CONCURRENT_TASKS;
};

}

#endif //_DEPENDENCY_MANAGER_HPP_
