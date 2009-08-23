/*  Sirikata Network Utilities
 *  IOServiceFactory.cpp
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
#include "util/Standard.hh"
#include "TCPDefinitions.hpp"
#include "IOServiceFactory.hpp"
#include "util/Time.hpp"
namespace Sirikata { namespace Network {
namespace {
boost::once_flag io_singleton=BOOST_ONCE_INIT;
bool called_io_service=false;
}
void IOServiceFactory::io_service_initializer(IOService*io_ret){
    static IOService io;
    called_io_service=true;
    io_ret=&io;
}

IOService&IOServiceFactory::singletonIOService() {
    static IOService*io=NULL;
    boost::call_once(io_singleton,boost::bind(io_service_initializer,io));
    return *io;
}
IOService*IOServiceFactory::makeIOService() {
    return new IOService;
}
void IOServiceFactory::destroyIOService(IOService*io) {
    if (called_io_service==false)
        delete io;
    else if (&singletonIOService()!=io)
        delete io;
}

std::size_t IOServiceFactory::pollService(IOService*ios){
    return ios->poll();
}
std::size_t IOServiceFactory::runService(IOService*ios){
    return ios->run();
}
std::size_t IOServiceFactory::pollOneService(IOService*ios){
    return ios->poll_one();
}
std::size_t IOServiceFactory::runOneService(IOService*ios){
    return ios->run_one();
}
void IOServiceFactory::stopService(IOService*ios){
    ios->stop();
}
void IOServiceFactory::resetService(IOService*ios){
    ios->reset();
}
void IOServiceFactory::dispatchServiceMessage(IOService*ios,const std::tr1::function<void()>&f){
    ios->dispatch(f);
}
namespace {
void handle_deadline_timer(const boost::system::error_code&e,const std::tr1::shared_ptr<boost::asio::deadline_timer>&timer,const std::tr1::function<void()>&f) {
    if (e) {
    }else {
        f();
    }
}
}
void IOServiceFactory::dispatchServiceMessage(IOService*ios,const Duration&waitFor,const std::tr1::function<void()>&f){
    std::tr1::shared_ptr<boost::asio::deadline_timer> t(new boost::asio::deadline_timer(*ios,boost::posix_time::microseconds(waitFor.toMicroseconds())));
    using std::tr1::placeholders::_1;
    t->async_wait(std::tr1::bind(&handle_deadline_timer,_1,t,f));
}

class BoostAsioDeadlineTimer : public boost::asio::deadline_timer {
public:
    BoostAsioDeadlineTimer(boost::asio::io_service &io)
        : boost::asio::deadline_timer(io) {
    }
};

class TimerHandle::TimedOut {
public:
    static void timedOut(
            const boost::system::error_code &error,
            std::tr1::weak_ptr<TimerHandle> wthis)
    {
        std::tr1::shared_ptr<TimerHandle> sharedThis (wthis.lock());
        if (!sharedThis) {
            return; // we've been deleted already.
        }
        if (error == boost::asio::error::operation_aborted) {
            return; // don't care if the timer was cancelled.
        }
        sharedThis->mFunc();
    }
};

TimerHandle::TimerHandle(IOService *io) {
    mTimer = new BoostAsioDeadlineTimer(*io);
}

TimerHandle::TimerHandle(IOService *io, const std::tr1::function<void()>&f) {
    mTimer = new BoostAsioDeadlineTimer(*io);
    setCallback(f);
}

void TimerHandle::wait(
        const std::tr1::shared_ptr<TimerHandle> &thisPtr,
        const Duration &num_seconds) {
    mTimer->expires_from_now(boost::posix_time::microseconds(num_seconds.toMicroseconds()));
    mTimer->async_wait(
        boost::bind(
            &TimerHandle::TimedOut::timedOut,
            boost::asio::placeholders::error,
            thisPtr));
}

TimerHandle::~TimerHandle() {
    cancel();
}

void TimerHandle::cancel() {
    mTimer->cancel();
}

IOService::IOService():boost::asio::io_service(1){}
IOService::~IOService(){}
} }
