/*  Sirikata
 *  Signal.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_CORE_SERVICE_SIGNAL_HPP_
#define _SIRIKATA_CORE_SERVICE_SIGNAL_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {

/** Provides utilities for interacting with signals. These are all static since
 *  signals are program wide.  For each type of signal we maintain a set of
 *  listeners and invoke them on the signal.
 */
namespace Signal {

enum Type {
    INT,
    HUP,
    ABORT,
    TERM,
    KILL
};

typedef std::tr1::function<void(Type)> Handler;
typedef int32 HandlerID;

SIRIKATA_EXPORT HandlerID registerHandler(Handler handler);
SIRIKATA_EXPORT void unregisterHandler(HandlerID& handler);

SIRIKATA_EXPORT String typeAsString(Type t);

} // namespace Signal
} // namespace Sirikata

#endif //_SIRIKATA_CORE_SERVICE_SIGNAL_HPP_
