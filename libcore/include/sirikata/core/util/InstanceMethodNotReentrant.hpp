// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_CORE_UTIL_INSTANCE_METHOD_NOT_REENTRANT_HPP_
#define _SIRIKATA_CORE_UTIL_INSTANCE_METHOD_NOT_REENTRANT_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {

/** A utility class that lets you specify that a method isn't reentrant and
 *  validate in debug mode that you're not calling it recursively. It checks
 *  reentrancy per instance, i.e. it is expected that it is reentrant for
 *  different instances (e.g. you might have class A and have instance A1::foo
 *  call instance A2::foo and be fine, but A1::foo eventually hitting A1::foo
 *  again on the same stack is not ok).
 *
 *  This might seem simple, but in cases where you are using listeners or
 *  generic callbacks it might be hard to verify statically that you don't
 *  accidentally have recursion. This lets you just annotate a bit of code to
 *  make sure it remains true. In release mode all checks are disabled and the
 *  class uses no space.
 *
 *  Note that you should use one of these *per non-reentrant method*. If you
 *  have A::foo and A::bar, you need one of these for each one.
 *
 *  To use the class, you want a declaraction like
 *    InstanceMethodNotReentrant mFooNotRentrant;
 *  in your class and then create a token in the method, e.g.
 *    void A::foo() {
 *      InstanceMethodNotReentrant::Token not_reentrant(mFooNotRentrant);
 *    }
 *
 */
class SIRIKATA_EXPORT InstanceMethodNotReentrant {
public:
    InstanceMethodNotReentrant()
        : count(0)
    {}

    class Token {
    public:
        Token(InstanceMethodNotReentrant& par)
#if SIRIKATA_DEBUG
         : parent(par)
#endif
        {
#if SIRIKATA_DEBUG
            assert(parent.count == 0);
            parent.count++;
#endif
        }
        ~Token() {
#if SIRIKATA_DEBUG
            parent.count--;
#endif
        }

    private:
#if SIRIKATA_DEBUG
        InstanceMethodNotReentrant& parent;
#endif
    };

#if SIRIKATA_DEBUG
    char count;
#endif
}; // class InstanceMethodNotReentrant

} // namespace Sirikata


#endif //_SIRIKATA_CORE_UTIL_INSTANCE_METHOD_NOT_REENTRANT_HPP_
