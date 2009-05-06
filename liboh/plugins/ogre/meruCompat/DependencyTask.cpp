/*  Meru
 *  DependencyTask.cpp
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
#include "DependencyTask.hpp"
#include "DependencyManager.hpp"

#define DependencyTask MeruDependencyTask

namespace Meru
{

DependencyTask::DependencyTask(DependencyManager *mgr, const String &debugName, unsigned int priority)
    : mDebugName(debugName), mManager(mgr), priority(priority)
{
  dependencies = new std::set<DependencyTask *, DependencyTaskLessThanFunctor>();
  dependents = new std::set<DependencyTask *, DependencyTaskLessThanFunctor>();
  disestablishedDependencies = new std::set<DependencyTask *, DependencyTaskLessThanFunctor>();
  disestablishedDependents = new std::set<DependencyTask *, DependencyTaskLessThanFunctor>();
  mCompleted = false;
  mStarted = false;
  mFailed = false;
  this->signaled=false;
}

DependencyTask::~DependencyTask()
{
  delete (dependencies);
  delete (dependents);
  delete (disestablishedDependencies);
  delete (disestablishedDependents);
}

std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *DependencyTask::getDependencies()
{
	return dependencies;
}

std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *DependencyTask::getDependents()
{
	return dependents;
}

std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *DependencyTask::getDisestablishedDependencies()
{
	return disestablishedDependencies;
}

std::set<DependencyTask *, DependencyTask::DependencyTaskLessThanFunctor> *DependencyTask::getDisestablishedDependents()
{
	return disestablishedDependents;
}

void DependencyTask::addDependency(DependencyTask *dependencyTask)
{
	dependencies->insert(dependencyTask);
}

void DependencyTask::removeDependency(DependencyTask *dependencyTask)
{
	dependencies->erase(dependencies->find(dependencyTask));
}

void DependencyTask::addDependent(DependencyTask *dependentTask)
{
	dependents->insert(dependentTask);
}

void DependencyTask::removeDependent(DependencyTask *dependentTask)
{
	dependents->erase(dependents->find(dependentTask));
}

void DependencyTask::addDisestablishedDependency(DependencyTask *dependencyTask)
{
	disestablishedDependencies->insert(dependencyTask);
}

void DependencyTask::removeDisestablishedDependency(DependencyTask *dependencyTask)
{
	disestablishedDependencies->erase(disestablishedDependencies->find(dependencyTask));
}

void DependencyTask::addDisestablishedDependent(DependencyTask *dependentTask)
{
	disestablishedDependents->insert(dependentTask);
}

void DependencyTask::removeDisestablishedDependent(DependencyTask *dependentTask)
{
	disestablishedDependents->erase(disestablishedDependents->find(dependentTask));
}

void DependencyTask::setPriority(unsigned int newPriority)
{
	priority = newPriority;
}

unsigned int DependencyTask::getPriority() const
{
	return (priority);
}

void DependencyTask::setCompleted(bool completed)
{
  mCompleted = completed;
}

bool DependencyTask::isCompleted()
{
  return mCompleted;
}

void DependencyTask::setStarted(bool started)
{
  mStarted = started;
}

bool DependencyTask::isStarted()
{
  return mStarted;
}

void DependencyTask::run()
{
  signalCompletion();
}

void DependencyTask::signalCompletion(bool successful)
{
  if (mCompleted)
  {
    return;
  }

  if (this->signaled) {
      SILOG(resource,error," Task already signaled completion "<<((size_t)this));
  }
  this->signaled=true;
  if (!successful)
  {
    handleFailure();
  }
  mManager->handleTaskCompletion(this, successful);
}

void DependencyTask::handleFailure()
{
  // NO OP by default.
}

void DependencyTask::failUponCompletion()
{
  mFailed = true;
}

bool DependencyTask::isFailedUponCompletion()
{
  return mFailed;
}

}
