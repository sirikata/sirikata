/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  ListenerProvider.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _SIRIKATA_LISTENER_PROVIDER_HPP_
#define _SIRIKATA_LISTENER_PROVIDER_HPP_


namespace Sirikata {
/**
 * This class gives listeners an interface to register themselves and a mechanism to notify listeners
 * Users of this class should remember to notify new listeners
 */
template <typename ListenerPtr> class Provider {
protected:
    typedef std::vector<ListenerPtr> ListenerVector;
    typedef std::map<ListenerPtr,uint32> ListenerMap;
    ///A list of listeners interested in updates from this class
    ListenerVector mListeners;
    ///A map from listener pointers to indexes in the listeners vector
    ListenerMap mListenerIndex;
    virtual ~Provider(){}
   ///This function is called with a new listener just after every listener is added to the callbacks (Override for interesting behavior, such as feeding the initial values to it)
    virtual void listenerAdded(ListenerPtr ){}
   ///This function is called with the dated listener just before that listener is removed from the callbacks (Override for interesting behavior)
    virtual void listenerRemoved(ListenerPtr ){}
   ///This function is called with a new listener when the first listener signs up (Override to propogate network requests for example)
    virtual void firstListenerAdded(ListenerPtr ){}
   ///This function is called with the defunct listener just before the last listener is removed frmo the callbacks. Override for interesting behavior
    virtual void lastListenerRemoved(ListenerPtr ){}
    /**
     *  This function notifies all listeners. Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     */
    template <typename T> void notify(T func){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)();
        }
    }    /**
     *  This function notifies all listeners. Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the singular argument passed to func
     */
    template <typename T, typename A> void notify(T func, A newA){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA);
        }
    }
    /**
     *  This function notifies all listeners. Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     */
    template <typename T, typename A, typename B>
      void notify(T func, A newA, B newB){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB);
        }
    }
    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     */
    template <typename T, typename A, typename B, typename C>
      void notify(T func, A newA, B newB, C newC){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC);
        }
    }
    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     *  \param newD is the fourth argument passed to func
     */
    template <typename T, typename A, typename B, typename C, typename D>
      void notify(T func, A newA, B newB, C newC, D newD){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC,newD);
        }
    }
    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     *  \param newD is the fourth argument passed to func
     *  \param newE is the fifth argument passed to func
     */
    template <typename T, typename A, typename B, typename C, typename D, typename E>
      void notify(T func, A newA, B newB, C newC, D newD, E newE){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC,newD,newE);
        }
    }
    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     *  \param newD is the fourth argument passed to func
     *  \param newE is the fifth argument passed to func
     *  \param newF is the sixth argument passed to func
     */
    template <typename T, typename A, typename B, typename C, typename D, typename E, typename F>
      void notify(T func, A newA, B newB, C newC, D newD, E newE, F newF){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC,newD,newE,newF);
        }
    }

    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     *  \param newD is the fourth argument passed to func
     *  \param newE is the fifth argument passed to func
     *  \param newF is the sixth argument passed to func
     *  \param newG is the seventh argument passed to func
     */
    template <typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G>
    void notify(T func, A newA, B newB, C newC, D newD, E newE, F newF, G newG){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC,newD,newE,newF,newG);
        }
    }

    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     *  \param newD is the fourth argument passed to func
     *  \param newE is the fifth argument passed to func
     *  \param newF is the sixth argument passed to func
     *  \param newG is the seventh argument passed to func
     *  \param newH is the eighth argument passed to func
     */
    template <typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H>
    void notify(T func, A newA, B newB, C newC, D newD, E newE, F newF, G newG, H newH){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC,newD,newE,newF,newG,newH);
        }
    }

    /**
     *  This function notifies all listeners.
     *  Listeners may add other listeners or remove themselves,
     *  though undefined behavior results from removing other listeners during the call.
     *  \param func which must be a member function of ListenerPtr gets called on all listeners
     *  \param newA is the first argument passed to func
     *  \param newB is the second argument passed to func
     *  \param newC is the third argument passed to func
     *  \param newD is the fourth argument passed to func
     *  \param newE is the fifth argument passed to func
     *  \param newF is the sixth argument passed to func
     *  \param newG is the seventh argument passed to func
     *  \param newH is the eighth argument passed to func
     *  \param newI is the eighth argument passed to func
     */
    template <typename T, typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I>
    void notify(T func, A newA, B newB, C newC, D newD, E newE, F newF, G newG, H newH, I newI){
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            ((&*mListeners[i])->*func)(newA,newB,newC,newD,newE,newF,newG,newH,newI);
        }
    }


public:
    /**
     *  This function adds a new listener to listen for notification
     *  This may be called during a notify call, but new functions will not be called until the next notification
     */
    virtual void addListener(ListenerPtr p) {
        if (mListeners.empty()) {
            mListeners.push_back(p);
            this->firstListenerAdded(p);
        }else if (mListeners.size()==1) {
            mListenerIndex[mListeners[0]]=0;
            mListenerIndex[p]=1;
            mListeners.push_back(p);
        }else {
            mListenerIndex[p]=mListeners.size();
            mListeners.push_back(p);
        }
        this->listenerAdded(p);
    }
    /**
     *  This function removes a listener from listening for notification
     *  This may be called during a notify call on the currently notified listener only
     */
    virtual void removeListener(ListenerPtr p) {
        this->listenerRemoved(p);
        if (mListeners.size()>1) {
            typename ListenerMap::iterator where=mListenerIndex.find(p);
            assert(where!=mListenerIndex.end());
            if (where->second+1!=mListeners.size()) {
                mListenerIndex[mListeners.back()]=where->second;
                mListeners[where->second]=mListeners.back();
            }
            mListeners.resize(mListeners.size()-1);
            mListenerIndex.erase(where);
        }else {
            this->lastListenerRemoved(p);
            assert(mListeners[0]==p);
            mListeners.resize(0);
            mListenerIndex=ListenerMap();
        }
    }
};

}
#endif
