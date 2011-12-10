/*  Sirikata
 *  Liveness.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#ifndef _SIRIKATA_CORE_UTIL_LIVENESS_TOKEN_HPP_
#define _SIRIKATA_CORE_UTIL_LIVENESS_TOKEN_HPP_

namespace Sirikata {

/** Liveness makes it simple to track whether an object is still alive when
 *  using asynchronous callbacks. A common problem we encounter is that
 *  callbacks to member functions need to be handled carefully: we need to be
 *  sure a callback is not invoked after an object is deleted. One way to do
 *  this is to force the class to be used through a shared_ptr and use the
 *  shared_ptr in the callback. A better approach is to use a weak_ptr in the
 *  callback so the object can be cleaned up -- SelfWeakPtr helps you do just
 *  that.
 *
 *  But these approaches are bad for two reasons. First, they force you
 *  to change how you manage your class, which can be a burden if the class is
 *  already in use. Second, with weak_ptrs you need to wrap the callback with a
 *  stub that locks the weak_ptrs, checks validity, and then invokes the real
 *  callback.
 *
 *  Liveness avoids these problems by providing a token inside the object
 *  which is checked for validity. You pass the token through to your callback
 *  and check if it is valid at the top of the callback. The token is just a
 *  shared_ptr maintained internally and passed through the callback as a
 *  weak_ptr, the same strategy as the SelfWeakPtr approach. Essentially, you
 *  pay for one additional heap allocation.
 *
 *  To use, inherit from Liveness, pass livenessToken() as an extra
 *  parameter to your callbacks, and check the token's validity as the first
 *  operation in the callback, e.g.
 *
 *  void MyClass::my_callback(Liveness::Token alive) {
 *    if (!alive) return;
 *  }
 *
 *  Since you haven't accessed any data in the object, this is safe even if the
 *  this pointer to the MyClass instance has been deleted.
 *
 *  This basic approach doesn't protect you against multithreading: if the
 *  object is valid at the beginning of the callback but deleted by another
 *  thread during its execution, you'll end up accessing unallocated memory. In
 *  those cases, you can use a stronger primitive, a Liveness::Lock, which keeps
 *  the Liveness object from dying while you are using it: as long as there are
 *  Liveness::Lock objects which point to the Liveness object, it will block the
 *  thread trying to invalidate it. To support this, objects that inherit from
 *  Liveness call Liveness::letDie() when they want to disallow future access to
 *  themselves, e.g. because they are becoming invalid or being deleted. Then,
 *  the user of the object tries to allocate a lock, much like using
 *  weak_ptr::lock():
 *
 *  void MyClass::my_callback(Liveness::Token alive) {
 *    Liveness::Lock locked(alive);
 *    if (!locked) return;
 *  }
 *
 *  This is similar but not exactly the same as using SelfWeakPtr, and is
 *  especially useful as it doesn't force you to use shared_ptrs for all your
 *  classes.
 *
 *  Classes that inherit from Liveness *must* call letDie() and should be
 *  careful about when they call it. It *must* be called before any method calls
 *  could be invalid. Because virtual methods could become invalid during
 *  destruction, the absolute latest you can call it is in the destructor of the
 *  deepest subclass of Liveness. This means that you should be very careful
 *  about inheriting from other classes that themselves inherit from Liveness.
 */
class SIRIKATA_EXPORT Liveness {
    typedef std::tr1::shared_ptr<int> StrongInternalToken;
    typedef std::tr1::weak_ptr<int> InternalToken;

  public:
    struct SIRIKATA_EXPORT Token {
        InternalToken mData;

        Token(InternalToken t);
        operator bool() const { return mData.lock(); }
    };

    struct SIRIKATA_EXPORT Lock {
        StrongInternalToken mData;

        Lock(InternalToken t);
        Lock(const Token& t);
        operator bool() const { return mData; }
    };

    Liveness();
    ~Liveness();

    /** Get a token which can be used to determine whether the object is still
     *  alive or obtain a liveness lock on it.
     */
    Token livenessToken() const { return Token(mLivenessStrongToken); }

    /** Allow this object to "die", blocking until no other threads have access
     *  anymore to return.  You should call this before your object will become
     *  invalid for further calls -- definitely by the destructor of the
     *  inheriting class (when data and virtual methods will become invalid),
     *  but possibly earlier (e.g. when Service::stop() is called for the
     *  object).
     */
    void letDie();

  protected:
    /** Helper method to determine whether this object is still alive. Useful
     *  for classes that inherit from Liveness and also have subclasses, but can
     *  be valid on their own. For example if we have Liveness <- A <- B, then A
     *  might use this method to determine whether it is a B which has already
     *  called letDie() or if it's just a regular A and needs to call letDie()
     *  itself.
     */
    bool livenessAlive() const { return mLivenessStrongToken; }

  private:
    StrongInternalToken mLivenessStrongToken;
};

}

#endif //_SIRIKATA_CORE_UTIL_LIVENESS_TOKEN_HPP_
