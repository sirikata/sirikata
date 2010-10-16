/*  Sirikata
 *  Time.hpp
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

#ifndef _SIRIKATA_TIME_HPP_
#define _SIRIKATA_TIME_HPP_
#include "../task/Time.hpp"
namespace Sirikata {
class Time : private Task::LocalTime{
    Time (uint64 raw) :LocalTime(raw){}
    Time (const LocalTime &local):LocalTime(local) {

    }
  public:
    Time ()
     : LocalTime()
    {}

    uint64 raw() const {
        return static_cast<const LocalTime*>(this)->raw();
    }
	/// Equality comparison (same as (*this - other) == 0)
    inline bool operator== (const Time &other) const {
		return *static_cast<const LocalTime*>(this)==other;
	}
    inline bool operator!= (const Time& other) const {
		return *static_cast<const LocalTime*>(this)!=other;
    }

	/// Ordering comparison (same as (*this - other) < 0)
	inline bool operator< (const Time &other) const {
		return *static_cast<const LocalTime*>(this)<other;
	}
	inline bool operator<= (const Time &other) const {
		return *static_cast<const LocalTime*>(this)<=other;
	}
	inline bool operator> (const Time &other) const {
		return *static_cast<const LocalTime*>(this)>other;
	}
	inline bool operator>= (const Time &other) const {
		return *static_cast<const LocalTime*>(this)>=other;
	}

	/**
	 * Takes the difference of two absolute times.
	 *
	 * @returns a DeltaTime that can be cast to a double in seconds.
	 */
	inline Duration operator- (const Time &other) const {
		return *static_cast<const LocalTime*>(this)-other;
	}
	/**
	 * Adds a time difference to a given absolute time.
	 *
	 * @returns a new absolute time--does not modify the existing one.
	 */
	inline Time operator+ (const Duration &otherDelta) const {
        return *static_cast<const LocalTime*>(this)+otherDelta;
	}
	inline Time operator- (const Duration &otherDelta) const {
        return *static_cast<const LocalTime*>(this)-otherDelta;
	}
	inline void operator+= (const Duration &otherDelta) {
        return *static_cast<LocalTime*>(this)+=otherDelta;
	}
	inline void operator-= (const Duration &otherDelta) {
        return *static_cast<LocalTime*>(this)-=otherDelta;
	}

    static Time microseconds(int64 abstime){
        return Time(abstime);
    }

	/**
	 * The only public construction function for absolute times.
	 *
	 * @returns the current system time (gettimeofday), not to be used
	 * for time synchronization over the network.
	 */
	static Time now(const Duration&spaceDurationOffset){
        return Time(LocalTime::now()+spaceDurationOffset);
    }

    /** Return the current local time, i.e. Time::now() with no offset.
     *  \returns the current local time
     */
    static Time local() {
        return now(Duration::zero());
    }

	/**
	 * Creates the time when items are 0
	 *
	 * @returns the beginning of time--so that a subtraction can be used with an actual time
	 */
	static Time epoch() { return Time(0); }

	/**
	 * Creates a 'null' absolute time that is equivalent to
	 * a long time ago in a galaxy far away.  Always less than
	 * a real time, and equal to another null() value.
	 *
	 * @returns a 'null' time to be used if a timeout is not applicable.
	 */
	static Time null() { return Time(0); }
    /**
     * Converts from a local computer time to a Time based on a server offset computed for a given space server
     */
    static Time convertFrom(const LocalTime&lt, const Duration&serverOffset){
        return Time(lt)+serverOffset;
    }
};
}
#endif
