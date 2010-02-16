#ifndef _MONO_CONTEXT_HPP_
#define _MONO_CONTEXT_HPP_

#include <boost/thread/tss.hpp>
#include <stack>

namespace Sirikata {

class MonoContext;

/** This is the storage class for MonoContext data.  This is
 *  made available so copies of context data can be carried
 *  around and maintained across calls.  However, data is only
 *  made accessible when the data is loaded into a current
 *  context.
 */
class MonoContextData {
private:
    friend class MonoContext;
    Mono::Domain CurrentDomain;
    HostedObjectWPtr Object;
public:
    MonoContextData();
    MonoContextData(const Mono::Domain& domain, HostedObjectPtr vwobj);
};


/** Represents some current execution state within Mono.
 *  This is a singleton because this maintains per thread
 *  state, so there is no point in a non-singleton version.
 *  This can easily serve multiple object hosts.  A stack of
 *  context data is maintained.
 */
class MonoContext : public Sirikata::AutoSingleton<MonoContext> {
public:

    /** Initialize this thread with an empty context.
     *  All context state will have default values.
     */
    void initializeThread();
    static  MonoContext&getSingleton();
    /** Get the current context data. */
    MonoContextData& current();
    const MonoContextData& current() const;

    /** Set the current context data. */
    void set(const MonoContextData& val);

    /** Push the specified context onto the top of the stack. */
    void push(const MonoContextData& val);

    /** Push a copy of the current state onto the stack. */
    void push();

    /** Pop the top context data off the stack. */
    void pop();

    /** Get the current VWObject being called.
     *  May return a null object.
     */
    HostedObjectPtr getVWObject() const;

    /**
     * Get the domain the object was allocated and is running under
     */
    Mono::Domain& getDomain() const;
    /** Get the UUID of the current VWObject
     *  being called.  If the object is null,
     *  then a null UUID will be returned.
     */
    Sirikata::UUID getUUID() const;

private:
    typedef std::stack<MonoContextData> ContextStack;
    boost::thread_specific_ptr<ContextStack> mThreadContext;
}; // class Context




} // namespace Sirikata

#endif //_MONO_CONTEXT_HPP_
