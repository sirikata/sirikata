/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  ThreadSafeQueue.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include "util/Platform.hh"
#include "ThreadSafeQueue.hpp"
#include <boost/thread.hpp>
namespace Sirikata {
namespace ThreadSafeQueueNS{
class Lock :public boost::mutex {
};
class Condition :public boost::condition_variable {
};
void lock(Lock*lok) {
    lok->lock();
}
void wait(Lock*lok,Condition *cond, bool (*check) (void*, void*), void * arg1, void * arg2){
    boost::unique_lock<boost::mutex> lock(*lok);
    while ((*check)(arg1,arg2)){
        cond->wait(lock);
    }
}
void notify(Condition *cond){
    cond->notify_one();
}
void unlock(Lock*lok){
    lok->unlock();
}
Lock* lockCreate(){
    return new Lock;
}
Condition* condCreate(){
    return new Condition;
}
void lockDestroy(Lock*oldlock){
    delete oldlock;
}
void condDestroy(Condition*oldcond) {
    delete oldcond;
}

}
}
