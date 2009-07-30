/*  Sirikata - Mono Embedding
 *  MonoDelegate.cpp
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
#include "oh/Platform.hpp"
#include "MonoObject.hpp"
class MonoContextData;
#include "oh/SpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "MonoDomain.hpp"
#include "MonoDelegate.hpp"
#include "MonoMethodLookupCache.hpp"

namespace Mono {

Delegate::Delegate() {
}

Delegate::Delegate(const Object& delobj)
    : mDelegateObj(delobj)
{
}

bool Delegate::null() const {
    return mDelegateObj.null();
}

Object Delegate::invoke() const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke");
}

Object Delegate::invoke(const Object& p1) const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke", p1);
}

Object Delegate::invoke(const Object& p1, const Object& p2) const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke", p1, p2);
}

Object Delegate::invoke(const Object& p1, const Object& p2, const Object& p3) const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke", p1, p2, p3);
}

Object Delegate::invoke(const Object& p1, const Object& p2, const Object& p3, const Object& p4) const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke", p1, p2, p3, p4);
}

Object Delegate::invoke(const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke", p1, p2, p3, p4, p5);
}

Object Delegate::invoke(const std::vector<Object>& args) const {
    static ThreadSafeSingleMethodLookupCache invoke_cache;
    return mDelegateObj.send(&invoke_cache, "Invoke", args);
}


} // namespace Mono



namespace Sirikata {

ContextualMonoDelegate::ContextualMonoDelegate()
    : Mono::Delegate(),
     mContext()
{
}

ContextualMonoDelegate::ContextualMonoDelegate(const Mono::Object& delobj)
    : Mono::Delegate(delobj),
     mContext( MonoContext::getSingleton().current() )
{
}

ContextualMonoDelegate::ContextualMonoDelegate(const Mono::Object& delobj, const MonoContextData& ctx)
    : Mono::Delegate(delobj),
     mContext( ctx )
{
}

void ContextualMonoDelegate::pushContext() const {
    MonoContext::getSingleton().push(mContext);
}

void ContextualMonoDelegate::popContext() const {
    MonoContext::getSingleton().pop();
}


Mono::Object ContextualMonoDelegate::invoke() const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke");
    popContext();
    return rv;
}

Mono::Object ContextualMonoDelegate::invoke(const Mono::Object& p1) const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke", p1);
    popContext();
    return rv;
}

Mono::Object ContextualMonoDelegate::invoke(const Mono::Object& p1, const Mono::Object& p2) const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke", p1, p2);
    popContext();
    return rv;
}

Mono::Object ContextualMonoDelegate::invoke(const Mono::Object& p1, const Mono::Object& p2, const Mono::Object& p3) const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke", p1, p2, p3);
    popContext();
    return rv;
}

Mono::Object ContextualMonoDelegate::invoke(const Mono::Object& p1, const Mono::Object& p2, const Mono::Object& p3, const Mono::Object& p4) const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke", p1, p2, p3, p4);
    popContext();
    return rv;
}

Mono::Object ContextualMonoDelegate::invoke(const Mono::Object& p1, const Mono::Object& p2, const Mono::Object& p3, const Mono::Object& p4, const Mono::Object& p5) const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke", p1, p2, p3, p4, p5);
    popContext();
    return rv;
}

Mono::Object ContextualMonoDelegate::invoke(const std::vector<Mono::Object>& args) const {
    pushContext();
    static Mono::ThreadSafeSingleMethodLookupCache invoke_cache;
    Mono::Object rv = mDelegateObj.send(&invoke_cache, "Invoke", args);
    popContext();
    return rv;
}

} // namespace Sirikata
