/*  Sirikata - Mono Embedding
 *  MonoDelegate.hpp
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
#ifndef _MONO_DELEGATE_HPP_
#define _MONO_DELEGATE_HPP_

#include "MonoObject.hpp"
#include "MonoContext.hpp"

namespace Mono {

class Delegate {
public:
    Delegate();
    explicit Delegate(const Object& delobj);

    /** Returns true if this object is a null reference. */
    bool null() const;

    /** Invoke the delegate. */
    Object invoke() const;
    Object invoke(const Object& p1) const;
    Object invoke(const Object& p1, const Object& p2) const;
    Object invoke(const Object& p1, const Object& p2, const Object& p3) const;
    Object invoke(const Object& p1, const Object& p2, const Object& p3, const Object& p4) const;
    Object invoke(const Object& p1, const Object& p2, const Object& p3, const Object& p4, const Object& p5) const;
    Object invoke(const std::vector<Object>& args) const;

protected:
    Object mDelegateObj;
}; //class Delegate

} // namespace Mono



namespace Sirikata {

/** A Mono Delegate the also captures context information
 *  and restores it upon invokation.
 */
class ContextualMonoDelegate : public Mono::Delegate {
public:
    /** Constructs a null ContextualMonoDelegate. */
    ContextualMonoDelegate();
    /** Constructs a ContextualMonoDelegate using the given Mono::Object
     *  as the delegate and the current MonoContext for this thread.
     *  \param delobj
     */
    explicit ContextualMonoDelegate(const Mono::Object& delobj);
    /** Constructs a ContextualMonoDelegate using the given Mono::Object
     *  as the delegate and the given ContextData as the context.
     */
    ContextualMonoDelegate(const Mono::Object& delobj, const MonoContextData& ctx);


    /** Invoke the delegate with the appropriate context. */
    Mono::Object invoke() const;
    Mono::Object invoke(const Mono::Object& p1) const;
    Mono::Object invoke(const Mono::Object& p1, const Mono::Object& p2) const;
    Mono::Object invoke(const Mono::Object& p1, const Mono::Object& p2, const Mono::Object& p3) const;
    Mono::Object invoke(const Mono::Object& p1, const Mono::Object& p2, const Mono::Object& p3, const Mono::Object& p4) const;
    Mono::Object invoke(const Mono::Object& p1, const Mono::Object& p2, const Mono::Object& p3, const Mono::Object& p4, const Mono::Object& p5) const;
    Mono::Object invoke(const std::vector<Mono::Object>& args) const;


protected:
    void pushContext() const;
    void popContext() const;

private:
    MonoContextData mContext;
}; // class MonoDelegate

} // namespace Sirikata

#endif //_MONO_DELEGATE_HPP_
