/*  Meru
 *  DependencyManager.cpp
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
#include "precomp.hpp"
#include "DependencyManager.hpp"
#include "EventSource.hpp"
#include "SequentialWorkQueue.hpp"

namespace Meru
{

unsigned int DependencyManager::DEFAULT_MAX_CONCURRENT_TASKS = 10;

DependencyManager::DependencyManager(bool destroy_on_completion)
  : mDestroyOnCompletion(destroy_on_completion)
{
	numConcurrentTasks = 0;
	maxConcurrentTasks = DEFAULT_MAX_CONCURRENT_TASKS;
	startQueue = new std::priority_queue<DependencyTask *, std::vector<DependencyTask *>, DependencyTask::DependencyTaskPriorityLessThanFunctor>();
	processedTasks = new std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>();
    mWithinRunQueuedTasks=false;
}

DependencyManager::~DependencyManager()
{
  while (!startQueue->empty())
  {
    DependencyTask *taskToDelete = startQueue->top();
    startQueue->pop();
    processedTasks->erase(taskToDelete);
    delete(taskToDelete);
  }
  delete (startQueue);

  for (std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>::iterator dependencyIter = processedTasks->begin(); dependencyIter != processedTasks->end(); ++dependencyIter)
  {
    delete (*dependencyIter);
  }
  delete (processedTasks);
}
static const String DependencyManagerRootString("dependencyRoot");
void DependencyManager::queueDependencyRoot(DependencyTask *task)
{
	if (!task->getDependents()->empty())
	{
		fprintf (stderr, "Warning in DependencyManager::queueDependencyRoot: Task is not root of dependency graph.\n");
	}
	else
	{
		// Add a parent dependent task to the given root task, so that any children dependencies
		// added by the completion of the root task can be added to its parent.
		establishDependencyRelationship(new DependencyTask(this,DependencyManagerRootString), task);
	}
	queueTask(task,true);
}

void DependencyManager::queueTask(DependencyTask *task,bool canRunTasks=true)
{
  // If given task has any dependencies, queue those instead.
  std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *dependencies = task->getDependencies();
  if (!dependencies->empty())
  {
    for (std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>::iterator dependencyIter = dependencies->begin(); dependencyIter != dependencies->end(); ++dependencyIter)
    {
      queueTask(*dependencyIter,false);
    }
  }
  else if (processedTasks->find(task) == processedTasks->end())
  {
    // Queue task.
    startQueue->push(task);
    processedTasks->insert(task);
  }

  if (canRunTasks&&!mWithinRunQueuedTasks)
  {
    SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(DependencyManager::runQueuedTasksStatic, this));
    //runQueuedTasksStatic(this);
  }
}

void DependencyManager::handleTaskCompletion(DependencyTask *task, bool successful)
{
  if (task->isFailedUponCompletion())
  {
    successful = false;
    task->handleFailure();
  }

    if (numConcurrentTasks>0)
        --numConcurrentTasks;
    else
        SILOG(resource,error,"Negative Number of concurrent tasks (i.e. a single task signaled completion more than once");

    task->setCompleted(true);

    if (successful)
    {
	// Consider queueing dependent tasks.
	std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *dependents = task->getDependents();
	for (std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>::iterator dependentIter = dependents->begin(); dependentIter != dependents->end(); dependentIter = dependents->begin())
	{
		DependencyTask *dependentTask = *dependentIter;
		disestablishDependencyRelationship(dependentTask, task);
		queueTask(dependentTask);
	}
    }
    else
    {
        // Fail dependent tasks.
	std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *dependents = task->getDependents();
	for (std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>::iterator dependentIter = dependents->begin(); dependentIter != dependents->end(); dependentIter = dependents->begin())
	{
		DependencyTask *dependentTask = *dependentIter;
		disestablishDependencyRelationship(dependentTask, task);
		if (!dependentTask->isCompleted())
		{
		  if ((!dependentTask->isStarted()) && (processedTasks->find(dependentTask) == processedTasks->end()))
		  {
		    ++numConcurrentTasks;
		    dependentTask->signalCompletion(false);
		  }
		  else
		  {
		    dependentTask->failUponCompletion();
		  }
		}
	}

	// Fail dependency tasks.
	std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *dependencies = task->getDependencies();
	for (std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>::iterator dependencyIter = dependencies->begin(); dependencyIter != dependencies->end(); dependencyIter = dependencies->begin())
	{
		DependencyTask *dependencyTask = *dependencyIter;
		disestablishDependencyRelationship(task, dependencyTask);
		if (!dependencyTask->isCompleted())
		{
		  if ((!dependencyTask->isStarted()) && (processedTasks->find(dependencyTask) == processedTasks->end()))
		  {
		    ++numConcurrentTasks;
		    dependencyTask->signalCompletion(false);
		  }
		  else
		  {
		    dependencyTask->failUponCompletion();
		  }
		}
	}
    }

    std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor>::iterator taskIter = processedTasks->find(task);
    if (taskIter != processedTasks->end())
    {
      processedTasks->erase(taskIter);
    }
    delete task;

    SequentialWorkQueue::getSingleton().queueWork(std::tr1::bind(DependencyManager::runQueuedTasksStatic, this));
}

bool DependencyManager::runQueuedTasksStatic(DependencyManager *manager)
{
  return (manager->runQueuedTasksInstance());
}

bool DependencyManager::runQueuedTasksInstance()
{
  mWithinRunQueuedTasks=true;
  while ((numConcurrentTasks < maxConcurrentTasks) && (!startQueue->empty()))
  {
    // Must pop task from queue before running it.
    DependencyTask *nextTaskToRun = startQueue->top();
    startQueue->pop();
    runTask(nextTaskToRun);
  }
  mWithinRunQueuedTasks=false;
  return true;
}

void DependencyManager::runTask(DependencyTask *task)
{
  if (task->isCompleted())
  {
    return;
  }

  task->setStarted(true);
  ++numConcurrentTasks;
  task->run();
}

void DependencyManager::establishDependencyRelationship(DependencyTask *dependentTask, DependencyTask *dependencyTask)
{
	dependentTask->addDependency(dependencyTask);
	dependencyTask->addDependent(dependentTask);
}

void DependencyManager::disestablishDependencyRelationship(DependencyTask *dependentTask, DependencyTask *dependencyTask)
{
	dependentTask->removeDependency(dependencyTask);
	dependentTask->addDisestablishedDependency(dependencyTask);
	dependencyTask->removeDependent(dependentTask);
	dependencyTask->addDisestablishedDependent(dependentTask);
}

void DependencyManager::destroyDisestablishedDependencyRelationship(DependencyTask *dependentTask, DependencyTask *dependencyTask)
{
        dependentTask->removeDisestablishedDependency(dependencyTask);
        dependencyTask->removeDisestablishedDependent(dependentTask);
}

}
