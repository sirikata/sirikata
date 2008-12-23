/** Iridium Kernel -- Task scheduling system
 *  Event.hpp
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

#ifndef IRIDIUM_Event_HPP__
#define IRIDIUM_Event_HPP__

#include "EventManager.hpp"

namespace Iridium {
namespace Task {

typedef class EventManager<Event> GenEventManager;


int HASH(const std::string &s);

class IdPair {
public:
	class Secondary {
	public:
		typedef intptr_t IntType;
	private:
		intptr_t mIntValue;
		std::string mStrValue;
	public:
		Secondary(intptr_t i) : mIntValue(i) {}
		Secondary(const std::string &str) : mStrValue(str), mIntValue(HASH(str)) {}

		inline bool operator== (const Secondary &otherId) const {
			return (mIntValue == otherId.mIntValue && 
					mStrValue == otherId.mStrValue);
		}
		inline bool operator< (const Secondary &otherId) const {
			if (mIntValue == otherId.mIntValue) {
				return (mStrValue < otherId.mStrValue);
			} else {
				return (mIntValue < otherId.mIntValue);
			}
		}

		static inline Secondary null() {
			return Secondary(0);
		}
	};

	class Primary {
	private:
		int mId;
	public:
		PrimaryId(const std::string &eventName);

		inline bool operator< (const Primary &other) const {
			return mId < other.mId;
		}
		inline bool operator== (const Primary &other) const {
			return mId == other.mId;
		}
	};

	Primary mPriId;
	Secondary mSecId;
	
	IdPair(const Primary &pri, const Secondary &sec)
		: mPriId(pri), mSecId(sec) {
	}
	IdPair(const std::string &pri, const std::string &sec)
		: mPriId(pri), mSecId(sec) {
	}
	IdPair(const std::string &pri, Secondary::IntType sec)
		: mPriId(pri), mSecId(sec) {
	}
};


/** Base class for any events that are to be thrown */
class Event {
protected:
	/**
	 * a IdPair used to determine which EventListeners are interested:
	 * The PrimaryId must not be an empty string, and should be one of a
	 * predefined set of strings for efficiency reasons.
	 * 
	 * Most events should have a SecondaryId (string, pointer, or number)
	 * which identifies it, however in a few cases the secondary ID does not make
	 * sense, in which case it should be <code>SecondaryId::null()</code>
	 */
	IdPair mId;
public:
	Event(const IdPair &id) 
		: mId(id) {
	}

	/** Most subclasses will use aditional members, free them properly. */
	virtual ~Event();

	inline bool operator == (const IdPair &otherId) const {
		return (mId == otherId);
	}
};

}
}

#endif
