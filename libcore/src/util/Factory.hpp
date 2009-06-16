/*  Sirikata Utilities -- Sirikata Utilities
 *  Factory.hpp
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

#ifndef _SIRIKATA_FACTORY_HPP_
#define _SIRIKATA_FACTORY_HPP_

namespace Sirikata {
template<class T, class Ftype>
class FactoryImpl {

    typedef std::map<String,Ftype> ConstructorMap;
    ConstructorMap mConstructors;
    template <class U> static U* noop(U*) {
        return NULL;
    }
    template <class U> static U noop(const U&) {
        return U();
    }
    static T staticNoop() {
        T temp=T();
        return noop(temp);
    }
    Ftype mNoop;
public:
	FactoryImpl():mNoop(std::tr1::bind(&FactoryImpl<T,Ftype>::staticNoop)){}
    bool unregisterConstructor(const String& name, bool isDefault=false) {
        typename ConstructorMap::iterator where=mConstructors.find(name);
        if (where==mConstructors.end())
            return false;
        mConstructors.erase(where);
        if (isDefault) {
            where=mConstructors.find(String());
            if (where==mConstructors.end())
                return false;
            mConstructors.erase(where);
        }
        return true;
    }
    bool registerConstructor(const String& name,
                             const Ftype &constructor,
                             bool isDefault=false) {
        if (mConstructors.find(name)!=mConstructors.end())
            return false;
        mConstructors[name]=constructor;
        if (isDefault)
            mConstructors[String()]=constructor;
        return true;
    }
    const Ftype &getConstructor(const String&name)const{
        std::cout << "dbm: getConstructor" << std::endl;
        typename ConstructorMap::const_iterator where=mConstructors.find(name);
        if (where==mConstructors.end()) {
            std::cout << "dbm: getConstructor return 1" << std::endl;
            return mNoop;
        }
        std::cout << "dbm: getConstructor return 2:" << where->second << std::endl;
        return where->second;
    }
    const Ftype& getDefaultConstructor()const{
        return getConstructor(String());
    }
};

template <class T>class Factory:public FactoryImpl<T,std::tr1::function<T()> >{};

template <class T, typename A>class Factory1:public FactoryImpl<T,std::tr1::function<T(A)> >{};

template <class T, typename A, typename B>class Factory2:public FactoryImpl<T,std::tr1::function<T(A,B)> >{};

template <class T, typename A, typename B, typename C>class Factory3:public FactoryImpl<T,std::tr1::function<T(A,B,C)> >{};

template <class T, typename A, typename B, typename C, typename D>class Factory4:public FactoryImpl<T,std::tr1::function<T(A,B,C,D)> >{};

template <class T, typename A, typename B, typename C, typename D, typename E>class Factory5:public FactoryImpl<T,std::tr1::function<T(A,B,C,D,E)> >{};

template <class T, typename A, typename B, typename C, typename D, typename E, typename F>class Factory6:public FactoryImpl<T,std::tr1::function<T(A,B,C,D,E,F)> >{};

}
#endif
