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
 *  pay for one additional heap allocation to avoid .
 *
 *  To use, inherit from Liveness, pass livenessToken() as an extra
 *  parameter to your callbacks, and check the token's validity as the first
 *  operation in the callback, e.g.
 *
 *  void MyClass::my_callback(Liveness::Token alive) {
 *    if (!alive) return;
 *  }
 *
 *  Note that this, of course, does not protect you against multithreading,
 *  e.g. if the object is valid at the beginning of the callback but deleted by
 *  another thread during its execution. In those cases, you should use the
 *  SelfWeakPtr approach.
 */
class SIRIKATA_EXPORT Liveness {
    typedef std::tr1::shared_ptr<int> StrongInternalToken;
    typedef std::tr1::weak_ptr<int> InternalToken;

  public:
    struct Token {
        InternalToken mData;

        Token(InternalToken t);
        operator bool() const { return mData.lock(); }
    };

    Liveness();

    Token livenessToken() const { return Token(mLivenessStrongToken); }

  private:
    StrongInternalToken mLivenessStrongToken;
};

}

#endif //_SIRIKATA_CORE_UTIL_LIVENESS_TOKEN_HPP_
