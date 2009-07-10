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

#ifndef _CBR_POLIST_HPP_
#define _CBR_POLIST_HPP_

#include "Utility.hpp"

namespace CBR {

/** Functor to retrieve the size of an object via a size() method. */
template<typename ElementType>
struct PartiallyOrderedMethodSizeFunctor {
    uint32 operator()(const ElementType& e) const {
        return e->size();
    }
};
template<typename ElementType, typename Destination>
struct ElementDestinationFunctor {
    Destination operator()(const ElementType& e) const {
        return e->dest();
    }
};

/** Queue with maximum bytes of storage. */
template <typename ElementType, typename Destination, typename ElementDestinationType = ElementDestinationFunctor<ElementType, Destination>, typename SizeFunctorType = PartiallyOrderedMethodSizeFunctor<ElementType> >
class PartiallyOrderedList {
    std::map<Destination,std::deque<ElementType> > mLists;
    std::deque<Destination> mElements;
    uint32 mMaxSize;
    uint32 mSize;
public:
    PartiallyOrderedList(uint32 max_size){
        mMaxSize=max_size;
        mSize = 0;
    }
    ~PartiallyOrderedList(){}

    QueueEnum::PushResult push(const ElementType &msg){
        uint32 msg_size = SizeFunctorType()(msg);
        if (mSize + msg_size > mMaxSize)
            return QueueEnum::PushExceededMaximumSize;
        Destination dst=ElementDestinationType()(msg);
        if (mLists.find(dst)==mLists.end()) {
            mElements.push_back(dst);
        }
        mLists[dst].push_back(msg);
        mSize += msg_size;
        return QueueEnum::PushSucceeded;
    }
    void deprioritize(){
        mElements.push_back(mElements.front());
        mElements.pop_front();
    }
    const ElementType& front() const{
        return mLists.find(mElements.front())->second.front();
    }

    ElementType& front() {
        return mLists[mElements.front()].front();
    }

    ElementType pop(){
        std::deque<ElementType>*items=&mLists[mElements.front()];

        ElementType m = items->front();
        items->pop_front();
        if (items->empty()) {
            mLists.erase(mLists.find(mElements.front()));
            mElements.pop_front();
        }else {
            deprioritize();
        }
        uint32 m_size = SizeFunctorType()(m);
        mSize -= m_size;
        return m;
    }

    bool empty() const{
        return mElements.empty();
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

} // namespace CBR

#endif //_CBR_QUEUE_HPP_
