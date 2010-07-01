/*  Sirikata
 *  BatchedBuffer.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/trace/BatchedBuffer.hpp>

namespace Sirikata {

BatchedBuffer::BatchedBuffer()
 : filling(NULL)
{
}

// write the specified number of bytes from the pointer to the buffer
void BatchedBuffer::write(const void* buf, uint32 nbytes)
{
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    const uint8* bufptr = (const uint8*)buf;
    while( nbytes > 0 ) {
        if (filling == NULL)
            filling = new ByteBatch();

        uint32 to_copy = std::min(filling->avail(), nbytes);

        memcpy( &filling->items[filling->size], bufptr, to_copy);
        filling->size += to_copy;
        bufptr += to_copy;
        nbytes -= to_copy;

        if (filling->full())
            flush();
    }
}

void BatchedBuffer::write(const IOVec* iov, uint32 iovcnt) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    for(uint32 i = 0; i < iovcnt; i++)
        write(iov[i].base, iov[i].len);
}

void BatchedBuffer::flush() {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    if (filling == NULL)
        return;

    batches.push_back(filling);
    filling = NULL;
}

// write the buffer to an ostream
void BatchedBuffer::store(FILE* os) {
    boost::lock_guard<boost::recursive_mutex> lck(mMutex);

    std::deque<ByteBatch*> bufs;
    batches.swap(bufs);

    for(std::deque<ByteBatch*>::iterator it = bufs.begin(); it != bufs.end(); it++) {
        ByteBatch* bb = *it;
        fwrite((void*)&(bb->items[0]), 1, bb->size, os);
        delete bb;
    }
}

} // namespace Sirikata
