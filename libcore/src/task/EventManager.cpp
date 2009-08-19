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

#include "WorkQueue.hpp"
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
struct EventManager<T>::ListenerUnsubRequest : public WorkItem {
	EventManager<T> *mParent;
	SubscriptionId mListenerId;
	bool mNotifyListener;

	ListenerUnsubRequest(EventManager<T> *parent,
			SubscriptionId myId,
			bool notifyListener)
		: mParent(parent),
		  mListenerId(myId),
		  mNotifyListener(notifyListener){
	}

	virtual void operator() () {
		AutoPtr ref(this); // Allow deletion at the end.

		typename RemoveMap::iterator iter = mParent->mRemoveById.find(mListenerId);
		if (iter == mParent->mRemoveById.end()) {
			SILOG(task,warning,"Double-Unsubscribe for removeId " << mListenerId);
		} else {
			EventSubscriptionInfo &subInfo = (*iter).second;
			SILOG(task,debug,"**** Unsubscribe " << mListenerId);
			if (mNotifyListener) {
				(*subInfo.mIter).first(EventPtr());
			}
			subInfo.mList->erase(subInfo.mIter);
			if (subInfo.secondaryMap) {
				SILOG(task,debug," with Secondary ID " <<
						  subInfo.secondaryId << std::endl << "\t");
				typename SecondaryListenerMap::iterator deleteIter =
					subInfo.secondaryMap->find(subInfo.secondaryId);
				assert(!(deleteIter==subInfo.secondaryMap->end()));
				mParent->cleanUp(subInfo.secondaryMap, deleteIter);
			}

			mParent->mRemoveById.erase(iter);
			SubscriptionIdClass::free(mListenerId);
		}
	}
};

template <class T>
struct EventManager<T>::ListenerSubRequest : public WorkItem {
	EventManager<T> *mParent;
	SubscriptionId mListenerId;
	IdPair mEventId;
	EventListener mListenerFunc;
	EventOrder mWhichOrder;
	bool mOnlyPrimary;

	ListenerSubRequest(EventManager<T> *parent,
			SubscriptionId myId,
			const IdPair::Primary &priId,
			const EventListener &listenerFunc,
			EventOrder myOrder)
		: mParent(parent),
		  mListenerId(myId),
		  mEventId(priId),
		  mListenerFunc(listenerFunc),
		  mWhichOrder(myOrder),
		  mOnlyPrimary(true) {
	}

	ListenerSubRequest(EventManager<T> *parent,
			SubscriptionId myId,
			const IdPair &fullId,
			const EventListener &listenerFunc,
			EventOrder myOrder)
		: mParent(parent),
		  mListenerId(myId),
		  mEventId(fullId),
		  mListenerFunc(listenerFunc),
		  mWhichOrder(myOrder),
		  mOnlyPrimary(false) {
	}

	virtual void operator() () {
		AutoPtr ref(this); // Allow deletion at the end.

		PrimaryListenerInfo *newPrimary = mParent->insertPriId(mEventId.mPriId);
		ListenerList *insertList;
		SecondaryListenerMap *secondListeners = NULL;
		if (mOnlyPrimary) {
			insertList = &(newPrimary->first.get(mWhichOrder));
		} else {
			secondListeners = &(newPrimary->second);
			typename SecondaryListenerMap::iterator secondIter =
				mParent->insertSecId(*secondListeners, mEventId.mSecId);
			insertList = &((*secondIter).second->get(mWhichOrder));
		}
		typename ListenerList::iterator iter =
			mParent->addListener(insertList, mListenerFunc, mListenerId);

		if (mListenerId != SubscriptionIdClass::null()) {
			mParent->mRemoveById.insert(
				typename RemoveMap::value_type(mListenerId,
					EventSubscriptionInfo(
						insertList,
						iter,
						secondListeners,
						mEventId.mSecId)));
		}
	}
};

template <class T>
struct EventManager<T>::FireEvent : public WorkItem {
	EventManager<T> *mParent;
	typedef typename EventManager<T>::EventPtr EventPtr;
	EventPtr mEvent;

	FireEvent(EventManager<T> *parent,
			  const EventPtr &event)
		: mParent(parent), mEvent(event) {
	}

	virtual void operator() () {
		AutoPtr ref(this); // Allow deletion at the end.

		mParent->mSubscriptionQueue->dequeueAll();

		typename PrimaryListenerMap::iterator priIter =
			mParent->mListeners.find(mEvent->getId().mPriId);
		if (priIter == mParent->mListeners.end()) {
			SILOG(task,insane," >>>\tNo listeners for type " <<
                  "event type " << mEvent->getId().mPriId);
			return;
		}

		PartiallyOrderedListenerList *primaryLists =
			&((*priIter).second->first);
		SecondaryListenerMap *secondaryMap =
			&((*priIter).second->second);

		typename SecondaryListenerMap::iterator secIter;
		secIter = secondaryMap->find(mEvent->getId().mSecId);

        bool cancel = false;
        EventHistory eventHistory=EVENT_UNHANDLED;
		// Call once per event order.
		for (int i = 0; i < NUM_EVENTORDER && cancel == false; i++) {
			SILOG(task,insane," >>>\tFiring " << mEvent << ": " << mEvent->getId() <<
                  " [order " << i << "]");
			ListenerList *currentList = &(primaryLists->get(i));
			if (!currentList->empty())
				eventHistory=EVENT_HANDLED;
			if (mParent->callAllListeners(mEvent, currentList)) {
				cancel = cancel || true;
			}

			if (secIter != secondaryMap->end() &&
					!(*secIter).second->get(i).empty()) {
				currentList = &((*secIter).second->get(i));
				if (!currentList->empty())
					eventHistory=EVENT_HANDLED;

				if (mParent->callAllListeners(mEvent, currentList)) {
					cancel = cancel || true;
				}
				// all listeners may have returned false.
				// cleanUp(secondaryMap, secIter);
				// secIter = secondaryMap->find(ev->getId().mSecId);
			}

			if (cancel) {
				SILOG(task,insane," >>>\tCancelling " << mEvent->getId());
			}
		}
		if (secIter != secondaryMap->end()) {
			mParent->cleanUp(secondaryMap, secIter);
		}

        if (cancel) eventHistory=EVENT_CANCELED;
        (*mEvent)(eventHistory);
		SILOG(task,insane," >>>\tFinished " << mEvent->getId());
	}
};

template <class T>
EventManager<T>::EventManager(WorkQueue *workQueue)
		: mWorkQueue(workQueue) {
	mSubscriptionQueue = new LockFreeWorkQueue;
}

template <class T>
EventManager<T>::~EventManager() {
	typename PrimaryListenerMap::iterator iter;
	typename SecondaryListenerMap::iterator secIter;
	for (iter = mListeners.begin(); iter != mListeners.end(); ++iter) {
		SecondaryListenerMap *secMap = &((*iter).second->second);
		for (secIter = secMap->begin(); secIter != secMap->end(); ++secIter) {
			delete (*secIter).second;
		}
		delete (*iter).second;
	}
	mListeners.clear();
	delete mSubscriptionQueue;
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

template <class T>
void EventManager<T>::subscribe(const IdPair &eventId,
			const EventListener &listener,
			EventOrder whichOrder)
{
	if (whichOrder < 0 || whichOrder >= NUM_EVENTORDER) {
		throw EventOrderException();
	}

	mSubscriptionQueue->enqueue(new ListenerSubRequest(
			this, SubscriptionIdClass::null(),
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

	mSubscriptionQueue->enqueue(new ListenerSubRequest(
			this, SubscriptionIdClass::null(),
			primaryId, listener, whichOrder));
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

	mSubscriptionQueue->enqueue(new ListenerSubRequest(
			this, removeId,
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

	mSubscriptionQueue->enqueue(new ListenerSubRequest(
			this, removeId,
			priId, listener, whichOrder));

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

// ============= UNSUBSCRIPTION FUNCTIONS ==============

template <class T>
void EventManager<T>::clearRemoveId(
			SubscriptionId removeId)
{
	typename RemoveMap::iterator iter = mRemoveById.find(removeId);
	if (iter == mRemoveById.end()) {
		SILOG(task,error,"!!! Failed to clear removeId " << removeId <<
              " -- usually this is from a double-unsubscribe. ");
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
	mSubscriptionQueue->enqueue(new ListenerUnsubRequest(
			this, removeId, notifyListener));
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
		SILOG(task,debug,"[Cleaning up Secondary ID " << (*slm_iter).first << "]");
		delete (*slm_iter).second;
		slm->erase(slm_iter);
	}
	return isEmpty;
}


// =============== EVENT QUEUE FUNCTIONS ===============

template <class T>
void EventManager<T>::fire(EventPtr ev) {
	mWorkQueue->enqueue(new FireEvent(this, ev));
	SILOG(task,insane,"**** Firing event " << (void*)(&(*ev)) <<
		" with " << ev->getId());
};


/* FIXME: We need a "never" constant for LocalTime that is
   always grreater than anything else */

template <class T>
bool EventManager<T>::callAllListeners(EventPtr ev,
			ListenerList *lili) {

	bool cancel = false;
	typename ListenerList::iterator iter = lili->begin();
	/* Disallow 'unsubscribe()' while in this while loop.
	 * We make a temporary mUnsubscribe list, and unsubscribe all
	 * of them after we are done here.
	 *
	 * The reason for this is 'iter' or 'next' may end up getting
	 * deleted in the process.
	 */
	SILOG(task,insane," >>>\tHas " << lili->size() <<
		" Listeners registered.");
	while (iter!=lili->end()) {
		typename ListenerList::iterator next = iter;
		++next;
		// Now call the event listener.
		SILOG(task,insane," >>>\tCalling " << (*iter).second <<"...");
		EventResponse resp = (*iter).first(ev);
		if (((int)resp.mResp) & EventResponse::DELETE_LISTENER) {
			if (((int)resp.mResp) & EventResponse::CANCEL_EVENT) {
				SILOG(task,insane," >>>\t\tReturned DELETE_LISTENER and CANCEL_EVENT");
			} else {
				SILOG(task,insane," >>>\t\tReturned DELETE_LISTENER");
			}
			if ((*iter).second != SubscriptionIdClass::null()) {
				clearRemoveId((*iter).second);
				// We do not want to send a NULL message to it.
				// if we are removing due to return value.
			}
			lili->erase(iter);
		}
		if (((int)resp.mResp) & EventResponse::CANCEL_EVENT) {
			if (!(((int)resp.mResp) & EventResponse::DELETE_LISTENER)) {
				SILOG(task,insane," >>>\t\tReturned CANCEL_EVENT");
			}
			cancel = true;
		}
		iter = next;
	}
	return cancel;
}

// instantiate any versions of this queue.
template class EventManager<Event>;

}
}
