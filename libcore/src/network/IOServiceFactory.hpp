/*  Sirikata Network Utilities
 *  IOServiceFactory.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _SIRIKATA_IOSERVICEFACTORY_HPP_
#define _SIRIKATA_IOSERVICEFACTORY_HPP_

namespace Sirikata {
namespace Network {

class IOService;
class BoostAsioDeadlineTimer;

class SIRIKATA_EXPORT TimerHandle {
    BoostAsioDeadlineTimer *mTimer;
    std::tr1::function<void()> mFunc;
    class TimedOut;
public:
    TimerHandle(IOService *io);

    TimerHandle(IOService *io, const std::tr1::function<void()>&f);

    void wait(const std::tr1::shared_ptr<TimerHandle> &thisPtr,
              const Duration &num_seconds);

    void wait(const std::tr1::shared_ptr<TimerHandle> &thisPtr,
              const Duration &num_seconds,
              const std::tr1::function<void()>&f) {
        setCallback(f);
        wait(thisPtr, num_seconds);
    }

    void setCallback(const std::tr1::function<void()>&f) {
        mFunc = f;
    }

    ~TimerHandle();

    void cancel();
};

typedef std::tr1::shared_ptr<TimerHandle> TimerHandlePtr;
typedef std::tr1::weak_ptr<TimerHandle> TimerHandleWPtr;


class SIRIKATA_EXPORT IOServiceFactory {
    static void io_service_initializer(IOService*io_ret);
  public:
    static IOService* makeIOService();
    static void destroyIOService(IOService*io);
    static IOService& singletonIOService();
    static std::size_t pollService(IOService*);
    static std::size_t runService(IOService*);
    static std::size_t pollOneService(IOService*);
    static std::size_t runOneService(IOService*);
    static void stopService(IOService*);
    static void resetService(IOService*);
    static void dispatchServiceMessage(IOService*,const std::tr1::function<void()>&f);
    static void postServiceMessage(IOService*,const std::tr1::function<void()>&f);
    static void postServiceMessage(IOService*,const Duration& waitFor, const std::tr1::function<void()>&f);
};

} // namespace Network
} // namespace Sirikata

#endif
