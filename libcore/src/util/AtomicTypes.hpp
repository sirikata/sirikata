/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  AtomicTypes.hpp
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

#ifndef _SIRIKATA_ATOMIC_TYPES_HPP_
#define _SIRIKATA_ATOMIC_TYPES_HPP_

#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#endif
namespace Sirikata {

#ifdef _WIN32
template <int size> class SizedAtomicValue {

};

template<> class SizedAtomicValue<4> {
public:
    template<typename T> static T add(volatile T*scalar, T other) {
        return other + (T)InterlockedExchangeAdd((volatile LONG*)scalar,(int32) other);
    }
    template<typename T> static T inc(volatile T*scalar) {
        return (T)InterlockedIncrement((volatile LONG*)scalar);
    }
    template<typename T> static T dec(volatile T*scalar) {
        return (T)InterlockedDecrement((volatile LONG*)scalar);
    }
};
template<> class SizedAtomicValue<8> {
public:
    template<typename T> static T add(volatile T*scalar, T other) {
        return other + (T)InterlockedExchangeAdd64((volatile LONGLONG*)scalar,(LONGLONG) other);
    }
    template<typename T> static T inc(volatile T*scalar) {
        return (T)InterlockedIncrement64((volatile LONGLONG*)scalar);
    }
    template<typename T> static T dec(volatile T*scalar) {
        return (T)InterlockedDecrement64((volatile LONGLONG*)scalar);
    }
};
#elif defined(__APPLE__)
template<int size> class SizedAtomicValue {

};

template<> class SizedAtomicValue<4> {
public:
    template <typename T> static T add(volatile T* scalar,T other) {
        return (T)OSAtomicAdd32((int32)other, (int32*)scalar);
    }
    template <typename T> static T inc(volatile T*scalar) {
        return (T)OSAtomicIncrement32((int32*)scalar);
    }
    template <typename T> static T dec(volatile T*scalar) {
        return (T)OSAtomicDecrement32((int32*)scalar);
    }
};

template<> class SizedAtomicValue<8> {
public:
    template <typename T> static T add(volatile T* scalar, T other) {
        return (T)OSAtomicAdd64((int64)other, (int64*)scalar);
    }
    template <typename T> static T inc(volatile T*scalar) {
        return (T)OSAtomicIncrement64((int64*)scalar);
    }
    template <typename T> static T dec(volatile T*scalar) {
        return (T)OSAtomicDecrement64((int64*)scalar);
    }
};
#else
template<int size> class SizedAtomicValue {
public:
    template <typename T> static T add(volatile T*scalar, T other) {
        return __sync_add_and_fetch(scalar, other);
    }
    template <typename T> static T inc(volatile T*scalar) {
        return __sync_add_and_fetch(scalar, 1);
    }
    template <typename T> static T dec(volatile T*scalar) {
        return __sync_sub_and_fetch(scalar, 1);
    }
};
#endif
#ifdef _WIN32
#pragma warning( push )
#pragma warning (disable : 4312)
#pragma warning (disable : 4197)
#endif
template <typename T>
class AtomicValue {
private:
    volatile char mMemory[sizeof(T)+(sizeof(T)==4?4:(sizeof(T)==2?2:(sizeof(T)==8?8:16)))];
    static volatile T*getThisAlignedAddress(volatile char* data) {
        size_t bitandammt=sizeof(T)==4?3:(sizeof(T)==2?1:(sizeof(T)==8?7:15));
        size_t notbitandammt=~bitandammt;
        return (volatile T*)((((size_t)data)+bitandammt)&notbitandammt);
    }
    static volatile const T*getThisAlignedAddress(volatile const char* data) {
        size_t bitandammt=sizeof(T)==4?3:(sizeof(T)==2?1:(sizeof(T)==8?7:15));
        size_t notbitandammt=~bitandammt;
        return (volatile const T*)((((size_t)data)+bitandammt)&notbitandammt);
    }
public:
    AtomicValue() {
    }
    explicit AtomicValue (T other) {
        *(T*)getThisAlignedAddress(mMemory)=other;
    }
    AtomicValue(const AtomicValue&other) {
        *(T*)getThisAlignedAddress(mMemory)=*(T*)getThisAlignedAddress(other.mMemory);
    }
    const AtomicValue<T>& operator =(T other) {
        *(T*)getThisAlignedAddress(mMemory)=other;
        return *this;
    }
    const AtomicValue& operator =(const AtomicValue& other) {
        *(T*)getThisAlignedAddress(mMemory)=*(T*)getThisAlignedAddress(other.mMemory);
        return *this;
    }
    bool operator ==(T other) const{
        return *(T*)getThisAlignedAddress(mMemory)==other;
    }
    bool operator ==(const AtomicValue& other) const{
        return *(T*)getThisAlignedAddress(mMemory)==*(T*)getThisAlignedAddress(other.mMemory);
    }
    operator T ()const {
        return *(T*)getThisAlignedAddress(mMemory);
    }
    T read() const {
        return *(T*)getThisAlignedAddress(mMemory);
    }
    T operator +=(const T&other) {
        return SizedAtomicValue<sizeof(T)>::add(getThisAlignedAddress(mMemory),other);
    }
    T operator -=(const T&other) {
        T temp=0;
        temp-=other;//to avoid unsigned unary operator problems, just wrap
        return *this+=temp;
    }
    T operator ++() {
        return (T)SizedAtomicValue<sizeof(T)>::inc(getThisAlignedAddress(mMemory));
    }
    T operator --() {
        return (T)SizedAtomicValue<sizeof(T)>::dec(getThisAlignedAddress(mMemory));
    }
    T operator++(int) {
        return (++*this)-(T)1;
    }
    T operator--(int) {
        return (--*this)+(T)1;
    }
};

template <class Node>
inline bool compare_and_swap(volatile Node*volatile *target, volatile Node *comperand, volatile Node * exchange){
#ifdef _WIN32
        return InterlockedCompareExchangePointer((volatile PVOID*)target, (volatile PVOID)exchange, (volatile PVOID)comperand)==comperand;
#else
#ifdef __APPLE__
        if (sizeof(exchange)==4) {
            return OSAtomicCompareAndSwap32((int32_t)comperand, (int32_t)exchange, (int32_t*)target);
        }else {
            return OSAtomicCompareAndSwap64((int64_t)comperand, (int64_t)exchange, (int64_t*)target);
        }
#else
        return __sync_bool_compare_and_swap (target, comperand, exchange);
#endif
#endif
}

#ifdef _WIN32
#pragma warning( pop )
#endif
}
#endif //_SIRIKATA_ATOMIC_TYPES_HPP_
