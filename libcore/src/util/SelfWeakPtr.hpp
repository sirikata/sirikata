/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  SelfWeakPtr.hpp
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



namespace Sirikata {
template <class T>
class SelfWeakPtr {
    std::tr1::weak_ptr<T> mWeakPtr;
protected:
    SelfWeakPtr() {}

    ~SelfWeakPtr() {}

    template <class U> static std::tr1::shared_ptr<U> internalConstruct(U * u){
        std::tr1::shared_ptr<U> retval(u);
        std::tr1::shared_ptr<T> sharedPtr(retval);
        sharedPtr->mWeakPtr=sharedPtr;
        return retval;
    }
    
public:
    const std::tr1::weak_ptr<T>& getWeakPtr() const {
        return mWeakPtr;
    }
    std::tr1::shared_ptr<T> getSharedPtr() const {
        std::tr1::shared_ptr<T> retval(getWeakPtr());
        return retval;
    }
    template <class U> static std::tr1::shared_ptr<U>construct(){
        return internalConstruct(new U());
    }
    template <class A> static std::tr1::shared_ptr<T>construct(A a){
        return internalConstruct(new T(a));
    }
    template <class A,class B> 
      static std::tr1::shared_ptr<T>construct(A a, B b){
        return internalConstruct(new T(a,b));
    }
    template <class A,class B,class C>
      static std::tr1::shared_ptr<T>construct(A a, B b, C c){
        return internalConstruct(new T(a,b,c));
    }
    template <class A,class B,class C,class D>
      static std::tr1::shared_ptr<T>construct(A a, B b, C c, D d){
        return internalConstruct(new T(a,b,c,d));
    }
    template <class A,class B,class C,class D,class E> 
      static std::tr1::shared_ptr<T>construct(A a,B b,C c,D d,E e){
        return internalConstruct(new T(a,b,c,d,e));
    }
    template <class U,class A,class B,class C,class D,class E, class F> 
      static std::tr1::shared_ptr<T>construct(A a,B b,C c,D d,E e, F f){
        return internalConstruct(new T(a,b,c,d,e,f));
    }
    // add more if you need to, keptain...
};

}
