namespace Sirikata {
template <typename ListenerPtr> class Provider {
    typedef std::vector<ListenerPtr> ListenerVector;
    typedef std::tr1::unordered_map<ListenerPtr,size_t> ListenerMap;
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

}
