/*  Sirikata - Mono Embedding
 *  MonoPropertyLookupCache.cpp
 *
 *  Copyright (c) 2009, Stanford University
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
#include <sirikata/oh/Platform.hpp>
#include "MonoPropertyLookupCache.hpp"

namespace Mono {

PropertyLookupCache::~PropertyLookupCache() {
}

SinglePropertyLookupCache::SinglePropertyLookupCache()
 : mDestClass(NULL),
   mResolvedGetMethod(NULL),
   mResolvedSetMethod(NULL)
{
}

SinglePropertyLookupCache::~SinglePropertyLookupCache() {
}

MonoMethod* SinglePropertyLookupCache::lookupGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) {
    if (dest_class == mDestClass)
        return mResolvedGetMethod;
    return NULL;
}

MonoMethod* SinglePropertyLookupCache::lookupSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) {
    if (dest_class == mDestClass)
        return mResolvedSetMethod;
    return NULL;
}

void SinglePropertyLookupCache::updateGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) {
    mDestClass = dest_class;
    mResolvedGetMethod = resolved;
    mResolvedSetMethod = NULL;
}

void SinglePropertyLookupCache::updateSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) {
    mDestClass = dest_class;
    mResolvedGetMethod = NULL;
    mResolvedSetMethod = resolved;
}



ThreadSafeSinglePropertyLookupCache::ThreadSafeSinglePropertyLookupCache()
 : mDestClass(NULL),
   mResolvedGetMethod(NULL),
   mResolvedSetMethod(NULL)
{
}

ThreadSafeSinglePropertyLookupCache::~ThreadSafeSinglePropertyLookupCache() {
}

MonoMethod* ThreadSafeSinglePropertyLookupCache::lookupGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) {
    if (dest_class == mDestClass)
        return mResolvedGetMethod;
    return NULL;
}

MonoMethod* ThreadSafeSinglePropertyLookupCache::lookupSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) {
    if (dest_class == mDestClass)
        return mResolvedSetMethod;
    return NULL;
}

void ThreadSafeSinglePropertyLookupCache::updateGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) {
    boost::mutex::scoped_lock updateLock(mMutex, boost::try_to_lock_t());
    if (!updateLock.owns_lock())
        return;

    mDestClass = dest_class;
    mResolvedGetMethod = resolved;
    mResolvedSetMethod = NULL;
}

void ThreadSafeSinglePropertyLookupCache::updateSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) {
    boost::mutex::scoped_lock updateLock(mMutex, boost::try_to_lock_t());
    if (!updateLock.owns_lock())
        return;

    mDestClass = dest_class;
    mResolvedGetMethod = NULL;
    mResolvedSetMethod = resolved;
}


} // namespace Mono
