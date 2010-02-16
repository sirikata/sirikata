#include "oh/Platform.hpp"
#include "util/UUID.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/HostedObject.hpp"
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
    std::tr1::shared_ptr<HostedObject> tmp=getVWObject();
    if (tmp)
        return tmp->getUUID();
    return UUID::null();
}





} // namespace Mono
