/*  Sirikata - Mono Embedding
 *  MonoMethodLookupCache.hpp
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
#ifndef _MONO_METHOD_LOOKUP_CACHE_
#define _MONO_METHOD_LOOKUP_CACHE_

#include <mono/metadata/metadata.h>
#include <mono/metadata/object.h>

#include <boost/thread.hpp>

namespace Mono {

/** Interface for method lookup caches.  This interface doesn't guarantee
 *  much about the behavior of the implementation.  Whether various parts
 *  of the call (receiving type, message name, argument types) are checked
 *  before returning a cached value is dependent on the context and should
 *  be handled by subclasses.
 */
class MethodLookupCache {
public:
    virtual ~MethodLookupCache();

    virtual MonoMethod* lookup(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs) = 0;
    virtual void update(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved) = 0;
};

/** Specialization of MethodLookupCache for a single entry cache that only
 *  checks receiver type.  This should be used in scenarios where you are always
 *  sending the same message and know the exact types of objects because you
 *  construct them yourself, e.g. your cacheless code looks like:
 *
 *    Mono::Object arg1 = Mono::Domain::root().Int32(input_arg1);
 *    Mono::Object arg2 = Mono::Domain::root().String(input_arg2);
 *    destObject.send("MyMethod", arg1, arg2);
 */
class SingleMethodLookupCache : public MethodLookupCache {
public:
    SingleMethodLookupCache();
    virtual ~SingleMethodLookupCache();

    virtual MonoMethod* lookup(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs);
    virtual void update(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved);

private:
    MonoClass* mDestClass;
    MonoMethod* mResolvedMethod;
};

/** A thread safe version of SingleMethodLookupCache. Cache updates only occur
 *  if the mutex can be immediately locked in order to avoid long latency cache
 *  updates.
 */
class ThreadSafeSingleMethodLookupCache : public MethodLookupCache {
public:
    ThreadSafeSingleMethodLookupCache();
    virtual ~ThreadSafeSingleMethodLookupCache();

    virtual MonoMethod* lookup(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs);
    virtual void update(MonoClass* dest_class, const char* message, MonoObject* args[], int nargs, MonoMethod* resolved);

private:
    boost::mutex mMutex;
    MonoClass* mDestClass;
    MonoMethod* mResolvedMethod;
};

} // namespace Mono

#endif //_MONO_METHOD_LOOKUP_CACHE_
