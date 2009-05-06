/*  Meru
 *  DependencyTask.hpp
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
#ifndef _DEPENDENCY_TASK_HPP_
#define _DEPENDENCY_TASK_HPP_

#include "MeruDefs.hpp"
#include <set>

namespace Meru
{
class DependencyManager;
/**
 * Base dependency task class for management by the DependencyManager class.
 *
 * Author: Michael Chung
 */
class DependencyTask
{
public:

	struct DependencyTaskEqualFunctor
	{
		bool operator()(const DependencyTask *task1, const DependencyTask *task2) const
		{
			// TODO: Finish implementing this.
			return true;
		}
	};

	struct DependencyTaskLessThanFunctor
	{
		bool operator()(const DependencyTask *task1, const DependencyTask *task2) const
		{
			// TODO: This is temporary pointer comparison. Eventually a hash set should be used, and this should be unnecessary.
			return (task1 < task2);
		}
	};

	struct DependencyTaskPriorityLessThanFunctor
	{
		bool operator()(const DependencyTask *task1, const DependencyTask *task2) const
		{
			return (task1->getPriority() < task2->getPriority());
		}
	};

	DependencyTask(DependencyManager* mgr, const String &debugName, unsigned int priority = DependencyTask::DEFAULT_PRIORITY);
	virtual ~DependencyTask();

	/**
	 * Runs the task - in this base implementation immediately completes.
	 * This method is meant to be subclassed, and should eventually lead
	 * to calling complete() when the task is finished.
	 */
	virtual void run();

  /** Signal the completion of the task to the DependencyManager.
   *  Doing this will free up a task execution slot and allow any
   *  dependent tasks with all other dependencies satisfied to be
   *  run.
   */
  void signalCompletion(bool successful = true);

  /**
   * Returns all tasks depended on by this task.
   */
  virtual std::set<DependencyTask *, DependencyTaskLessThanFunctor> *getDependencies();

  /**
   * Returns all tasks that depend on this task.
   */
  virtual std::set<DependencyTask *, DependencyTaskLessThanFunctor> *getDependents();

  /**
   * Returns all tasks that were depended on by this task (for garbage collection purposes).
   */
  virtual std::set<DependencyTask *, DependencyTaskLessThanFunctor> *getDisestablishedDependencies();

  /**
   * Returns all tasks that used to depend on this task (for garbage collection purposes).
   */
  virtual std::set<DependencyTask *, DependencyTaskLessThanFunctor> *getDisestablishedDependents();

  /**
   * Sets given task to be depended on by this task.
   */
  virtual void addDependency(DependencyTask *dependencyTask);

  /**
   * Sets given task to no longer be depended upon by this task.
   */
  virtual void removeDependency(DependencyTask *dependencyTask);

  /**
   * Sets given task to depend on this task.
   */
  virtual void addDependent(DependencyTask *dependentTask);

  /**
   * Sets given task to no longer depend on this task.
   */
  virtual void removeDependent(DependencyTask *dependentTask);

  /**
   * Sets given task to be depended on by this task, only for garbage collection purposes.
   */
  virtual void addDisestablishedDependency(DependencyTask *dependencyTask);

  /**
   * Sets given task to no longer be depended on by this task, only for garbage collection purposes.
   */
  virtual void removeDisestablishedDependency(DependencyTask *dependencyTask);

  /**
   * Sets given task to depend on this task, only for garbage collection purposes.
   */
  virtual void addDisestablishedDependent(DependencyTask *dependentTask);

  /**
   * Sets given task to no longer depend on this task, only for garbage collection purposes.
   */
  virtual void removeDisestablishedDependent(DependencyTask *dependentTask);

  /**
   * Sets task's priority.
   */
  virtual void setPriority(unsigned int newPriority);

  /**
   * Gets tasks' priority.
   */
  virtual unsigned int getPriority() const;

  /**
   * Sets task's completion status.
   */
  virtual void setCompleted(bool completed);

  /**
   * Gets task's completion status.
   */
  virtual bool isCompleted();

  /**
   * Sets task's started status.
   */
  virtual void setStarted(bool started);

  /**
   * Gets task's started status.
   */
  virtual bool isStarted();

  virtual void handleFailure();

  virtual void failUponCompletion();

  virtual bool isFailedUponCompletion();

protected:

  String mDebugName;
  DependencyManager* mManager;
  std::set<DependencyTask *, DependencyTaskLessThanFunctor> *dependencies;
  std::set<DependencyTask *, DependencyTaskLessThanFunctor> *dependents;
  std::set<DependencyTask *, DependencyTaskLessThanFunctor> *disestablishedDependencies;
  std::set<DependencyTask *, DependencyTaskLessThanFunctor> *disestablishedDependents;
  unsigned int priority;
  bool mCompleted;
  bool mStarted;
  bool mFailed;
  bool signaled;

  static const unsigned int DEFAULT_PRIORITY = 0;
};

}

#endif // _DEPENDENCY_TASK_HPP_
