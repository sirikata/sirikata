#ifndef EVENT_MANAGER_HPP__
#define EVENT_MANAGER_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <string>
#include <algorithm>

namespace Iridium {
namespace Task {

class Event;
class EventResponse {
	enum {
		NOP,
		DELETE_LISTENER=1,
		CANCEL_EVENT=2,
	} mResp;
};

typedef boost::shared_ptr<Event> EventPtr;
typedef boost::function1<EventResponse, EventPtr> EventListener;

int HASH(const std::string &s);

class SecondaryId {
private:
	intptr_t mIntValue;
	std::string mStrValue;
public:
	SecondaryId(intptr_t i) : mIntValue(i) {}
	SecondaryId(const std::string &str) : mStrValue(str), mIntValue(HASH(str)) {}

	inline bool operator== (const SecondaryId &otherId) const {
		return (mIntValue == otherId.mIntValue && 
				mStrValue == otherId.mStrValue);
	}
	inline bool operator< (const SecondaryId &otherId) const {
		if (mIntValue == otherId.mIntValue) {
			return (mStrValue < otherId.mStrValue);
		} else {
			return (mIntValue < otherId.mIntValue);
		}
	}
};

class PrimaryId {
private:
	int mId;
public:
	PrimaryId(const std::string &eventName);

	inline bool operator< (const PrimaryId &other) const {
		return mId < other.mId;
	}
	inline bool operator== (const PrimaryId &other) const {
		return mId == other.mId;
	}
};

class ListenerId {
public:
	void *thisPtr; // do not dereference
	std::string uniqueId; // usuallycontains class or function name, and other arguments.
};

enum EventOrder {
	EARLY,
	MIDDLE,
	LATE,
	NUM_EVENTORDER
};

class IdPair {
	PrimaryId mPriId;
	SecondaryId mSecId;
};

class EventManager {
	
public:
	void addHandler(IdPair id, EventListener listener, EventOrder when=MIDDLE);
	void addHandler(PrimaryId priId, EventListener listener, EventOrder when=MIDDLE);

	void removeByListener(EventListener);
	void removeAllByInterest(IdPair removeAll);
	void removeAllByInterest(PrimaryId removeAll);

};

}
}

#endif
