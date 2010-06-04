/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  MemoryReference.hpp
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

#ifndef _SIRIKATA_MEMORY_REFERENCE_HPP_
#define _SIRIKATA_MEMORY_REFERENCE_HPP_

#include <cstddef>
#include <stdexcept>

namespace Sirikata {
template <class T> class DataReference {
    T first;
    size_t second;
public:
    DataReference(T data, size_t size) {
        first=data;
        second=size;
    }
    static DataReference null() {
        return DataReference(NULL,0);
    }
    explicit DataReference(const std::string&s) {
        first=s.data();
        second=s.length();
    }
    template <class U> explicit DataReference(const std::vector<U> &v) {
        if (v.empty()) {
            first=NULL;
            second=0;
        }else {
            first=(T)&v[0];
            second=v.size();
        }
    }
    T begin() const {
        return first;
    }
    T end() const {
        // FIXME: If second really is measured in bytes, why do we use void* and not char*?
        return (void*)(((char*)first)+second);
    }
    T data() const {
        return first;
    }
    size_t size() const {
        return second;
    }
    inline size_t length() const {
        return size();
    }
};

typedef DataReference<const void*> MemoryReference;
}
#endif //_SIRIKATA_ARRAY_HPP_
