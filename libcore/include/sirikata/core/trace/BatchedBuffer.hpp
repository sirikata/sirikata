/*  Sirikata
 *  BatchedBuffer.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_BATCHED_BUFFER_HPP_
#define _SIRIKATA_BATCHED_BUFFER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace Sirikata {

template<typename T>
struct Batch {
    static const uint16 max_size = 65535;
    uint16 size;
    T items[max_size];

    Batch() : size(0) {}

    bool full() const {
        return (size >= max_size);
    }

    uint32 avail() const {
        return max_size - size;
    }
};

class BatchedBuffer {
public:
    struct IOVec {
        IOVec()
                : base(NULL),
                  len(0)
        {}

        IOVec(const void* _b, uint32 _l)
                : base(_b),
                  len(_l)
        {}

        const void* base;
        uint32 len;
    };

    BatchedBuffer();

    void write(const IOVec* iov, uint32 iovcnt);

    void flush();

    // write the buffer to an ostream
    void store(FILE* os);
private:
    // write the specified number of bytes from the pointer to the buffer
    void write(const void* buf, uint32 nbytes);

    typedef Batch<uint8> ByteBatch;
    boost::recursive_mutex mMutex;
    ByteBatch* filling;
    std::deque<ByteBatch*> batches;
};

} // namespace Sirikata

#endif //_SIRIKATA_BATCHED_BUFFER_HPP_
