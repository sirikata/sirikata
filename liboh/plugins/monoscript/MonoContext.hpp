/*  Sirikata
 *  MonoContext.hpp
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
