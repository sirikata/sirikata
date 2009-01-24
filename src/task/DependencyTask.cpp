#include "EventManager.hpp"
#include "DependencyTask.hpp"
namespace Iridium {
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
            boost::shared_ptr<Event> ev(new CallDependencyFailedTask(this));
            gEventManager->fire(ev);
        }else {
            boost::shared_ptr<Event> ev(new CallDependencyTask(this));
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
