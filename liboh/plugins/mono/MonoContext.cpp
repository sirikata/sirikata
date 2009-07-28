#include "oh/Platform.hpp"
#include "util/UUID.hpp"
#include "MonoContext.hpp"
#include "MonoDomain.hpp"
#include "MonoObject.hpp"
AUTO_SINGLETON_INSTANCE(Sirikata::MonoContext);

namespace Sirikata {


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


UUID MonoContext::getVWObject() const {
    return current().Object;
}

void MonoContext::setVWObject(const UUID& vwobj) {
    current().Object = vwobj;
}


Sirikata::UUID MonoContext::getUUID() const {
    UUID vwobj = getVWObject();
    //FIXME
    return vwobj;
}



static MonoObject* Mono_Context_CurrentUUID() {
    UUID uuid = MonoContext::getSingleton().getUUID();
    Mono::Object obj = Mono::Domain::root().UUID(uuid);
    return obj.object();
}

static MonoObject* Mono_Context_CurrentObject() {
    Mono::Object obj =  Mono::Domain::root().UUID(MonoContext::getSingleton().getVWObject());
    return obj.object();
}


void MonoContextInit() {
    mono_add_internal_call ("Sirikata.Runtime.Context::GetCurrentUUID", (void*)Mono_Context_CurrentUUID);
    mono_add_internal_call ("Sirikata.Runtime.Context::GetCurrentObject", (void*)Mono_Context_CurrentObject);
}

} // namespace Mono
