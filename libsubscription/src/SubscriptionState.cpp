#include <subscription/Platform.hpp>

#include "network/TCPStream.hpp"
#include "network/TCPStreamListener.hpp"
#include "util/UUID.hpp"
#include "subscription/Server.hpp"
#include "subscription/SubscriptionState.hpp"



namespace Sirikata { namespace Subscription {
SubscriptionState::SubscriptionState(Network::Stream*broadcaster):mName(UUID::null()),mLatestSentTime(Time::null()),mLatestUnsentTime(Time::null()){
    mEpoch=ReservedEpoch;
    mBroadcaster=broadcaster;
    mEverReceivedMessage=false;
    mPolling=false;
}
Time SubscriptionState::pushSubscriber(Subscriber*subscriber,std::vector<SubscriberTimePair>*heap) {
    Time retval=subscriber->computeNextUpdateFromNow();
    heap->push_back(SubscriberTimePair(retval,subscriber));
    std::push_heap(heap->begin(),heap->end());
    return retval;
}

void SubscriptionState::pushNewSubscriber(Subscriber*s,const Time&oldTime) {
    mUnsentSubscribersHeap.push_back(SubscriberTimePair(oldTime,s));
    std::push_heap(mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());
    if (mLatestUnsentTime<oldTime) {
        mLatestUnsentTime=oldTime;
    }
}
void SubscriptionState::pushJustReceivedSubscriber(Subscriber*s) {
    Time retval=pushSubscriber(s,&mSentSubscribersHeap);
    if (mLatestSentTime<retval) {
        mLatestSentTime=retval;
    }
}
void SubscriptionState::registerSubscriber(const std::tr1::shared_ptr<Network::Stream>&stream, const Protocol::Subscribe&subscriptionMessage){
    Subscriber*subscriber =new Subscriber(stream,subscriptionMessage);
    if (mEverReceivedMessage)
        subscriber->broadcast(MemoryReference(mLastSentMessage));
    pushJustReceivedSubscriber(subscriber);
}


void SubscriptionState::broadcast(Server*poll,const MemoryReference&data){
    Time now=Time::now();
    std::vector<SubscriberTimePair> newUnsenders;
    if (mLatestSentTime<now||mLatestUnsentTime<now) {//there exists a completing queue
        Time latestSentTime=mLatestSentTime;
        bool unsentSwap=false;
        if (!(mLatestSentTime<now)) {//if the queue of those items that were marked as having sent the last packet isn't fully ready to send
            mLatestSentTime=now;//make sure to reset the time for things in the sent queue
            mSentSubscribersHeap.swap(mUnsentSubscribersHeap);//swap it with that queue that is... since by the end of this operation all items that haven't sent this latest packet won't
                                                              //have sent the latest packet and can be placed in the unsent heap
            unsentSwap=true;//swap the latest times
        }
        if (mLatestSentTime<now&&mLatestUnsentTime<now) {//if both queues are going to empty, combine them as the sent queue
            mSentSubscribersHeap.insert(mSentSubscribersHeap.end(),mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());            
            mUnsentSubscribersHeap.clear();
        }else if (!mUnsentSubscribersHeap.empty()) {//otherwise pop items off the Unsent queue onto the sent queue for mass processing
            if (unsentSwap)
                mLatestUnsentTime=latestSentTime;
            while(mUnsentSubscribersHeap.front().mNextUpdateTime<now) {
                mSentSubscribersHeap.push_back(mUnsentSubscribersHeap.front());
                std::pop_heap(mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());                
                mUnsentSubscribersHeap.pop_back();
            }
        }
        std::vector<SubscriberTimePair>::iterator i,ie=mSentSubscribersHeap.end();
        for (i=mSentSubscribersHeap.begin(); i!=ie;++i) {
            i->mSubscriber->broadcast(data);
            i->mNextUpdateTime=i->mSubscriber->computeNextUpdateFromNow();
            if (mLatestSentTime<i->mNextUpdateTime) {
                mLatestSentTime=i->mNextUpdateTime;
            }
        }
        std::make_heap(mSentSubscribersHeap.begin(),mSentSubscribersHeap.end());//since the sent queue's heap property was violated, reheapify
    }else {
        //neither queue completes, meaning both will have partial entries that will need to be merged into a heap
        std::vector<SubscriberTimePair> temp,sent;
        sent.swap(mSentSubscribersHeap);
        mLatestSentTime=now;
        std::vector<SubscriberTimePair>::iterator unsentHeapEnd=mUnsentSubscribersHeap.end(),sentEnd=sent.end(),iter;
        size_t unsentSize=mUnsentSubscribersHeap.size();
        for (size_t i=0;i<unsentSize;++i) {
            if (mUnsentSubscribersHeap.front().mNextUpdateTime<now) {
                mUnsentSubscribersHeap.front().mSubscriber->broadcast(data);
                std::pop_heap(mUnsentSubscribersHeap.begin(),unsentHeapEnd);
                --unsentHeapEnd;
                pushJustReceivedSubscriber(unsentHeapEnd->mSubscriber);
            }else {
                if (unsentHeapEnd!=mUnsentSubscribersHeap.end())
                    mUnsentSubscribersHeap.erase(unsentHeapEnd,mUnsentSubscribersHeap.end());
                break;
            }
        }
        size_t sentSize=sent.size();
        for (size_t i=0;i<sentSize;++i) {
            if (sent.front().mNextUpdateTime<now) {
                sent.front().mSubscriber->broadcast(data);
                std::pop_heap(sent.begin(),sentEnd);
                --sentEnd;
                pushJustReceivedSubscriber(sentEnd->mSubscriber);
            }else {
                for (iter=sent.begin();iter!=sentEnd;++iter) {
                    pushNewSubscriber(iter->mSubscriber,iter->mNextUpdateTime);
                }
                break;
            }
        }
    }
}

SubscriptionState::~SubscriptionState(){
    std::vector<SubscriberTimePair>::iterator i=mUnsentSubscribersHeap.begin(),ie=mUnsentSubscribersHeap.end();
    for (;i!=ie;++i) {
        delete i->mSubscriber;
    }
    i=mSentSubscribersHeap.begin();
    ie=mSentSubscribersHeap.end();
    for (;i!=ie;++i) {
        delete i->mSubscriber;
    }
    delete mBroadcaster;
}

SubscriptionState::Subscriber::Subscriber(const std::tr1::shared_ptr<Network::Stream>&sender,const Protocol::Subscribe&msg):mSender(sender),mPeriod(msg.has_update_period()?msg.update_period():Duration::microseconds(0)) {
    mSentEpoch=ReservedEpoch;
}
void SubscriptionState::Subscriber::broadcast(const MemoryReference&data){
    std::tr1::shared_ptr<Network::Stream> sender=mSender.lock();
    if (sender) {
        sender->send(data,Network::ReliableOrdered);
    }
}
void SubscriptionState::poll(Server*parent) {
    Time now=Time::now();
    while (!mUnsentSubscribersHeap.empty()) {
        SubscriberTimePair* iter=&mUnsentSubscribersHeap.front();
        if (mUnsentSubscribersHeap.front().mNextUpdateTime<now) {            
            iter->mSubscriber->broadcast(MemoryReference(mLastSentMessage));
            pushJustReceivedSubscriber(iter->mSubscriber);
            std::pop_heap(mUnsentSubscribersHeap.begin(),mUnsentSubscribersHeap.end());
            mUnsentSubscribersHeap.pop_back();
        }else {
            assert(mName!=UUID::null());
            parent->initiatePolling(mName,mUnsentSubscribersHeap.front().mNextUpdateTime-now);
            break;
        }
    }
    if (mUnsentSubscribersHeap.empty()) {
        mPolling=false;
    }
}

Time SubscriptionState::Subscriber::computeNextUpdateFromNow(){
    return Time::now()+mPeriod;
}




} }

