/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  SizedThreadSafeQueue.hpp
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

#ifndef SIRIKATA_SizedThreadSafeQueue_HPP__
#define SIRIKATA_SizedThreadSafeQueue_HPP__
#include "ThreadSafeQueue.hpp"


namespace Sirikata {


class SizedPointerResourceMonitor {
    AtomicValue<int32> mSize;
    const int32 mLimit;
public:
    void reset() {
        mSize=0;
    }
    uint32 maxSize() const {
        return (uint32)mLimit;
    }
    uint32 filledSize() const {
        return (uint32)mSize.read();
    }
    SizedPointerResourceMonitor(uint32 limit):mSize(0),mLimit((int32)limit){
        assert(mLimit>=0);
    }
    template <class T>bool preIncrement(T value, bool force) {
        if ((mSize+=(int32)value->size())>=mLimit&&mLimit&&!force) {
            mSize-=value->size();
            return false;
        }
        return true;
    }
    template <class T> void postDecrement(T value) {      
        int32 check=(mSize-=(int32)value->size());
        assert(check>=0);        
    }
    template <class T> bool probablyCanPush(T t) {
        return mSize.read()+(uint32)t->size()<=mLimit||!mLimit;
    }
    bool probablyCanPush(size_t size) {
        return mSize.read()+(int32)size<=mLimit||!mLimit;
    }
};

/**
 * This class acts like a thread safe queue but it conservatively tracks a particular resource size 
 * of the list to avoid wasting too much memory
 * Is an adapter on any type of thread safe queue including Lock Free queues
 */

template <typename T, class ResourceMonitor, class Superclass=ThreadSafeQueue<T> > class SizedThreadSafeQueue : protected Superclass {
    ResourceMonitor mResourceMonitor;
public:
    SizedThreadSafeQueue(const ResourceMonitor&rm):mResourceMonitor(rm){
        mResourceMonitor.reset();
    }
    const ResourceMonitor&getResourceMonitor()const{return mResourceMonitor;}
    void popAll(std::deque<T>*popResults) {
        Superclass::popAll(popResults);
        for (typename std::deque<T>::iterator i=popResults->begin(),ie=popResults->end();i!=ie;++i) {
            mResourceMonitor.postDecrement(*i);
        }
    }
    bool push(const T &value, bool force) {
        if (mResourceMonitor.preIncrement(value,force)) {                
            try {
                Superclass::push(value);
                return true;
            } catch (...) {
                mResourceMonitor.postDecrement(value);
                throw;//didn't get successfully pushed
            }
        }
        return false;
    }
    bool pop(T &value) {
        if (Superclass::pop(value)) {
            mResourceMonitor.postDecrement(value);
            return true;
        }
        return false;
    }
    void blockingPop(T&value) {
        Superclass::blockingPop(value);
        mResourceMonitor.postDecrement(value);
    }
    template <class U> bool probablyCanPush(const U&specifier) {
        return mResourceMonitor.probablyCanPush(specifier);
    }
    bool probablyEmpty() {
        return Superclass::probablyEmpty();
    }
};

}

#endif
