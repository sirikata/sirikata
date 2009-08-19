/*  Sirikata Kernel -- Task scheduling system
 *  TimerQueue.hpp
 *
 *  Copyright (c) 2008, Patrick Reiter Horn
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

#ifndef SIRIKATA_TimerQueue_HPP__
#define SIRIKATA_TimerQueue_HPP__

#include "Time.hpp"
#include "UniqueId.hpp"


namespace Sirikata {

/** TimerQueue.hpp -- includes definitions for TimedEvent and TimerQueue */
namespace Task {

/**
 * A thunk to be called upon reaching the specified absolute time.
 *
 * @returns the number of seconds until the function shall be called
 * again, 0 if the function should be called next frame (ASAP), or
 * negative if it should be removed from the timer queue.
 */
typedef std::tr1::function<DeltaTime()> TimedEvent;



/** A work queue that runs on each frame. */
class TimerQueue {
	std::map<LocalTime, std::list<TimedEvent> > mQueue;
public:

	/**
	 * Schedules this event to occur at nextTime.  The only way to remove
	 * ev from the timer queue is by returning a negative delta time from ev.
	 *
	 * Note that as long as ev returns a positive return value, the event will
	 * be rescheduled for that time.
	 *
	 * @param nextTime  The absolute time to sechedule ev.
	 * @param ev        A (usually bound) std::tr1::function to be called at nextTime.
	 */
	void schedule(LocalTime nextTime,
				const TimedEvent &ev);

	/**
	 * Schedules this event to occur at nextTime.  In addition to the handler's
	 * return value, the event may be removed by calling unschedule(id).
	 *
	 * Note that as long as ev returns a positive return value, the event will
	 * be rescheduled for that time.
	 *
	 * @param nextTime  The absolute time to sechedule ev.
	 * @param ev        A (usually bound) std::tr1::function to be called at nextTime.
	 * @returns         An id that can be passed to unschedule to prematurely cancel
	 *                  the timed event from being thrown.
	 */
	SubscriptionId scheduleId(LocalTime nextTime,
				const TimedEvent &ev);

	/**
	 * Unsubscribes from the event matching removeId. The removeId should be
	 * the value returned when creating the subscription.
	 *
	 * @param removeId  the exact SubscriptionID to search for.
	 */
	void unschedule(const SubscriptionId &removeId);
};

/// Global TimerQueue singleton.
extern TimerQueue timer_queue;

}
}

#endif
