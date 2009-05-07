/*  Sirikata Kernel -- Task scheduling system
 *  EventManager.hpp
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

#ifndef SIRIKATA_EventManager_HPP__
#define SIRIKATA_EventManager_HPP__

#include "util/LockFreeQueue.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "util/AtomicTypes.hpp"
#include "options/Options.hpp"
#include "HashMap.hpp"
#include "UniqueId.hpp"
#include "Event.hpp"
#include "Time.hpp"

/** @namespace Sirikata::Task
 * Sirikata::Task contains the task-oriented functions for communication
 * across the program, as well as a scheduler to manage space CPU cycles
 * between graphics frames.
 */

namespace Sirikata {

/**
 * EventManager.hpp -- EventManager class definition, as well as the
 * EventResponse and EventOrder enumerations.
 */
namespace Task {

// TODO: Add events with timeouts.

// TODO: If two people register two events with the same removeId,
// it may be better to store two copies of the event and have unsubscribe
// only unsubscribe one of those two (use a multi_map for mRemoveById.

class WorkQueue;

/**
 * Defines the set of return values for an EventListener. An acceptable
 * value includes the bitwise or of any values in the enum.
 */
class SIRIKATA_EXPORT EventResponse {
	enum {
		NOP,
		DELETE_LISTENER=1,
		CANCEL_EVENT=2,
		DELETE_LISTENER_AND_CANCEL_EVENT = DELETE_LISTENER | CANCEL_EVENT,
	} mResp;

	template <class Ev> friend class EventManager;

public:
	/// the event listener will be called again, and event stays on the queue.
	static EventResponse nop() {
		EventResponse retval;
		retval.mResp=NOP;
		return retval;
	}
	/// Never call this function again (One-shot listener).
	static EventResponse del() {
		EventResponse retval;
		retval.mResp=DELETE_LISTENER;
		return retval;
	}
	/// Take the event off the queue after this stage (EARLY,MIDDLE,LATE)
	static EventResponse cancel() {
		EventResponse retval;
		retval.mResp=CANCEL_EVENT;
		return retval;
	}
	/// Kill the listener and kill the event (probably not useful).
	static EventResponse cancelAndDel() {
		EventResponse retval;
		retval.mResp=DELETE_LISTENER_AND_CANCEL_EVENT;
		return retval;
	}
};

/**
 * Defines constants to allow a strict ordering of event processing.
 * At the moment, since there is not a good use case for this,
 * there are only three legal orderings specified.
 */
enum EventOrder {
	EARLY,
	MIDDLE,
	LATE,
	NUM_EVENTORDER
};
/// Exception thrown if an invalid EventOrder is passed.
class SIRIKATA_EXPORT EventOrderException : std::exception {};

/** Some EventManagers may require a different base class which
 * inherits from Event but have additional properties. */
template <class EventBase=Event>
class SIRIKATA_EXPORT EventManager {

	/* TYPEDEFS */
public:
	/// A shared_ptr to an Event.
	typedef std::tr1::shared_ptr<EventBase> EventPtr;

	/**
	 * A std::tr1::function1 taking an Event and returning a value indicating
	 * Whether to cancel the event, remove the event responder, or some
	 * other values.
	 *
	 * @see EventResponse
	 */
	typedef std::tr1::function<EventResponse(const EventPtr&)> EventListener;

private:

	/// if the listener does not corresond to an id, use SubscriptionId::null().
	typedef std::pair<EventListener, SubscriptionId> ListenerSubscriptionInfo;
	typedef std::list<ListenerSubscriptionInfo> ListenerList;

	/** Since std::map is free to reallocate its elements at its own choosing
	 this class must be a pointer, not a statically-allocated array. (we want
	 to be able to carry ListenerList iterators around) */
	class PartiallyOrderedListenerList {
		ListenerList ll[NUM_EVENTORDER];
	public:
		ListenerList &get (size_t i) {
			return ll[i];
		}
		//ListenerList &operator [] (size_t i) {
		//	return ll[i];
		//}
	};

	typedef std::tr1::unordered_map<IdPair::Secondary,
				PartiallyOrderedListenerList*,
				IdPair::Secondary::Hasher> SecondaryListenerMap;
	typedef std::pair<PartiallyOrderedListenerList, SecondaryListenerMap> PrimaryListenerInfo;
	typedef std::map<IdPair::Primary, PrimaryListenerInfo*> PrimaryListenerMap;

	struct SIRIKATA_EXPORT EventSubscriptionInfo {
		ListenerList *mList;
		typename ListenerList::iterator mIter;

		// used for garbage collection after unsubscribing.
		SecondaryListenerMap *secondaryMap;
		IdPair::Secondary secondaryId;

		EventSubscriptionInfo(ListenerList *list,
					typename ListenerList::iterator &iter)
			: mList(list), mIter(iter),
			  secondaryMap(NULL), secondaryId(IdPair::Secondary::null()) {
		}

		EventSubscriptionInfo(ListenerList *list,
					typename ListenerList::iterator &iter,
					SecondaryListenerMap *slm,
					const IdPair::Secondary &slmKey)
			: mList(list), mIter(iter),
			 secondaryMap(slm), secondaryId(slmKey) {
		}
	};
	typedef std::tr1::unordered_map<SubscriptionId, EventSubscriptionInfo, SubscriptionIdHasher> RemoveMap;

	struct ListenerSubRequest;
	struct ListenerUnsubRequest;
	struct FireEvent;
	friend struct ListenerSubRequest;
	friend struct ListenerUnsubRequest;
	friend struct FireEvent;

	/* MEMBERS */

	WorkQueue *mWorkQueue;
	PrimaryListenerMap mListeners;

	RemoveMap mRemoveById; ///< Used for unsubscribe: always keep in sync.

	/* PRIVATE FUNCTIONS */

	PrimaryListenerInfo *insertPriId(const IdPair::Primary &pri);

	typename SecondaryListenerMap::iterator insertSecId(
				SecondaryListenerMap &map,
				const IdPair::Secondary &sec);


	/** Cleans up the removal ID like unsubscribe, but does not
	 * free the actual event or put it in mRemovedListeners. */
	void clearRemoveId(SubscriptionId removeId);

	/** Removee if the passed element has no items left.
	 * Call after anything that could remove items from the map. */
	bool cleanUp(SecondaryListenerMap *slm,
				typename SecondaryListenerMap::iterator &slm_iter);

	typename ListenerList::iterator
		addListener(ListenerList *insertList,
				const EventListener &listener,
				SubscriptionId removeId);

	bool callAllListeners(EventPtr ev,
				ListenerList *lili);
public:

	EventManager(WorkQueue *queue);

	~EventManager();

	inline WorkQueue *getWorkQueue() {
		return mWorkQueue;
    }

	/**
	 * Subscribes to a specific event. The listener function will receieve
	 * only events whose type matches eventId.mPriId, and whose secondary
	 * information matches eventId.mSecId.
	 *
	 * Using this function, the listener may be unsubscribed only if it
	 * returns DELETE_LISTENER after being called for an event.
	 *
	 * @param eventId    the specific event to subscribe to
	 * @param listener   a (usually bound) std::tr1::function taking an EventPtr
	 * @param when       Guarantees a specific ordering. Defaults to MIDDLE.
	 *
	 * @see EventResponse
	 * @see EventListener
	 */
	void subscribe(const IdPair &eventId,
				const EventListener &listener,
				EventOrder when=MIDDLE);
	/**
	 * Subscribes to a given event type. The listener function will receieve ALL
	 * Events for the primaryId, no matter what SecondaryId it has.
	 *
	 * Using this function, the listener may be unsubscribed only if it
	 * returns DELETE_LISTENER after being called for an event.
	 *
	 * @param primaryId  the event type to subscribe to
	 * @param listener   a (usually bound) std::tr1::function taking an EventPtr
	 * @param when       Guarantees a specific ordering. Defaults to MIDDLE.
	 *
	 * @see EventResponse
	 * @see EventListener
	 */
	void subscribe(const IdPair::Primary & primaryId,
				const EventListener & listener,
				EventOrder when=MIDDLE);

	/**
	 * Subscribes to a specific event. The listener function will receieve
	 * only events whose type matches eventId.mPriId, and whose secondary
	 * information matches eventId.mSecId.
	 *
	 * The removeId specifies an identifier, generated by the GEN_ID or CLASS_ID
	 * macros, which may later be passed into unsubscribe.  The handler function
	 * may also unsubscribe the event by returning DELETE_LISTENER. Note also that
	 * if two subscriptions are created with the same removeId, the original is
	 * unsubscribed and superceded by this listener.
	 *
	 * @param eventId    the specific event to subscribe to
	 * @param listener   a (usually bound) std::tr1::function taking an EventPtr
	 * @param when       Guarantees a specific ordering. Defaults to MIDDLE.
	 * @returns          a SubscriptionId that can be passed to unsubscribe
	 *
	 * @see SubscriptionId
	 * @see EventResponse
	 * @see EventListener
	 */
	SubscriptionId subscribeId(const IdPair & eventId,
				const EventListener & listener,
				EventOrder when=MIDDLE);
	/**
	 * Subscribes to a given event type. The listener function will receieve ALL
	 * Events for the primaryId, no matter what SecondaryId it has.
	 *
	 * The removeId specifies an identifier, generated by the GEN_ID or CLASS_ID
	 * macros, which may later be passed into unsubscribe.  The handler function
	 * may also unsubscribe the event by returning DELETE_LISTENER. Note also that
	 * if two subscriptions are created with the same removeId, the original is
	 * unsubscribed and superceded by this listener.
	 *
	 * @param primaryId  the event type to subscribe to
	 * @param listener   a (usually bound) std::tr1::function taking an EventPtr
	 * @param when       Guarantees a specific ordering. Defaults to MIDDLE.
	 * @returns          a SubscriptionId that can be passed to unsubscribe
	 *
	 * @see SubscriptionId
	 * @see EventResponse
	 * @see EventListener
	 */
	SubscriptionId subscribeId(const IdPair::Primary & primaryId,
				const EventListener & listener,
				EventOrder when=MIDDLE);

	/**
	 * Unsubscribes from the event matching removeId. The removeId should be
	 * created using the same CLASS_ID or GEN_ID macros that were used when
	 * creating the subscription.
	 * Note that double-deletes *are* allowed because attempting to avoid
	 * them is too risky (timeouts, return value, explicit unsubscribe).
	 * Hence, there is no return value to 'unsubscribe'.
	 *
	 * @param removeId        the exact SubscriptionID to search for.
	 * @param notifyListener  whether to call the EventListener with NULL.
	 */
	void unsubscribe(SubscriptionId removeId, bool notifyListener=false);

	/**
	 * Puts the passed event into the mUnprocessed event queue, which will be
	 * fired at the end of the frame corresponding to its IdPair. See the Event
	 * class for more information.
	 *
	 * @param ev  A shared_ptr to an Event to be stored in the queue.
	 * @see   Event
	 */
	void fire(EventPtr ev);

};

/**
 * Generic Event Manager -- the most common type that accepts any
 * subclass of Event.
 */
typedef class EventManager<Event> GenEventManager;

}
}

#endif
