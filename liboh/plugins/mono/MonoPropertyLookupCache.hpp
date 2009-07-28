/*  Sirikata - Mono Embedding
 *  MonoPropertyLookupCache.hpp
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
#ifndef _MONO_PROPERTY_LOOKUP_CACHE_
#define _MONO_PROPERTY_LOOKUP_CACHE_

#include <mono/metadata/metadata.h>
#include <mono/metadata/object.h>

#include <boost/thread.hpp>

namespace Mono {

/** Interface for property lookup caches.  This interface doesn't guarantee
 *  much about the behavior of the implementation.  Whether various parts
 *  of the call (receiving type, message name, argument types) are checked
 *  before returning a cached value is dependent on the context and should
 *  be handled by subclasses.
 */
class PropertyLookupCache {
public:
    virtual ~PropertyLookupCache();

    virtual MonoMethod* lookupGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) = 0;
    virtual MonoMethod* lookupSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) = 0;
    virtual void updateGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) = 0;
    virtual void updateSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) = 0;
};

/** Specialization of PropertyLookupCache for a single entry cache that only
 *  checks receiver type.  This should be used in scenarios where you are always
 *  using the same property and know the exact types of objects because you
 *  construct them yourself.
 */
class SinglePropertyLookupCache : public PropertyLookupCache {
public:
    SinglePropertyLookupCache();
    virtual ~SinglePropertyLookupCache();

    virtual MonoMethod* lookupGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs);
    virtual MonoMethod* lookupSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs);
    virtual void updateGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved);
    virtual void updateSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved);

private:
    MonoClass* mDestClass;
    MonoMethod* mResolvedGetMethod;
    MonoMethod* mResolvedSetMethod;
};

/** A thread safe version of SinglePropertyLookupCache. Cache updates only occur
 *  if the mutex can be immediately locked in order to avoid long latency cache
 *  updates.
 */
class ThreadSafeSinglePropertyLookupCache : public PropertyLookupCache {
public:
    ThreadSafeSinglePropertyLookupCache();
    virtual ~ThreadSafeSinglePropertyLookupCache();

    virtual MonoMethod* lookupGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs);
    virtual MonoMethod* lookupSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs);
    virtual void updateGet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved);
    virtual void updateSet(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved);

private:
    boost::mutex mMutex;
    MonoClass* mDestClass;
    MonoMethod* mResolvedGetMethod;
    MonoMethod* mResolvedSetMethod;
};

} // namespace Mono

#endif //_MONO_METHOD_LOOKUP_CACHE_
