/*  Sirikata Network Utilities
 *  IOTimer.hpp
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
#ifndef _SIRIKATA_IOTIMER_HPP_
#define _SIRIKATA_IOTIMER_HPP_

#include "util/Platform.hpp"
#include "IODefs.hpp"

namespace Sirikata {
namespace Network {

typedef std::tr1::shared_ptr<IOTimer> IOTimerPtr;
typedef std::tr1::weak_ptr<IOTimer> IOTimerWPtr;

/** A timer which handles events using an IOService.  The user specifies
 *  a timeout and a callback and that callback is triggered after at least
 *  the specified duration has passed.  Repeated, periodic callbacks are
 *  supported by specifying a callback up front and
 */
class SIRIKATA_EXPORT IOTimer : public std::tr1::enable_shared_from_this<IOTimer> {
    DeadlineTimer *mTimer;
    IOCallback mFunc;
    class TimedOut;
public:
    /** Create a new timer, serviced by the specified IOService.
     *  \param io the IOService to service this timers events
     */
    IOTimer(IOService *io);
    IOTimer(IOService &io);

    /** Create a new timer, serviced by the specified IOService.
     *  \param io the IOService to service this timers events
     *  \param cb the handler for this timer's events.
     */
    IOTimer(IOService *io, const IOCallback& cb);
    IOTimer(IOService &io, const IOCallback& cb);

    ~IOTimer();

    /** Get the IOService associated with this timer. */
    IOService service() const;

    /** Set the callback which will be used by this timer.  Note that this sets
     *  the callback regardless of the current state of the timer, and will be
     *  used for timeouts currently in progress.
     */
    void setCallback(const IOCallback& cb);

    /** Wait for the specified duration, then call the previously set callback.
     *  \param waitFor the amount of time to wait.
     */
    void wait(const Duration &waitFor);

    /** Wait for the specified duration, then call the previously set callback.
     *  \param waitFor the amount of time to wait.
     *  \param cb the callback to set for this, and future, timeout events.
     */
    void wait(const Duration &waitFor, const IOCallback& cb);

    /** Cancel the current timer.  This will cancel the callback that would
     *  have resulted when the timer expired.
     */
    void cancel();
};

} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_IOTIMER_HPP_
