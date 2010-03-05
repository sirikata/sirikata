/*  cbr
 *  CountResourceMonitor.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_COUNT_RESOURCE_MONITOR_HPP_
#define _CBR_COUNT_RESOURCE_MONITOR_HPP_

#include "Utility.hpp"

namespace CBR {

class CountResourceMonitor {
    Sirikata::AtomicValue<int32> mSize;
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
    CountResourceMonitor(uint32 limit)
     : mSize(0),
       mLimit((int32)limit)
    {
        assert(mLimit>=0);
    }

    template <class T>
    bool preIncrement(T value, bool force) {
        if ((mSize+=1)>=mLimit&&mLimit&&!force) {
            mSize -= 1;
            return false;
        }
        return true;
    }

    template <class T>
    bool preIncrement(T* value, bool force) {
        if ((mSize+=1)>=mLimit&&mLimit&&!force) {
            mSize -= 1;
            return false;
        }
        return true;
    }

    template <class T>
    void postDecrement(T value) {
        int32 check=(mSize-=1);
        assert(check>=0);
    }

    template <class T>
    void postDecrement(T* value) {
        int32 check=(mSize-=1);
        assert(check>=0);
    }

    template <class T>
    bool probablyCanPush(T t) {
        return mSize.read()+1<=mLimit||!mLimit;
    }

    template <class T>
    bool probablyCanPush(T* t) {
        return mSize.read()+1<=mLimit||!mLimit;
    }

    bool probablyCanPush(size_t size) {
        return mSize.read()+1<=mLimit||!mLimit;
    }
};

} // namespace CBR

#endif //_CBR_COUNT_RESOURCE_MONITOR_HPP_
