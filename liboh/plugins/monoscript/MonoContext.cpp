/*  Sirikata
 *  MonoContext.cpp
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/UUID.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include "MonoDomain.hpp"
#include "MonoObject.hpp"
#include "MonoContext.hpp"

AUTO_SINGLETON_INSTANCE(Sirikata::MonoContext);

namespace Sirikata {

MonoContextData::MonoContextData()
 : CurrentDomain(Mono::Domain::root()),
   Object()
{
}

MonoContextData::MonoContextData(const Mono::Domain& domain, HostedObjectPtr vwobj)
 : CurrentDomain(domain),
   Object(vwobj)
{
}

MonoContext&MonoContext::getSingleton(){
    return AutoSingleton<MonoContext>::getSingleton();
}
void MonoContext::initializeThread() {
    assert( mThreadContext.get() == NULL );

    ContextStack* ctx_stack = new ContextStack();
    ctx_stack->push( MonoContextData() );
    mThreadContext.reset( ctx_stack );

    assert( mThreadContext.get() != NULL );
}

MonoContextData& MonoContext::current() {
    ContextStack* ctx_stack = mThreadContext.get();
    assert( ctx_stack != NULL );
    assert( !ctx_stack->empty() );
    return ctx_stack->top();
}

const MonoContextData& MonoContext::current() const {
    ContextStack* ctx_stack = mThreadContext.get();
    assert( ctx_stack != NULL );
    assert( !ctx_stack->empty() );
    return ctx_stack->top();
}

void MonoContext::set(const MonoContextData& val) {
    ContextStack* ctx_stack = mThreadContext.get();
    assert( ctx_stack != NULL );
    assert( !ctx_stack->empty() );
    ctx_stack->top() = val;
}

void MonoContext::push(const MonoContextData& val) {
    ContextStack* ctx_stack = mThreadContext.get();
    assert( ctx_stack != NULL );
    ctx_stack->push(val);
}

void MonoContext::push() {
    ContextStack* ctx_stack = mThreadContext.get();
    assert( ctx_stack != NULL );
    assert( !ctx_stack->empty() );
    ctx_stack->push( ctx_stack->top() );
}

void MonoContext::pop() {
    ContextStack* ctx_stack = mThreadContext.get();
    assert( ctx_stack != NULL );
    assert( !ctx_stack->empty() );
    ctx_stack->pop();
}


std::tr1::shared_ptr<HostedObject> MonoContext::getVWObject() const {
    return current().Object.lock();
}
Mono::Domain& MonoContext::getDomain() const {
    return *const_cast<Mono::Domain*>(&current().CurrentDomain);
}

Sirikata::UUID MonoContext::getUUID() const {
    //FIXME: returns null uuid
    
    return UUID::null();
}





} // namespace Mono
