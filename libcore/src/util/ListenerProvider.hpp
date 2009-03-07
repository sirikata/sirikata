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
public:
    virtual ~Provider(){}
    virtual void listenerAdded(ListenerPtr ){}
    virtual void listenerRemoved(ListenerPtr ){}
    virtual void firstListenerAdded(ListenerPtr ){}
    virtual void lastListenerRemoved(ListenerPtr ){}
    template <typename T, typename A> void notify(T func, A newA){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            ((&**i)->*func)(newA);
        }
    }
    template <typename T, typename A, typename B>
      void notify(T func, A newA, B newB){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            ((&**i)->*func)(newA,newB);
        }
    }
    template <typename T, typename A, typename B, typename C>
      void notify(T func, A newA, B newB, C newC){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            ((&**i)->*func)(newA,newB,newC);
        }
    }
    template <typename T, typename A, typename B, typename C, typename D> 
      void notify(T func, A newA, B newB, C newC, D newD){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            ((&**i)->*func)(newA,newB,newC,newD);
        }
    }
    template <typename T, typename A, typename B, typename C, typename D, typename E> 
      void notify(T func, A newA, B newB, C newC, D newD, E newE){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            ((&**i)->*func)(newA,newB,newC,newD,newE);
        }
    }
    template <typename T, typename A, typename B, typename C, typename D, typename E, typename F> 
      void notify(T func, A newA, B newB, C newC, D newD, E newE, F newF){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            ((&**i)->*func)(newA,newB,newC,newD,newE,newF);
        }
    }
public:
    virtual void addListener(ListenerPtr p) {
        mListenerIndex[p]=mListeners.size();
        mListeners.push_back(p);
        if (mListeners.size()==1)
            this->firstListenerAdded(p);
        this->listenerAdded(p);
    }
    virtual void removeListener(ListenerPtr p) {
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
            this->listenerRemoved(p);
            assert(mListeners[0]==p);
            mListeners.resize(0);
            mListenerIndex=ListenerMap();
        }
    }
};
template<typename ListenerPtr, class A> class MarkovianProvider1:protected Provider<ListenerPtr> {
    A mA;
protected:
    void notify(const A&a) {
        mA=a;
        for(typename std::vector<ListenerPtr>::iterator i=this->mListeners.begin()
                ,ie=this->mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(mA);
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

template<typename ListenerPtr, class A, class B> class MarkovianProvider2:protected Provider<ListenerPtr> {
    A mA;
    B mB;
protected:
    void notify(const A&a, const B&b) {
        mA=a;
        mB=b;
        for(typename std::vector<ListenerPtr>::iterator i=this->mListeners.begin()
                ,ie=this->mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(mA,mB);
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

template<typename ListenerPtr, class A, class B, class C> class MarkovianProvider3:protected Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
protected:
    void notify(const A&a, const B&b, const C&c) {
        mA=a;
        mB=b;
        mC=c;
        for(typename std::vector<ListenerPtr>::iterator i=this->mListeners.begin()
                ,ie=this->mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(mA,mB,mC);
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

template<typename ListenerPtr, class A, class B, class C, class D> class MarkovianProvider4:protected Provider<ListenerPtr> {
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
        for(typename std::vector<ListenerPtr>::iterator i=this->mListeners.begin()
                ,ie=this->mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(mA,mB,mC,mD);
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

template<typename ListenerPtr, class A, class B, class C, class D, class E> 
 class MarkovianProvider5:protected Provider<ListenerPtr> {
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
        for(typename std::vector<ListenerPtr>::iterator i=this->mListeners.begin()
                ,ie=this->mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(mA,mB,mC,mD,mE);
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

template<typename ListenerPtr, class A, class B, class C, class D, class E, class F> 
 class MarkovianProvider6:protected Provider<ListenerPtr> {
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
        for(typename std::vector<ListenerPtr>::iterator i=this->mListeners.begin()
                ,ie=this->mListeners.end();
            i!=ie;
            ++i) {
                    (*i)->notify(mA,mB,mC,mD,mE,mF);
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
