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
        std::cout<<"\n\nGot into 3 notify in listenerProvider.hpp\n\n";
        for (int32 i=(int32)mListeners.size()-1;
             i>=0&&i<(int32)mListeners.size();
             --i) {
            std::cout<<"\n\nNotifying i: "<<i<<"\n\n";
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

/**
 * The MarkovianProvider1 provides the Provider interface
 * The markovian provider recalls the last item sent to a listener and notifies new listeners withthis value
 * The provider only supports a single Listener function called notify which takes 1 argument
 */
template<typename ListenerPtr, class A> class MarkovianProvider1:public Provider<ListenerPtr> {
    A mA;
protected:
    void notify(const A&a) {
        mA=a;
        for (int32 i=(int32)this->mListeners.size()-1;
             i>=0&&i<(int32)this->mListeners.size();
             --i) {
            this->mListeners[i]->notify(mA);
        }
    }
public:
    MarkovianProvider1(const A&a):mA(a) {}
    virtual void addListener(ListenerPtr p) {
        p->notify(mA);
        this->Provider<ListenerPtr>::addListener(p);
    }
    virtual void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

/**
 * The MarkovianProvider2 provides the Provider interface
 * The markovian provider recalls the last item sent to a listener and notifies new listeners withthis value
 * The provider only supports a single Listener function called notify which takes 2 arguments
 */
template<typename ListenerPtr, class A, class B> class MarkovianProvider2:public Provider<ListenerPtr> {
    A mA;
    B mB;
protected:
    void notify(const A&a, const B&b) {
        mA=a;
        mB=b;
        for (int32 i=(int32)this->mListeners.size()-1;
             i>=0&&i<(int32)this->mListeners.size();
             --i) {
            this->mListeners[i]->notify(mA,mB);
        }
    }
public:
    MarkovianProvider2(const A&a, const B&b):mA(a),mB(b) {}
    void addListener(ListenerPtr p) {
        p->notify(mA,mB);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

/**
 * The MarkovianProvider3 provides the Provider interface
 * The markovian provider recalls the last item sent to a listener and notifies new listeners withthis value
 * The provider only supports a single Listener function called notify which takes 3 arguments
 */
template<typename ListenerPtr, class A, class B, class C> class MarkovianProvider3:public Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
protected:
    void notify(const A&a, const B&b, const C&c) {
        mA=a;
        mB=b;
        mC=c;
        for (int32 i=(int32)this->mListeners.size()-1;
             i>=0&&i<(int32)this->mListeners.size();
             --i) {
            this->mListeners[i]->notify(mA,mB,mC);
        }
    }
public:
    MarkovianProvider3(const A&a, const B&b, const C&c):mA(a),mB(b),mC(c) {}
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

/**
 * The MarkovianProvider4 provides the Provider interface
 * The markovian provider recalls the last item sent to a listener and notifies new listeners withthis value
 * The provider only supports a single Listener function called notify which takes 4 arguments
 */
template<typename ListenerPtr, class A, class B, class C, class D> class MarkovianProvider4:public Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
    D mD;
protected:
    void notify(const A&a, const B&b, const C&c, const D&d) {
        mA=a;
        mB=b;
        mC=c;
        mD=d;
        for (int32 i=(int32)this->mListeners.size()-1;
             i>=0&&i<(int32)this->mListeners.size();
             --i) {
            this->mListeners[i]->notify(mA,mB,mC,mD);
        }
    }
public:
    MarkovianProvider4(const A&a, const B&b, const C&c, const D&d):mA(a),mB(b),mC(c),mD(d) {}
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC,mD);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};
/**
 * The MarkovianProvider5 provides the Provider interface
 * The markovian provider recalls the last item sent to a listener and notifies new listeners withthis value
 * The provider only supports a single Listener function called notify which takes 5 arguments
 */
template<typename ListenerPtr, class A, class B, class C, class D, class E>
 class MarkovianProvider5:public Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
    D mD;
    E mE;
protected:
    void notify(const A&a, const B&b, const C&c, const D&d, const E&e) {
        mA=a;
        mB=b;
        mC=c;
        mD=d;
        mE=e;
        for (int32 i=(int32)this->mListeners.size()-1;
             i>=0&&i<(int32)this->mListeners.size();
             --i) {
            this->mListeners[i]->notify(mA,mB,mC,mD,mE);
        }
    }
public:
    MarkovianProvider5(const A&a, const B&b, const C&c,const D&d, const E&e):mA(a),mB(b),mC(c),mD(d),mE(e) {}
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC,mD,mE);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

/**
 * The MarkovianProvider6 provides the Provider interface
 * The markovian provider recalls the last item sent to a listener and notifies new listeners withthis value
 * The provider only supports a single Listener function called notify which takes 6 arguments
 */
template<typename ListenerPtr, class A, class B, class C, class D, class E, class F>
 class MarkovianProvider6:public Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
    D mD;
    E mE;
    F mF;
protected:
    void notify(const A&a, const B&b, const C&c, const D&d, const E&e, const F&f) {
        mA=a;
        mB=b;
        mC=c;
        mD=d;
        mE=e;
        mF=f;
        for (int32 i=(int32)this->mListeners.size()-1;
             i>=0&&i<(int32)this->mListeners.size();
             --i) {
            this->mListeners[i]->notify(mA,mB,mC,mD,mE,mF);
        }
    }
public:
    MarkovianProvider6(const A&a, const B&b, const C&c,const D&d, const E&e, const F&f):mA(a),mB(b),mC(c),mD(d),mE(e),mF(f) {}
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC,mD,mE,mF);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};


}
#endif
