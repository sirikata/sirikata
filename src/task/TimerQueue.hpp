/** Iridium Kernel -- Task scheduling system
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
 *  * Neither the name of Iridium nor the names of its contributors may
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

#ifndef IRIDIUM_TimerQueue_HPP__
#define IRIDIUM_TimerQueue_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <string>
#include <algorithm>

#include "Time.hpp"


namespace Iridium {
namespace Task {

typedef boost::function0<DeltaTime> TimedEvent;



/** A work queue that runs on each frame? */
class TimerQueue {
	std::map<AbsTime, std::list<TimedEvent> > mQueue;
public:

	/**
	 * Schedules this event to occur at nextTime.  The only way to remove
	 * ev from the timer queue is by returning a negative delta time from ev.
	 *
	 * Note that as long as ev returns a positive return value, the event will
	 * be rescheduled for that time.
	 * 
	 * @param nextTime  The absolute time to sechedule ev.
	 * @param ev        A (usually bound) boost::function to be called at nextTime.
	 */
	void schedule(AbsTime nextTime,
				const TimedEvent &ev);

	inline void schedule(DeltaTime delta,
				const TimedEvent &ev) {
		schedule(AbsTime.now() + delta, ev);
	}
	
	/**
	 * Schedules this event to occur at nextTime.  In addition to the handler's
	 * return value, the event may be removed by calling unschedule(id).
	 *
	 * Note that as long as ev returns a positive return value, the event will
	 * be rescheduled for that time.
	 * 
	 * @param nextTime  The absolute time to sechedule ev.
	 * @param ev        A (usually bound) boost::function to be called at nextTime.
	 * @param id        An id that can be passed to unschedule to prematurely cancel
	 *                  the timed event from being thrown.
	 */
	void schedule(AbsTime nextTime,
				const TimedEvent &ev,
				const SubscriptionId &id);

	inline void schedule(DeltaTime delta,
				const TimedEvent &ev,
				const SubscriptionId &id) {
		schedule(AbsTime.now() + delta, ev, id);
	}

	/**
	 * Unsubscribes from the event matching removeId. The removeId should be
	 * created using the same CLASS_ID or GEN_ID macros that were used when
	 * creating the subscription.
	 *
	 * @param removeId  the exact SubscriptionID to search for.
	 */
	void unschedule(const SubscriptionId &removeID);
};

extern TimerQueue timer_queue;

}
}


