/** Iridium Kernel -- Task scheduling system
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

#ifndef IRIDIUM_Time_HPP__
#define IRIDIUM_Time_HPP__

namespace Iridium {
namespace Task {


class DeltaTime {
	float mDeltaTime;

public:
	DeltaTime(double t) {
		this->mDeltaTime = t;
	}
	
	inline DeltaTime operator- (const DeltaTime &other) const {
		return DeltaTime(mDeltaTime - other.mDeltaTime);
	}
	inline DeltaTime operator+ (const DeltaTime &other) const {
		return DeltaTime(mDeltaTime + other.mDeltaTime);
	}
	inline DeltaTime operator- () const {
		return DeltaTime(-mDeltaTime);
	}

	inline operator double() const {
		return (double)mDeltaTime;
	}
	inline operator float() const {
		return mDeltaTime;
	}
	int toMilli() const {
		return (int)(mDeltaTime*1000);
	}
	int64_t toMicro() const {
		return (int64_t)(mDeltaTime*1000000);
	}
};


class AbsTime {
	
	double mTime;
	explicit AbsTime(double t) {
		this->mTime = t;
	}

/*
	static AbsTime sLastFrameTime; // updated in "updateFrameTime"
	static void updateFrameTime();
*/
public:
	inline DeltaTime operator- (const AbsTime &other) const {
		return DeltaTime.fromDifference(mTime - other.mTime);
	}
	inline AbsTime operator+ (const DeltaTime &otherDelta) const {
		return AbsTime(mTime + (double)otherDelta);
	}
	static AbsTime now(); // Only way to generate an AbsTime for now...

/*
	static inline AbsTime frameTime() {
		return sLastFrameTime;
	}
*/
};

}
}

#endif
