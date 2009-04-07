/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  Array.hpp
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

#ifndef _SIRIKATA_ARRAY_HPP_
#define _SIRIKATA_ARRAY_HPP_

#include <cstddef>
#include <stdexcept>

namespace Sirikata {
template <class T, std::size_t N, bool integral_type=true> class Array {
public:
    enum {static_size=N};
    static std::size_t size() {return N;}
private:
    T mElems[static_size];
public:
    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;
    template <typename InputIterator> Array* initialize(InputIterator begin,
                                                        InputIterator end) {
        std::size_t i=0;
        for (;i<N&&begin!=end; ++begin,++i) {
            mElems[i]=*begin;
        }
        if (i!=static_size) {
            throw std::invalid_argument("Array<> constructor not passed valid number of input arguments");
        }
        return this;
    }
    Array* memcpy(const void *input, std::size_t len) {
        if (len==(static_size*sizeof(T))) {
            for (std::size_t i=0;i<N;++i) {
                mElems[i]=((T*)input)[i];
            }
        }else {
            throw std::invalid_argument("Array<> constructor not passed valid length of input memory");
        }
        return this;
    }
    iterator begin() { return mElems; }
    const_iterator begin() const { return mElems; }
    iterator end() { return mElems+static_size; }
    const_iterator end() const { return mElems+static_size; }
    reference operator[](size_type i) {
        return mElems[i];
    }
    const_reference operator[](size_type i) const {
        return mElems[i];
    }
    reference front() {
        return mElems[0];
    }
    const_reference front() const {
        return mElems[0];
    }
    reference back() {
        return mElems[static_size-1];
    }
    const_reference back() const {
        return mElems[static_size-1];
    }
    const T* data() const { return mElems; }
    T* data() { return mElems; }
    static const Array& nil() {
        static Array nix;
        static unsigned char nothing[static_size*sizeof(T)]={0};
        static Array nil=*nix.memcpy(nothing,static_size);
        return nil;
    }
    template <class InputIterator> static Array construct(InputIterator begin,
                                                          InputIterator end) {
        Array retval;
        retval.initialize(begin,end);
        return retval;
    }
    template <class InputIterator> static Array construct(InputIterator begin) {
        Array retval;
        retval.initialize(begin,begin+static_size);
        return retval;
    }
    bool operator <(const Array &other)const {
        if (integral_type) {
            return std::memcmp(mElems,other.mElems,static_size*sizeof(T))<0;
        }else {
            for (size_t i=0;i<static_size;++i) {
                if (mElems[i]<other.mElems[i]) return true;
                if (!(mElems[i]==other.mElems[i])) return false;
            }
            return false;
        }
    }
    bool operator ==(const Array &other)const {
        if (integral_type) {
            return std::memcmp(mElems,other.mElems,static_size*sizeof(T))==0;
        }else {
            for (size_t i=0;i<static_size;++i) {
                if (!(mElems[i]==other.mElems[i])) return false;
            }
            return true;
        }
    }
    bool operator !=(const Array &other)const {
        return !(*this==other);
    }
};

}

#endif //_SIRIKATA_ARRAY_HPP_
