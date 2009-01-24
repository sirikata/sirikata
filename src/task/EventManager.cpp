/*  Sirikata Kernel -- Task scheduling system
 *  EventManager.cpp
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

#include "EventManager.hpp"
#include "Time.hpp"
#include "UniqueId.hpp"
#include "Event.hpp"

#include "TimerQueue.hpp"

#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace Sirikata {

/**
 * EventManager.cpp -- Definition for EventManager functions;
 * Also includes explicit template instantiations for EventManager<Event>
 */



namespace Task {


template <class T>
EventManager<T>::EventManager() {
	eventlock = new boost::mutex;
	listenerlock = new boost::mutex;
}

template <class T>
EventManager<T>::~EventManager() {
	delete eventlock;
	delete listenerlock;
}



// ============= SUBSCRIPTION FUNCTIONS ==============

template <class T>
typename EventManager<T>::PrimaryListenerInfo *
	EventManager<T>::insertPriId(
			const IdPair::Primary &pri)
{
	typename PrimaryListenerMap::iterator iter = mListeners.find(pri);
	if (iter == mListeners.end()) {
		iter = mListeners.insert(
			typename PrimaryListenerMap::value_type(
				pri, new PrimaryListenerInfo)
			).first;
	}
	return (*iter).second;
}


template <class T>
typename EventManager<T>::SecondaryListenerMap::iterator
	EventManager<T>::insertSecId(
			SecondaryListenerMap &secondListeners,
			const IdPair::Secondary &sec)
{
	typename SecondaryListenerMap::iterator iter2 = secondListeners.find(sec);
	if (iter2 == secondListeners.end()) {
		iter2 = secondListeners.insert(
			typename SecondaryListenerMap::value_type(
				sec, new PartiallyOrderedListenerList)
			).first;
	}
	return iter2;
}

	/**
	 * Standard function to add a listener to a ListenerList.
	 *
	 * Note that we use push_front so as not to disrupt any
	 * events currently being processed (and possibly go in
	 * an infinite loop, due to a stupid listener adding another
	 * copy of itself back into the end).
	 */
template <class T>
void EventManager<T>::addListener(ListenerList *insertList,
			const EventListener &listener,
			SubscriptionId removeId)
{
	insertList->push_front(
		ListenerSubscriptionInfo(listener, removeId));
}

template <class T>
void EventManager<T>::subscribe(const IdPair &eventId,
			const EventListener &listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	boost::lock_guard<boost::mutex>(*listenerlock);

	SecondaryListenerMap &secondListeners =
		insertPriId(eventId.mPriId)->second;
	typename SecondaryListenerMap::iterator secondIter =
		insertSecId(secondListeners, eventId.mSecId);
	ListenerList &insertList = (*secondIter).second->get(whichOrder);
	addListener(&insertList, listener, SubscriptionIdClass::null());
}

template <class T>
void EventManager<T>::subscribe(const IdPair::Primary & primaryId,
			const EventListener & listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	boost::lock_guard<boost::mutex>(*listenerlock);

	ListenerList &insertList =
		insertPriId(primaryId)->first.get(whichOrder);
	addListener(&insertList, listener, SubscriptionIdClass::null());
}

template <class T>
SubscriptionId EventManager<T>::subscribeId(
			const IdPair & eventId,
			const EventListener & listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	SubscriptionId removeId = SubscriptionIdClass::alloc();
	boost::lock_guard<boost::mutex>(*listenerlock);

	SecondaryListenerMap &secondListeners =
		insertPriId(eventId.mPriId)->second;
	typename SecondaryListenerMap::iterator secondIter =
		insertSecId(secondListeners, eventId.mSecId);
	ListenerList &insertList = (*secondIter).second->get(whichOrder);
	addListener(&insertList, listener, removeId);

	typename ListenerList::iterator iter = insertList.end();
	mRemoveById.insert(
		typename RemoveMap::value_type(removeId,
			EventSubscriptionInfo(
				&insertList,
				--iter,
				&secondListeners,
				secondIter)));

	return removeId;
}
template <class T>
SubscriptionId EventManager<T>::subscribeId(
			const IdPair::Primary & priId,
			const EventListener & listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	SubscriptionId removeId = SubscriptionIdClass::alloc();
	boost::lock_guard<boost::mutex>(*listenerlock);

	ListenerList &insertList =
		insertPriId(priId)->first.get(whichOrder);
	addListener(&insertList, listener, removeId);

	typename ListenerList::iterator iter = insertList.end();
	mRemoveById.insert(
		typename RemoveMap::value_type(removeId,
			EventSubscriptionInfo(
				&insertList,
				--iter)));

	return removeId;
}

// ============= UNSUBSCRIPTION FUNCTIONS ==============

template <class T>
void EventManager<T>::clearRemoveId(
			SubscriptionId removeId)
{
	boost::lock_guard<boost::mutex>(*listenerlock);
	typename RemoveMap::iterator iter = mRemoveById.find(removeId);
	if (iter == mRemoveById.end()) {
		std::cerr << "failed to clear removeId " << removeId <<
			" -- usually this is from a double-unsubscribe. ";
		int double_unsubscribe = 0;
		assert(double_unsubscribe);
	} else {
		mRemoveById.erase(iter);
		SubscriptionIdClass::free(removeId);
	}
}

template <class T>
void EventManager<T>::unsubscribe(
			SubscriptionId removeId,
			bool notifyListener)
{
	boost::lock_guard<boost::mutex>(*listenerlock);
	typename RemoveMap::iterator iter = mRemoveById.find(removeId);
	if (iter == mRemoveById.end()) {
		std::cerr << "!!! Unsubscribe for removeId " << removeId <<
			" -- usually this is from a double-unsubscribe. ";
	} else {
		EventSubscriptionInfo &subInfo = (*iter).second;
		if (subInfo.mList == mProcessingList) {
			// The reason there is no return value to unsubscribe.
			std::cout << "**** Pending unsubscribe " << removeId;
			if (mProcessingUnsubscribe.insert(removeId).second == true) {
				if (notifyListener) {
					mRemovedListeners.push_back((*subInfo.mIter).first);
				}
			}
		} else {
			std::cout << "**** Unsubscribe " << removeId;
			if (notifyListener) {
				mRemovedListeners.push_back((*subInfo.mIter).first);
			}
			subInfo.mList->erase(subInfo.mIter);
			if (subInfo.secondaryMap) {
				std::cout << " with Secondary ID " <<
					(*subInfo.secondaryIter).first << std::endl << "\t";
				cleanUp(subInfo.secondaryMap, subInfo.secondaryIter);
			}

			mRemoveById.erase(iter);
			SubscriptionIdClass::free(removeId);
		}
		std::cout << std::endl;
	}
}

template <class T>
bool EventManager<T>::cleanUp(
	SecondaryListenerMap *slm,
	typename SecondaryListenerMap::iterator &slm_iter)
{
	bool isEmpty = true;
	for (int i = 0; i < NUM_EVENTORDER; i++) {
		if (&((*slm_iter).second->get(i)) == mProcessingList) {
			return false; // don't clean up the list we're processing.
		}
		isEmpty = isEmpty && (*slm_iter).second->get(i).empty();
	}
	if (isEmpty) {
		std::cout << "[Cleaning up Secondary ID " << (*slm_iter).first << "]" << std::endl;
		delete (*slm_iter).second;
		slm->erase(slm_iter);
	}
	return isEmpty;
}


// =============== EVENT QUEUE FUNCTIONS ===============

template <class T>
void EventManager<T>::fire(EventPtr ev) {
	boost::lock_guard<boost::mutex>(*eventlock);
	std::cout << "**** Firing event " << (void*)(&(*ev)) <<
		" with " << ev->getId() << std::endl;
	mUnprocessed.push_back(ev);
};


/* FIXME: We need a "never" constant for AbsTime that is
   always grreater than anything else */

template <class T>
bool EventManager<T>::callAllListeners(EventPtr ev,
			ListenerList *lili,
			AbsTime forceCompletionBy) {

	bool cancel = false;
	typename ListenerList::iterator iter = lili->begin();
	/* Disallow 'unsubscribe()' while in this while loop.
	 * We make a temporary mUnsubscribe list, and unsubscribe all
	 * of them after we are done here.
	 *
	 * The reason for this is 'iter' or 'next' may end up getting
	 * deleted in the process.
	 */
	while (iter!=lili->end()) {
		typename ListenerList::iterator next = iter;
		++next;
		// Now call the event listener.
		std::cout << " >>>\tCalling " << (*iter).second <<
			"..." << std::endl;
		EventResponse resp = (*iter).first(ev);
		std::cout << " >>>\t\tReturned ";
		if (((int)resp.mResp) & EventResponse::DELETE_LISTENER) {
			std::cout << "DELETE_LISTENER ";
			lili->erase(iter);
			if ((*iter).second != SubscriptionIdClass::null()) {
				clearRemoveId((*iter).second);
				// We do not want to send a NULL message to it.
				// if we are removing due to return value.
			}
		}
		if (((int)resp.mResp) & EventResponse::CANCEL_EVENT) {
			std::cout << "CANCEL_EVENT";
			cancel = true;
		}
		std::cout << std::endl;
		iter = next;
	}
	return cancel;
}


template <class T>
void EventManager<T>::temporary_processEventQueue(AbsTime forceCompletionBy) {
	AbsTime startTime = AbsTime::now();
	std::cout << " >>> Processing events." << std::endl;
	std::list<EventListener> procListeners;
	{
		boost::lock_guard<boost::mutex>(*listenerlock);
		mRemovedListeners.swap(procListeners);
	}

	for (typename std::list<EventListener>::iterator iter = procListeners.begin();
			iter != procListeners.end();
			++iter) {
		std::cout << " >>>\tNotifying UNSUBSCRIBED listener." << std::endl;
		(*iter) (EventPtr((Event*)NULL));
	}

	EventList processingList; // allow people to keep adding new events
	{
		boost::lock_guard<boost::mutex>(*eventlock);
		mUnprocessed.swap(processingList);
	}

	typename EventList::iterator evIter;
	for (evIter = processingList.begin();
			evIter != processingList.end();
			++evIter) {
		EventPtr ev = (*evIter);
		typename PrimaryListenerMap::iterator priIter;
		PartiallyOrderedListenerList *primaryLists;
		SecondaryListenerMap *secondaryMap;

		{
			boost::lock_guard<boost::mutex> map_lookup(*listenerlock);
			priIter = mListeners.find(ev->getId().mPriId);
			if (priIter == mListeners.end()) {
				// FIXME: Should this ever happen?
				std::cerr << " >>>\tWARNING: No listeners for type " <<
				"event type " << ev->getId().mPriId << std::endl;
				continue;
			}
			primaryLists =
				&((*priIter).second->first);
			secondaryMap =
				&((*priIter).second->second);
		}
        bool cancel = false;
        EventHistory eventHistory=EVENT_UNHANDLED;
		// Call once per event order.
		for (int i = 0; i < NUM_EVENTORDER && cancel == false; i++) {
			boost::unique_lock<boost::mutex> lock_for_map_lookup(*listenerlock);
			std::cout << " >>>\tFiring " << ev->getId() <<
				" [order " << i << "]" << std::endl;
			mProcessingList = &(primaryLists->get(i));
			lock_for_map_lookup.unlock();
            if (!mProcessingList->empty())
                eventHistory=EVENT_HANDLED;
			if (callAllListeners(ev, mProcessingList, forceCompletionBy)) {
				cancel = cancel || true;
			}
			lock_for_map_lookup.lock();
			mProcessingList = NULL;
			/*
			 * We must do this lookup at each iteration because
			 * it is possible for elements in secondaryMap to be "cleaned up"
			 * and removed.
			 * On the other hand, because primary event types are supposed to
			 * be a fixed set of ids, it is not worthwhile cleaning them up.
			 */
			typename SecondaryListenerMap::iterator secIter;
			secIter = secondaryMap->find(ev->getId().mSecId);
			if (secIter != secondaryMap->end() &&
					!(*secIter).second->get(i).empty()) {
				mProcessingList = &((*secIter).second->get(i));
				lock_for_map_lookup.unlock();
                if (!mProcessingList->empty())
                    eventHistory=EVENT_HANDLED;

				if (callAllListeners(ev, mProcessingList, forceCompletionBy)) {
					cancel = cancel || true;
				}
				lock_for_map_lookup.lock();
				mProcessingList = NULL;
				// all listeners may have returned false.
				secIter = secondaryMap->find(ev->getId().mSecId);
				cleanUp(secondaryMap, secIter);

				// secIter may be invalidated.
				secIter = secondaryMap->end();
			}
			lock_for_map_lookup.unlock();

			std::set<SubscriptionId>::const_iterator unsubIter;
			for (unsubIter = mProcessingUnsubscribe.begin();
					unsubIter != mProcessingUnsubscribe.end();
					++unsubIter) {
			 	unsubscribe(*unsubIter, false);
			}
			mProcessingUnsubscribe.clear();

			if (cancel) {
				std::cout << " >>>\tCancelling " << ev->getId() << std::endl;
			}
		}
        if (cancel) eventHistory=EVENT_CANCELED;
        (*ev)(eventHistory);
		std::cout << " >>>\tFinished " << ev->getId() << std::endl;
	}

	AbsTime finishTime = AbsTime::now();
	std::cout << "**** Done processing events this round. " <<
		"Took " << (float)(finishTime-startTime) <<
		" seconds." << std::endl;
}

// instantiate any versions of this queue.
template class EventManager<Event>;

}
}
