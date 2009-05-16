/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  ThreadId.cpp
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
#include "util/Standard.hh"
#include "ThreadId.hpp"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>
boost::once_flag f=BOOST_ONCE_INIT;

#ifndef NDEBUG
namespace {
const int MAX_THREAD_IDS=1024;
boost::thread::id* threadGroupIds[MAX_THREAD_IDS]={NULL};
static boost::mutex *threadGroupMutex=NULL;
void initializeAtomicInt() {
    threadGroupMutex=new boost::mutex;
}
}
#endif
namespace Sirikata {
bool ThreadId::isInThreadGroup(const ThreadIdCheck &groupId) {
#ifdef NDEBUG
    return true;
#else
    boost::thread::id this_id=boost::this_thread::get_id();
    if (threadGroupIds[groupId.mThreadId]==NULL) {
        boost::thread::id * tmp=new boost::thread::id(this_id);
        if (threadGroupIds[groupId.mThreadId]==NULL) {            
            threadGroupIds[groupId.mThreadId]=tmp;//should do atomic compare/swap here
        }else {
            delete tmp;
        }
    }
    return *threadGroupIds[groupId.mThreadId]==this_id;
#endif
}
Sirikata::ThreadIdCheck ThreadId::registerThreadGroup(const char*group) {
#ifdef NDEBUG
    return ThreadIdCheck();
#else
    boost::call_once(f,&initializeAtomicInt);
    boost::unique_lock<boost::mutex> lok(*threadGroupMutex);    
    static int retval=0;
    if (group) {
        static std::map<std::string,int> groupMap;
        if (groupMap.find(group)!=groupMap.end()) {
            ThreadIdCheck retval;
            retval.mThreadId=groupMap[group];
            return retval;
        }
        assert(MAX_THREAD_IDS>retval);
        ThreadIdCheck xretval;
        xretval.mThreadId=(groupMap[group]=retval++);
        return xretval;
    }else {
        ThreadIdCheck xretval;
        xretval.mThreadId=retval++;
        return xretval;
    }
#endif
}
}
