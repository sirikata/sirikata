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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/service/Signal.hpp>

#include <sirikata/core/util/Platform.hpp>

#if SIRIKATA_PLATFORM == PLATFORM_LINUX
#include <signal.h>
#else
// Currently we don't handle signals on other platforms.
#endif

namespace Sirikata {
namespace Signal {

namespace {
typedef std::map<HandlerID, Handler> SignalHandlerMap;
SignalHandlerMap sSignalHandlers;
int32 sNextHandlerID = 0;

#if SIRIKATA_PLATFORM == PLATFORM_LINUX
void handle_signal(int signum) {
    // Reregister
    signal(signum, handle_signal);
    // Invoke handlers
    Type sigtype;
    switch(signum) {
      case SIGINT: sigtype = INT; break;
      case SIGHUP: sigtype = HUP; break;
      case SIGABRT: sigtype = ABORT; break;
      case SIGTERM: sigtype = TERM; break;
      case SIGKILL: sigtype = KILL; break;
      default: break;
    }
    for(SignalHandlerMap::iterator it = sSignalHandlers.begin(); it != sSignalHandlers.end(); it++)
        it->second(sigtype);
}
#endif

}

HandlerID registerHandler(Handler handler) {
    bool was_empty = sSignalHandlers.empty();

    HandlerID id = sNextHandlerID++;
    sSignalHandlers[id] = handler;

#if SIRIKATA_PLATFORM == PLATFORM_LINUX
    if (was_empty) {
        signal(SIGINT, handle_signal);
        signal(SIGHUP, handle_signal);
        signal(SIGABRT, handle_signal);
        signal(SIGTERM, handle_signal);
        signal(SIGKILL, handle_signal);
    }
#endif

    return id;
}

void unregisterHandler(HandlerID& handler) {
    sSignalHandlers.erase(handler);

#if SIRIKATA_PLATFORM == PLATFORM_LINUX
    if (sSignalHandlers.empty()) {
        signal(SIGINT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGKILL, SIG_DFL);
    }
#endif
}


} // namespace Signal
} // namespace Sirikata
