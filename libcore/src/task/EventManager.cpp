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
#include "util/Standard.hh"
#include "EventManager.hpp"
#include "UniqueId.hpp"
#include "Event.hpp"

#include "TimerQueue.hpp"

#include <iostream>

namespace Sirikata {

/**
 * EventManager.cpp -- Definition for EventManager functions;
 * Also includes explicit template instantiations for EventManager<Event>
 */



namespace Task {


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

template <class T>
void EventManager<T>::subscribe(const IdPair &eventId,
			const EventListener &listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	mListenerRequests.push(ListenerRequest(
			SubscriptionIdClass::null(),
			eventId, listener, whichOrder));
}

template <class T>
void EventManager<T>::subscribe(const IdPair::Primary & primaryId,
			const EventListener & listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	mListenerRequests.push(ListenerRequest(
			SubscriptionIdClass::null(),
			IdPair(primaryId), listener, whichOrder));
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

	mListenerRequests.push(ListenerRequest(
			removeId,
			eventId, listener, whichOrder));

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

	mListenerRequests.push(ListenerRequest(
			removeId,
			IdPair(priId), listener, whichOrder));

	return removeId;
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
typename EventManager<T>::ListenerList::iterator
EventManager<T>::addListener(ListenerList *insertList,
		const EventListener &listener,
		SubscriptionId removeId)
{
	return insertList->insert( insertList->begin(),
		ListenerSubscriptionInfo(listener, removeId));
}

template <class T>
void EventManager<T>::doSubscribeId(
		const ListenerRequest &req)
{
	PrimaryListenerInfo *newPrimary = insertPriId(req.eventId.mPriId);
	ListenerList *insertList;
	SecondaryListenerMap *secondListeners = NULL;
	if (req.onlyPrimary) {
		insertList = &(newPrimary->first.get(req.whichOrder));
	} else {
		secondListeners = &(newPrimary->second);
		typename SecondaryListenerMap::iterator secondIter =
			insertSecId(*secondListeners, req.eventId.mSecId);
		insertList = &((*secondIter).second->get(req.whichOrder));
	}
	typename ListenerList::iterator iter =
		addListener(insertList, req.listenerFunc, req.listenerId);

	if (req.listenerId != SubscriptionIdClass::null()) {
		mRemoveById.insert(
			typename RemoveMap::value_type(req.listenerId,
				EventSubscriptionInfo(
					insertList,
					iter,
					secondListeners,
					req.eventId.mSecId)));
	}
}

// ============= UNSUBSCRIPTION FUNCTIONS ==============

template <class T>
void EventManager<T>::clearRemoveId(
			SubscriptionId removeId)
{
	typename RemoveMap::iterator iter = mRemoveById.find(removeId);
	if (iter == mRemoveById.end()) {
		std::cerr << "!!! Failed to clear removeId " << removeId <<
			" -- usually this is from a double-unsubscribe. " << std::endl;
		//int double_unsubscribe = 0;
		//assert(double_unsubscribe);
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
	mListenerRequests.push(ListenerRequest(
			removeId, notifyListener));
}

template <class T>
void EventManager<T>::doUnsubscribe(
			SubscriptionId removeId,
			bool notifyListener)
{
	typename RemoveMap::iterator iter = mRemoveById.find(removeId);
	if (iter == mRemoveById.end()) {
		std::cerr << "!!! Unsubscribe for removeId " << removeId <<
			" -- usually this is from a double-unsubscribe. ";
	} else {
		EventSubscriptionInfo &subInfo = (*iter).second;
		std::cout << "**** Unsubscribe " << removeId;
		if (notifyListener) {
			(*subInfo.mIter).first(EventPtr());
		}
		subInfo.mList->erase(subInfo.mIter);
		if (subInfo.secondaryMap) {
			std::cout << " with Secondary ID " <<
				subInfo.secondaryId << std::endl << "\t";
			typename SecondaryListenerMap::iterator deleteIter =
				subInfo.secondaryMap->find(subInfo.secondaryId);
			assert(!(deleteIter==subInfo.secondaryMap->end()));
			cleanUp(subInfo.secondaryMap, deleteIter);
		}

		mRemoveById.erase(iter);
		SubscriptionIdClass::free(removeId);
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
	std::cout << "**** Firing event " << (void*)(&(*ev)) <<
		" with " << ev->getId() << std::endl;
	mUnprocessed.push(ev);
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
			if ((*iter).second != SubscriptionIdClass::null()) {
				clearRemoveId((*iter).second);
				// We do not want to send a NULL message to it.
				// if we are removing due to return value.
			}
			lili->erase(iter);
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

	{
		typename ListenerRequestList::NodeIterator procListeners(mListenerRequests);

		const ListenerRequest *req;
		while ((req = procListeners.next()) != NULL) {
			if (req->subscription) {
				std::cout << " >>>\tDoing subscription listener "<< req->listenerId << " for event " << req->eventId << " (" << req->onlyPrimary <<  ")." << std::endl;
				doSubscribeId(*req);
			} else {
				std::cout << " >>>\t";
				if (req->notifyListener) {
					std::cout << "Notifying";
				}
				std::cout << "UNSUBSCRIBED listener " << req->listenerId << "." << std::endl;
				doUnsubscribe(req->listenerId, req->notifyListener);
			}
		}
	}

	// allow people to keep adding new events
	typename EventList::NodeIterator processingList(mUnprocessed);

	EventPtr *evTemp;

	while ((evTemp = processingList.next())!=NULL) {
		EventPtr ev (*evTemp);

		typename PrimaryListenerMap::iterator priIter =
			mListeners.find(ev->getId().mPriId);
		if (priIter == mListeners.end()) {
			// FIXME: Should this ever happen?
			std::cerr << " >>>\tWARNING: No listeners for type " <<
			"event type " << ev->getId().mPriId << std::endl;
			continue;
		}

		PartiallyOrderedListenerList *primaryLists =
			&((*priIter).second->first);
		SecondaryListenerMap *secondaryMap =
			&((*priIter).second->second);

		typename SecondaryListenerMap::iterator secIter;
		secIter = secondaryMap->find(ev->getId().mSecId);

        bool cancel = false;
        EventHistory eventHistory=EVENT_UNHANDLED;
		// Call once per event order.
		for (int i = 0; i < NUM_EVENTORDER && cancel == false; i++) {
			std::cout << " >>>\tFiring " << ev->getId() <<
				" [order " << i << "]" << std::endl;
			ListenerList *currentList = &(primaryLists->get(i));
			if (!currentList->empty())
				eventHistory=EVENT_HANDLED;
			if (callAllListeners(ev, currentList, forceCompletionBy)) {
				cancel = cancel || true;
			}

			if (secIter != secondaryMap->end() &&
					!(*secIter).second->get(i).empty()) {
				currentList = &((*secIter).second->get(i));
				if (!currentList->empty())
					eventHistory=EVENT_HANDLED;

				if (callAllListeners(ev, currentList, forceCompletionBy)) {
					cancel = cancel || true;
				}
				// all listeners may have returned false.
				// cleanUp(secondaryMap, secIter);
				// secIter = secondaryMap->find(ev->getId().mSecId);
			}

			if (cancel) {
				std::cout << " >>>\tCancelling " << ev->getId() << std::endl;
			}
		}
		if (secIter != secondaryMap->end()) {
			cleanUp(secondaryMap, secIter);
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
