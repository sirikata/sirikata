/*  cobra
 *  Queue.hpp
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
 *  * Neither the name of cobra nor the names of its contributors may
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

#ifndef _SIRIKATA_QUEUE_HPP_
#define _SIRIKATA_QUEUE_HPP_

#include <sirikata/core/util/Platform.hpp>
#include "AbstractQueue.hpp"

namespace Sirikata {


/** Functor to retrieve the size of an object via a size() method. */
template<typename ElementType>
struct MethodSizeFunctor {
    uint32 operator()(const ElementType& e) const {
        return e.size();
    }
};

template<typename ElementType>
struct MethodSizeFunctor<ElementType*> {
    uint32 operator()(const ElementType* e) const {
        return e->size();
    }
};

/** Queue with maximum bytes of storage. */
template <typename ElementType, typename SizeFunctorType = MethodSizeFunctor<ElementType> >
class Queue : public AbstractQueue<ElementType> {

    std::deque<ElementType> mElements;
    SizeFunctorType mSizeFunctor;
    uint32 mMaxSize;
    uint32 mSize;
public:
    typedef ElementType Type;

    Queue(uint32 max_size){
        mMaxSize=max_size;
        mSize = 0;
    }
    ~Queue(){}

    QueueEnum::PushResult push(const ElementType &msg){
        uint32 msg_size = mSizeFunctor(msg);
        if (mSize + msg_size > mMaxSize) {
            if (msg_size > mMaxSize) SILOG(queue,fatal,"Tried to push element larger than maximum queue size: " << msg_size << " > " << mMaxSize);
            return QueueEnum::PushExceededMaximumSize;
        }
        mElements.push_back(msg);
        mSize += msg_size;
        return QueueEnum::PushSucceeded;
    }

    const ElementType& front() const{
        return mElements.front();
    }

    ElementType& front() {
        return mElements.front();
    }

    ElementType pop(){
        ElementType m = mElements.front();
        mElements.pop_front();
        uint32 m_size = mSizeFunctor(m);
        mSize -= m_size;
        return m;
    }

    bool empty() const{
        return mElements.empty();
    }

    uint32 maxSize() const {
        return mMaxSize;
    }

    uint32 size() const {
        return mSize;
    }

/** FIXME this is unsafe since we need to track the size of all messages
    std::deque<ElementType>& messages() {
        return mElements;
    }
*/

};

} // namespace Sirikata

#endif //_SIRIKATA_QUEUE_HPP_
