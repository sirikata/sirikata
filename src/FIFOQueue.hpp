/*  cbr
 *  FIFOQueue.hpp
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

#ifndef _FIFO_QUEUE_HPP_
#define _FIFO_QUEUE_HPP_

#include "Time.hpp"
#include "Queue.hpp"

namespace CBR {

template <class MessageType, class Key>
class FIFOQueue {
protected:
    struct KeyMessagePair {
        KeyMessagePair()
         : key(), msg(NULL) {}

        KeyMessagePair(Key k, MessageType* m)
         : key(k), msg(m) {}

        uint32 size() const {
            return msg->size();
        }

        Key key;
        MessageType* msg;
    };

    typedef Queue<KeyMessagePair> MessageQueue;
    typedef std::map<Key, uint32> SizeMap;

public:
    FIFOQueue(uint32 maxsize)
     : mMessages(maxsize)
    {
    }

    ~FIFOQueue() {
    }

    QueueEnum::PushResult push(Key dest_server, MessageType* msg) {
        QueueEnum::PushResult retval = mMessages.push(KeyMessagePair(dest_server, msg));
        if (retval == QueueEnum::PushSucceeded)
            mDestSizes[dest_server] = size(dest_server) + msg->size();
        return retval;
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately for intermediate null messages when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allocated
    MessageType* front(uint64* bytes) {
        if (mMessages.empty()) return NULL;

        KeyMessagePair result = mMessages.front();
        if (result.msg->size() > *bytes) return NULL;

        return result.msg;
    }

    // Returns the next message to deliver, given the number of bytes available for transmission
    // \param bytes number of bytes available; updated appropriately when returns
    // \returns the next message, or NULL if the queue is empty or the next message cannot be handled
    //          given the number of bytes allotted
    MessageType* pop(uint64* bytes) {
        if (mMessages.empty()) return NULL;

        KeyMessagePair result = mMessages.front();
        if (result.msg->size() > *bytes) return NULL;

        *bytes -= result.msg->size();
        mMessages.pop();
        mDestSizes[result.key] = size(result.key) - result.msg->size();

        return result.msg;
    }

    bool empty() const {
        return mMessages.empty();
    }

    // Returns the total amount of space that can be allocated for the destination
    uint32 maxSize(Key dest) const {
        return mMessages.maxSize();
    }

    // Returns the total amount of space currently used for the destination
    uint32 size(Key dest) const {
        typename SizeMap::const_iterator it = mDestSizes.find(dest);
        if (it == mDestSizes.end())
            return 0;
        return it->second;
    }

protected:
    MessageQueue mMessages;
    SizeMap mDestSizes;
}; // class FIFOQueue

} // namespace CBR

#endif //_FIFO_QUEUE_HPP_
