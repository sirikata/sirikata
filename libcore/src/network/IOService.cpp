/*  Sirikata Network Utilities
 *  IOService.cpp
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
#include "IOService.hpp"
#include "util/Time.hpp"
#include <boost/asio.hpp>

namespace Sirikata {
namespace Network {

typedef boost::asio::io_service InternalIOService;

typedef boost::asio::deadline_timer deadline_timer;
typedef std::tr1::shared_ptr<deadline_timer> deadline_timer_ptr;
typedef boost::posix_time::microseconds posix_microseconds;
using std::tr1::placeholders::_1;

IOService::IOService() {
    mImpl = new boost::asio::io_service(1);
}

IOService::~IOService(){
    delete mImpl;
}

uint32 IOService::pollOne() {
    return (uint32) mImpl->poll_one();
}

uint32 IOService::poll() {
    return (uint32) mImpl->poll();
}

uint32 IOService::runOne() {
    return (uint32) mImpl->run_one();
}

uint32 IOService::run() {
    return (uint32) mImpl->run();
}

void IOService::stop() {
    mImpl->stop();
}

void IOService::reset() {
    mImpl->reset();
}

void IOService::dispatch(const CompletionHandler& handler) {
    mImpl->dispatch(handler);
}

void IOService::post(const CompletionHandler& handler) {
    mImpl->post(handler);
}

namespace {
void handle_deadline_timer(const boost::system::error_code&e, const deadline_timer_ptr& timer, const IOService::CompletionHandler& handler) {
    if (e)
        return;

    handler();
}
} // namespace

void IOService::post(const Duration& waitFor, const CompletionHandler& handler) {
    deadline_timer_ptr timer(new deadline_timer(*mImpl, posix_microseconds(waitFor.toMicroseconds())));
    timer->async_wait(std::tr1::bind(&handle_deadline_timer, _1, timer, handler));
}

} // namespace Network
} // namespace Sirikata
