/*  Sirikata Kernel -- Task scheduling system
 *  DependencyTask.cpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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
#include "util/Standard.hh"
#include "EventManager.hpp"
#include "DependencyTask.hpp"
namespace Sirikata {
namespace Task {
namespace {
IdPair::Primary emptyId("");
}
class CallDependencyTask:public Event {
    DependentTask*mDependentTask;
  public:
    CallDependencyTask(DependentTask*dt):Event(IdPair(emptyId,0)){
        mDependentTask=dt;
    }

    virtual void operator()(EventHistory h) {
        (*mDependentTask)();
        this->Event::operator() (h);
    }
};
class CallDependencyFailedTask:public Event {
    DependentTask*mDependentTask;
  public:
    CallDependencyFailedTask(DependentTask *dt):Event(IdPair(emptyId,0)){
        mDependentTask=dt;
    }

    virtual void operator()(EventHistory h) {
        mDependentTask->finish(false);
        this->Event::operator() (h);
    }
};

void DependentTask::go() {
    if (mNumThisWaitingOn==0) {
        if (mFailure) {
            std::tr1::shared_ptr<Event> ev(new CallDependencyFailedTask(this));
            gEventManager->fire(ev);
        }else {
            std::tr1::shared_ptr<Event> ev(new CallDependencyTask(this));
            gEventManager->fire(ev);
        }
    }
}
void DependentTask::addDepender(DependentTask *depender) {
    depender->mNumThisWaitingOn++;
    mDependents.push_back(depender);
}
void DependentTask::finish(bool success) {
    std::vector<DependentTask*>::iterator deps=mDependents.begin(),depsend=mDependents.end();
    for (;
         deps!=depsend;
         ++deps) {
        assert((*deps)->mNumThisWaitingOn>0);
        --(*deps)->mNumThisWaitingOn;
        if (!success) {
            (*deps)->mFailure=true;
        }
        (*deps)->go();
    }
    mDependents.resize(0);
}
DependentTask::DependentTask() {
    mFailure=false;
    mNumThisWaitingOn=0;
}
}
}
