/*  Sirikata Kernel -- Task scheduling system
 *  Time.hpp
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

#ifndef SIRIKATA_Time_HPP__
#define SIRIKATA_Time_HPP__

namespace Sirikata {

/**
 * Time.hpp -- Task-oriented time functions: DeltaTime and LocalTime
 * for representing relative and absolute work queue times.
 */
namespace Task {


/**
 * Represents the difference of two time values. Can be created either by
 * calling the constructor with a double delta time, or by subtracting two
 * absolute times.
 *
 * @par
 * To convert x to an absolute time, use <code>LocalTime::now() + x</code>
 *
 * @see LocalTime
 */
class SIRIKATA_EXPORT DeltaTime {
    int64 mDeltaTime;

    explicit DeltaTime(int64 us)
        : mDeltaTime(us)
    {
    }
public:
    DeltaTime()
        : mDeltaTime(0)
    {
    }

    inline double operator/ (const DeltaTime&other)const {
        return (double)(mDeltaTime/other.mDeltaTime)+(double)(mDeltaTime%other.mDeltaTime)/(double)other.mDeltaTime;
    }

    inline DeltaTime operator/(float32 rhs) const {
        return DeltaTime((int64)((float64)mDeltaTime / (float64)rhs));
    }
    inline void operator/=(float32 rhs) {
        mDeltaTime = (int64)((float64)mDeltaTime / (float64)rhs);
    }
    inline DeltaTime operator/(float64 rhs) const {
        return DeltaTime((int64)((float64)mDeltaTime / (float64)rhs));
    }
    inline void operator/=(float64 rhs) {
        mDeltaTime = (int64)((float64)mDeltaTime / (float64)rhs);
    }

    inline DeltaTime operator*(float32 rhs) const {
        return DeltaTime((int64)((float64)mDeltaTime * (float64)rhs));
    }
    inline void operator*=(float32 rhs) {
        mDeltaTime = (int64)((float64)mDeltaTime * (float64)rhs);
    }
    inline DeltaTime operator*(float64 rhs) const {
        return DeltaTime((int64)((float64)mDeltaTime * (float64)rhs));
    }
    inline void operator*=(float64 rhs) {
        mDeltaTime = (int64)((float64)mDeltaTime * (float64)rhs);
    }

    static DeltaTime seconds(double s) {
        return DeltaTime((int64)(s*1000000.));
    }
    static DeltaTime milliseconds(double ms) {
        return DeltaTime((int64)(ms*1000.));
    }
    static DeltaTime milliseconds(int64 ms) {
        return DeltaTime(ms*1000);
    }
    static DeltaTime microseconds(int64 us) {
        return DeltaTime(us);
    }
    static DeltaTime nanoseconds(int64 ns) {
        return DeltaTime(ns/1000);
    }

    /** Return a zero length duration. */
    static DeltaTime zero() {
        return DeltaTime::nanoseconds(0);
    }

	/// Simple helper function -- returns "LocalTime::now() + (*this)".
	class LocalTime fromNow() const;

	/// Arethmetic operator-- subtract two DeltaTime values.
	inline DeltaTime operator- (const DeltaTime &other) const {
		return DeltaTime(mDeltaTime - other.mDeltaTime);
	}
	/// Arethmetic operator-- add two DeltaTime values.
	inline DeltaTime operator+ (const DeltaTime &other) const {
		return DeltaTime(mDeltaTime + other.mDeltaTime);
	}
	/// Arethmetic operator-- add two DeltaTime values.
	inline DeltaTime operator+= (const DeltaTime &other) {
		return DeltaTime(mDeltaTime += other.mDeltaTime);
	}
	/// Arethmetic operator-- subtract two DeltaTime values.
	inline DeltaTime operator-= (const DeltaTime &other) {
		return DeltaTime(mDeltaTime -= other.mDeltaTime);
	}
	/// Arethmetic operator-- negate a DeltaTime.
	inline DeltaTime operator- () const {
		return DeltaTime(-mDeltaTime);
	}
    double toSeconds () const {
        return mDeltaTime/1000000.;
    }
	/// Convert to an integer in milliseconds.
	int64 toMilliseconds() const {
		return mDeltaTime/1000;
	}
	/// Convert to an integer in microseconds.
	int64 toMicroseconds() const {
		return mDeltaTime;
	}
	/// Convert to an integer in microseconds.
	int64 toMicro() const {
        return toMicroseconds();
	}

	/// Equality comparison
	inline bool operator== (const DeltaTime &other) const {
		return (mDeltaTime == other.mDeltaTime);
	}
        inline bool operator!= (const DeltaTime& other) const {
            return (mDeltaTime != other.mDeltaTime);
        }
	/// Ordering comparison
	inline bool operator< (const DeltaTime &other) const {
		return (mDeltaTime < other.mDeltaTime);
	}
	inline bool operator<= (const DeltaTime &other) const {
		return (mDeltaTime <= other.mDeltaTime);
	}
	inline bool operator> (const DeltaTime &other) const {
		return (mDeltaTime > other.mDeltaTime);
	}
	inline bool operator>= (const DeltaTime &other) const {
		return (mDeltaTime >= other.mDeltaTime);
	}
};

/**
 * Represents an absolute system time on this machine.  Note: LocalTime is stored internally
 * as an int64.  The only two ways to create an LocalTime object are by adding
 * to another LocalTime, and by calling LocalTime::now().
 *
 * @par
 * Note that as LocalTime is a local time for purposes of event processing
 * only, there are no conversion functions, except by taking the difference
 * of two LocalTime objects.
 *
 * @see DeltaTime
 */
class SIRIKATA_EXPORT LocalTime {

	uint64 mTime;
protected:
	/// Private constructor-- Use "now()" to create an LocalTime.
	explicit LocalTime(uint64 t) {
		this->mTime = t;
	}

public:
    uint64 raw() const {
        return mTime;
    }
    
	/// Equality comparison (same as (*this - other) == 0)
    inline bool operator== (const LocalTime &other) const {
		return (mTime == other.mTime);
	}
    inline bool operator!= (const LocalTime& other) const {
        return (mTime != other.mTime);
    }

	/// Ordering comparison (same as (*this - other) < 0)
	inline bool operator< (const LocalTime &other) const {
		return (mTime < other.mTime);
	}
	inline bool operator<= (const LocalTime &other) const {
		return (mTime <= other.mTime);
	}
	inline bool operator> (const LocalTime &other) const {
		return (mTime > other.mTime);
	}
	inline bool operator>= (const LocalTime &other) const {
		return (mTime >= other.mTime);
	}

	/**
	 * Takes the difference of two absolute times.
	 *
	 * @returns a DeltaTime that can be cast to a double in seconds.
	 */
	inline DeltaTime operator- (const LocalTime &other) const {
		return DeltaTime::microseconds((int64)mTime - (int64)other.mTime);
	}
	/**
	 * Adds a time difference to a given absolute time.
	 *
	 * @returns a new absolute time--does not modify the existing one.
	 */
	inline LocalTime operator+ (const DeltaTime &otherDelta) const {
		return LocalTime(mTime + otherDelta.toMicro());
	}
	inline LocalTime operator- (const DeltaTime &otherDelta) const {
		return (*this) + (-otherDelta);
	}
	inline void operator+= (const DeltaTime &otherDelta) {
		mTime += otherDelta.toMicro();
	}
	inline void operator-= (const DeltaTime &otherDelta) {
		(*this) += (-otherDelta);
	}

    static LocalTime microseconds(int64 abstime){
        return LocalTime(abstime);
    }

	/**
	 * The only public construction function for absolute times.
	 *
	 * @returns the current system time (gettimeofday), not to be used
	 * for time synchronization over the network.
	 */
	static LocalTime now(); // Only way to generate an LocalTime for now...

	/**
	 * Creates the time when items are 0
	 *
	 * @returns the beginning of time--so that a subtraction can be used with an actual time
	 */
	static LocalTime epoch() { return LocalTime(0); }

	/**
	 * Creates a 'null' absolute time that is equivalent to
	 * a long time ago in a galaxy far away.  Always less than
	 * a real time, and equal to another null() value.
	 *
	 * @returns a 'null' time to be used if a timeout is not applicable.
	 */
	static LocalTime null() { return LocalTime(0); }


};

SIRIKATA_EXPORT std::ostream& operator<<(std::ostream& os, const DeltaTime& rhs);
SIRIKATA_EXPORT std::istream& operator>>(std::istream& is, DeltaTime& rhs);
inline LocalTime DeltaTime::fromNow() const {
	return LocalTime::now() + (*this);
}

}
}

#endif
