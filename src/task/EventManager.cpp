/*     Iridium Kernel -- Task scheduling system
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

#include "EventManager.hpp"
#include "Time.hpp"
#include "Subscription.hpp"
#include "Event.hpp"

#include "TimerQueue.hpp"

#include <iostream>

namespace Iridium {

/**
 * EventManager.cpp -- Definition for EventManager functions;
 * Also includes explicit template instantiations for EventManager<Event>
 */
namespace Task {

template <class T>
typename EventManager<T>::PrimaryListenerInfo &
	EventManager<T>::insertPriId(
			const IdPair::Primary &pri)
{
	typename PrimaryListenerMap::iterator iter = mListeners.find(pri);
	if (iter == mListeners.end()) {
		iter = mListeners.insert(
			typename PrimaryListenerMap::value_type(
				pri, PrimaryListenerInfo())
			).first;
	}
	return (*iter).second;
}

	
template <class T>
typename EventManager<T>::PartiallyOrderedListenerList &
	EventManager<T>::insertSecId(
			SecondaryListenerMap &secondListeners,
			const IdPair::Secondary &sec)
{
	typename SecondaryListenerMap::iterator iter2 = secondListeners.find(sec);
	if (iter2 == secondListeners.end()) {
		iter2 = secondListeners.insert(
			typename SecondaryListenerMap::value_type(
				sec, PartiallyOrderedListenerList())
			).first;
	}
	return (*iter2).second;
}

template <class T>
void EventManager<T>::subscribe(const IdPair &eventId,
			const EventListener &listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	SecondaryListenerMap &secondListeners = insertPriId(eventId.mPriId).second;
	ListenerList &insertList =
		insertSecId(secondListeners, eventId.mSecId)[whichOrder];
	insertList.push_back(ListenerSubscriptionInfo(listener, SubscriptionId::null()));
}

template <class T>
void EventManager<T>::subscribe(const IdPair::Primary & primaryId,
			const EventListener & listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	ListenerList &insertList = insertPriId(primaryId).first[whichOrder];
	insertList.push_back(ListenerSubscriptionInfo(listener, SubscriptionId::null()));
}
	
template <class T>
void EventManager<T>::subscribe(
			const IdPair & eventId,
			const EventListener & listener,
			const SubscriptionId & removeId,
			EventOrder whichOrder)
{
	if (removeId == SubscriptionId::null()) {
		subscribe(eventId, listener, whichOrder);
		return;
	}
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	SecondaryListenerMap &secondListeners = insertPriId(eventId.mPriId).second;
	ListenerList &insertList =
		insertSecId(secondListeners, eventId.mSecId)[whichOrder];
	insertList.push_back(ListenerSubscriptionInfo(listener, removeId));

	typename ListenerList::iterator iter = insertList.end();
	--iter;
	EventSubscriptionInfo info (insertList, iter);
	unsubscribe(removeId); // if anything was there before, remove it.
	mRemoveById.insert(typename RemoveMap::value_type(removeId, info));

}
template <class T>
void EventManager<T>::subscribe(
			const IdPair::Primary & priId,
			const EventListener & listener,
			const SubscriptionId & removeId,
			EventOrder whichOrder)
{
	if (removeId == SubscriptionId::null()) {
		subscribe(priId, listener, whichOrder);
		return;
	}
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	ListenerList &insertList = insertPriId(priId).first[whichOrder];
	insertList.push_back(ListenerSubscriptionInfo(listener, removeId));

	typename ListenerList::iterator iter = insertList.end();
	--iter;
	EventSubscriptionInfo info (
//				EventOrder(priId, IdPair::Secondary::null()),
//				whichOrder,
				insertList,
				iter);
	unsubscribe(removeId); // if anything was there before, remove it.
	mRemoveById.insert(typename RemoveMap::value_type(removeId, info));
}
	
template <class T>
void EventManager<T>::unsubscribe(
			const SubscriptionId &removeId)
{
	typename RemoveMap::iterator iter = mRemoveById.find(removeId);
	if (iter == mRemoveById.end()) {
		return; // throw an exception?
	}

	(*iter).second.mList.erase((*iter).second.mIter);
	mRemoveById.erase(iter);
}


template <class T>
int EventManager<T>::clearListenerList(
			ListenerList &list)
{
	SubscriptionId nullId = SubscriptionId::null();
	
	for (typename ListenerList::iterator iter = list.begin();
			iter!=list.end(); ++iter) {
		if (!((*iter).second == nullId)) {
			typename RemoveMap::iterator subIdIter;
			subIdIter = mRemoveById.find((*iter).second);
			if (subIdIter != mRemoveById.end()) {
				mRemoveById.erase(subIdIter);
			}
		}
	}
	int ret = list.size();
	if (mProcessingList == &list) {
		if (mClearCurrentList) {
			ret = 0; // already "cleared".
		}
		mClearCurrentList = true;
	} else {
		list.clear();
	}
	return ret;
}

template <class T>
bool EventManager<T>::cleanUp(
	typename PrimaryListenerMap::iterator &iter)
{
	bool isEmpty = true;
	for (int i = 0; i < NUM_EVENTORDER; i++) {
		isEmpty = isEmpty && (*iter).second.first[i].empty();
	}
	isEmpty = isEmpty && (*iter).second.second.empty();

	if (isEmpty) {
		mListeners.erase(iter);
	}
	return isEmpty;
}

template <class T>
bool EventManager<T>::cleanUp(
	SecondaryListenerMap &slm,
	typename SecondaryListenerMap::iterator &slm_iter)
{
	bool isEmpty = true;
	for (int i = 0; i < NUM_EVENTORDER; i++) {
		isEmpty = isEmpty && (*slm_iter).second[i].empty();
	}
	if (isEmpty) {
		slm.erase(slm_iter);
	}
	return isEmpty;
}

template <class T>
int EventManager<T>::removeAllByInterest(
			const IdPair &eventId)
{
	int total = 0;
	
	typename PrimaryListenerMap::iterator iter = mListeners.find(eventId.mPriId);
	if (iter == mListeners.end()) {
		return 0;
	}
	SecondaryListenerMap &secondListeners = (*iter).second.second;
	typename SecondaryListenerMap::iterator iter2;
	iter2 = secondListeners.find(eventId.mSecId);
	if (iter2 == secondListeners.end()) {
		return 0;
	}
	for (int i = 0; i < NUM_EVENTORDER; i++) {
		total += (*iter2).second[i].size();
		clearListenerList((*iter2).second[i]);
	}

	bool isEmpty = cleanUp(secondListeners, iter2);
	if (isEmpty) {
		cleanUp(iter);
	}

	// List should not be empty if we did not remove anything.
	assert(!(total == 0 && isEmpty == true));
	return total;
}


template <class T>
int EventManager<T>::removeAllByInterest(
			const IdPair::Primary &whichId,
			bool generic,
			bool specific)
{
	int total = 0;
	
	typename PrimaryListenerMap::iterator iter;
	iter = mListeners.find(whichId);
	if (iter == mListeners.end()) {
		return 0;
	}
	if (generic) {
		for (int i = 0; i < NUM_EVENTORDER; i++) {
			total += (*iter).second.first[i].size();
			clearListenerList((*iter).second.first[i]);
		}
	}

	if (specific) {
		SecondaryListenerMap &secmap = (*iter).second.second;
		for (typename SecondaryListenerMap::iterator iter2 = secmap.begin();
				iter2 != secmap.end();
				++iter2) {
			for (int i = 0; i < NUM_EVENTORDER; i++) {
				total += (*iter2).second[i].size();
				clearListenerList((*iter2).second[i]);
			}
		}
		secmap.clear();
	}
	
	bool isEmpty = cleanUp(iter);

	// List should not be empty if we did not remove anything.
	assert(!(total == 0 && isEmpty == true));

	return total;
}

template <class T>
void EventManager<T>::fire(EventPtr ev) {
	mUnprocessed.push_back(ev);
	
};


/* FIXME: We need a "never" constant for AbsTime that is always grreater than
   anything else */

template <class T>
bool EventManager<T>::fireAll(EventPtr ev,
			ListenerList &lili,
			AbsTime forceCompletionBy) {
	mClearCurrentList = false;
	mProcessingList = &lili;

	bool cancel = false;
	typename ListenerList::iterator iter = lili.begin();
	/* FIXME: Disallow 'unsubscribe()' while in this while loop.
	 * We make a temporary mUnsubscribe list, and unsubscribe all of them
	 * after we are done here.
	 */
	while (iter!=lili.end()) {
		typename ListenerList::iterator next = iter;
		++next;
		// Now call the event listener.
		EventResponse resp = (*iter).first(ev);
		if (((int)resp.mResp) & EventResponse::DELETE_LISTENER) {
			lili.erase(iter);
		}
		if (((int)resp.mResp) & EventResponse::CANCEL_EVENT) {
			cancel = true;
		}
		iter = next;
	}
	mProcessingList = NULL;
	if (mClearCurrentList) {
		clearListenerList(lili);
		mClearCurrentList = false;
	}
	return cancel;
}
template <class T>
void EventManager<T>::temporary_processEventQueue(AbsTime forceCompletionBy) {
	mProcessing = true;
	EventList processingList; // allow people to keep adding new events
	mUnprocessed.swap(processingList);

	for (typename EventList::iterator evIter = processingList.begin();
			evIter != processingList.end(); ++evIter) {
		EventPtr ev = (*evIter);
		typename PrimaryListenerMap::iterator priIter = mListeners.find(ev->getId().mPriId);
		if (priIter == mListeners.end()) {
			// FIXME: Should this ever happen?
			std::cerr << "WARNING: No listeners defined for event type " << ev->getId().mPriId << std::endl;
			continue;
		}
		for (int i = 0; i < NUM_EVENTORDER; i++) {
			std::cout << "Firing all " << i << std::endl;
			bool cancel = false;
			cancel = cancel || fireAll(ev, (*priIter).second.first[i], forceCompletionBy);
			typename SecondaryListenerMap::iterator secIter = (*priIter).second.second.find(ev->getId().mSecId);
			if (secIter != (*priIter).second.second.end()) {
				cancel = cancel || fireAll(ev, (*secIter).second[i], forceCompletionBy);
			}
			std::list<SubscriptionId>::const_iterator unsubIter;
			mProcessing = false; // allow unsubscribe() to work.
			for (unsubIter = mUnsubscribeList.begin();
					unsubIter != mUnsubscribeList.end();
					++unsubIter) {
			 	unsubscribe(*unsubIter);
			}
			mUnsubscribeList.clear();
			mProcessing = true;
			//if (secIter != (*priIter).second.second.end()) {
			//	cleanUp((*priIter).second.second, secIter);
			//}
			//cleanUp(priIter);
			// *** FIXME: priIter or secIter may be invalid ***
			if (cancel) {
				std::cout << "Cancelling " << ev->getId() << std::endl;
				break;
			}
		}
		std::cout << "Finished " << ev->getId() << std::endl;
	}
	mProcessing = false;
}

// instantiate any versions of this queue.
template class EventManager<Event>; 

}
}
