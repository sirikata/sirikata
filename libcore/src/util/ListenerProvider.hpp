namespace Sirikata {
template <typename ListenerPtr> class Provider {
    typedef std::vector<ListenerPtr> ListenerVector;
    typedef std::map<ListenerPtr,size_t> ListenerMap;
    ListenerVector mListeners;
    ListenerMap mListenerIndex;
public:
    template <typename A> void notify(A newA){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(newA);
        }
    }
    template <typename A, typename B> void notify(A newA, B newB){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(newA,newB);
        }
    }
    template <typename A, typename B, typename C> void notify(A newA, B newB, C newC){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(newA,newB,newC);
        }
    }
    template <typename A, typename B, typename C, typename D> 
      void notify(A newA, B newB, C newC, D newD){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(newA,newB,newC,newD);
        }
    }
    template <typename A, typename B, typename C, typename D, typename E> 
      void notify(A newA, B newB, C newC, D newD, E newE){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(newA,newB,newC,newD,newE);
        }
    }
    template <typename A, typename B, typename C, typename D, typename E, typename F> 
      void notify(A newA, B newB, C newC, D newD, E newE, F newF){
        for(typename ListenerVector::iterator i=mListeners.begin()
                ,ie=mListeners.end();
            i!=ie;
            ++i) {
            (*i)->notify(newA,newB,newC,newD,newE,newF);
        }
    }
    void addListener(ListenerPtr p) {
        mListenerIndex[p]=mListeners.size();
        mListeners.push_back(p);
    }
    void removeListener(ListenerPtr p) {
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
            assert(mListeners[0]==p);
            mListeners.resize(0);
            mListenerIndex=ListenerMap();
        }
    }
};
template<typename ListenerPtr, class A> class StatelessProvider1:protected Provider<ListenerPtr> {
    A mA;
public:
    StatelessProvider1(const A&a):mA(a) {}
    void notify(const A&a) {
        mA=a;
        this->Provider<ListenerPtr>::notify(mA);
    }
    void addListener(ListenerPtr p) {
        p->notify(mA);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

template<typename ListenerPtr, class A, class B> class StatelessProvider2:protected Provider<ListenerPtr> {
    A mA;
    B mB;
public:
    StatelessProvider2(const A&a, const B&b):mA(a),mB(b) {}
    void notify(const A&a, const B&b) {
        mA=a;
        mB=b;
        this->Provider<ListenerPtr>::notify(mA,mB);
    }
    void addListener(ListenerPtr p) {
        p->notify(mA,mB);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

template<typename ListenerPtr, class A, class B, class C> class StatelessProvider3:protected Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
public:
    StatelessProvider3(const A&a, const B&b, const C&c):mA(a),mB(b),mC(c) {}
    void notify(const A&a, const B&b, const C&c) {
        mA=a;
        mB=b;
        mC=c;
        this->Provider<ListenerPtr>::notify(mA,mB,mC);
    }
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

template<typename ListenerPtr, class A, class B, class C, class D> class StatelessProvider4:protected Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
    D mD;
public:
    StatelessProvider4(const A&a, const B&b, const C&c, const D&d):mA(a),mB(b),mC(c),mD(d) {}
    void notify(const A&a, const B&b, const C&c, const D&d) {
        mA=a;
        mB=b;
        mC=c;
        mD=d;
        this->Provider<ListenerPtr>::notify(mA,mB,mC,mD);
    }
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC,mD);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

template<typename ListenerPtr, class A, class B, class C, class D, class E> 
 class StatelessProvider5:protected Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
    D mD;
    E mE;
public:
    StatelessProvider5(const A&a, const B&b, const C&c,const D&d, const E&e):mA(a),mB(b),mC(c),mD(d),mE(e) {}
    void notify(const A&a, const B&b, const C&c, const D&d, const E&e) {
        mA=a;
        mB=b;
        mC=c;
        mD=d;
        mE=e;
        this->Provider<ListenerPtr>::notify(mA,mB,mC,mD,mE);
    }
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC,mD,mE);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};

template<typename ListenerPtr, class A, class B, class C, class D, class E, class F> 
 class StatelessProvider6:protected Provider<ListenerPtr> {
    A mA;
    B mB;
    C mC;
    D mD;
    E mE;
    F mF;
public:
    StatelessProvider6(const A&a, const B&b, const C&c,const D&d, const E&e, const F&f):mA(a),mB(b),mC(c),mD(d),mE(e),mF(f) {}
    void notify(const A&a, const B&b, const C&c, const D&d, const E&e, const F&f) {
        mA=a;
        mB=b;
        mC=c;
        mD=d;
        mE=e;
        mF=f;
        this->Provider<ListenerPtr>::notify(mA,mB,mC,mD,mE,mF);
    }
    void addListener(ListenerPtr p) {
        p->notify(mA,mB,mC,mD,mE,mF);
        this->Provider<ListenerPtr>::addListener(p);
    }
    void removeListener(ListenerPtr p) {
        this->Provider<ListenerPtr>::removeListener(p);
    }
};


}
