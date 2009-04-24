/*  Meru
 *  Factory.hpp
 *
 *  Copyright (c) 2009, Stanford University
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
#ifndef _FACTORY_HPP_
#define _FACTORY_HPP_

#include "MeruDefs.hpp"
#include <map>

namespace Meru {

template<class T, class TRaw>
class FactoryBase {
public:

    typedef typename std::tr1::function<T()> CreateFunctionPtr;

    FactoryBase()
     : mDefaultType("")
    {
    }

    ~FactoryBase() {
    }

    bool registerType(const String& type, CreateFunctionPtr cfptr, bool defaultType = false) {
        CreateFunctionMapIterator it = mCreateFunctionMap.find(type);
        if (it != mCreateFunctionMap.end())
            return false;
        mCreateFunctionMap[type] = cfptr;
        if (defaultType)
            mDefaultType = type;
        return true;
    }

    T createInstance(const String& type) {
        CreateFunctionMapIterator it = mCreateFunctionMap.find(type);
        if (it == mCreateFunctionMap.end())
            return T((TRaw)NULL);
        return ((*it).second)();
    }

    T createInstance() {
        return createInstance(mDefaultType);
    }

    void destroyInstance(T inst) {
        delete inst;
    }

private:
    typedef typename std::map<String, CreateFunctionPtr> CreateFunctionMap;
    typedef typename CreateFunctionMap::iterator CreateFunctionMapIterator;

    CreateFunctionMap mCreateFunctionMap;
    String mDefaultType;
};


template<class T> class Factory : public FactoryBase<T*, T*> {
};

template<class T> class SharedPtrFactory : public FactoryBase<std::tr1::shared_ptr<T>, T*> {
};



/** Factory that generates objects of type T, which represent underlying TRaw pointers,
 *  with parameters of type U.
 */
template <class T, class TRaw, class U> class FactoryBase1 {
public:

    typedef typename std::tr1::function<T(U)> CreateFunctionPtr;

    FactoryBase1()
     : mDefaultType("")
    {
    }

    ~FactoryBase1() {
    }

    bool registerType(const String& type, CreateFunctionPtr cfptr, bool defaultType = false) {
        CreateFunctionMapIterator it = mCreateFunctionMap.find(type);
        if (it != mCreateFunctionMap.end())
            return false;
        mCreateFunctionMap[type] = cfptr;
        if (defaultType)
            mDefaultType = type;
        return true;
    }

    T createInstance(const String& type, U argument) {
        CreateFunctionMapIterator it = mCreateFunctionMap.find(type);
        if (it == mCreateFunctionMap.end())
            return T((TRaw)NULL);
        return ((*it).second)(argument);
    }

    T createInstance(U argument) {
        return createInstance(mDefaultType,argument);
    }

    void destroyInstance(T inst) {
        delete inst;
    }
    CreateFunctionPtr createInstanceFactory(const String &type) {
        CreateFunctionMapIterator it = mCreateFunctionMap.find(type);
        if (it == mCreateFunctionMap.end()) {
            String instanceunavail="Factory Instance unavailable: "+type;
            char * teststr=strdup(type.c_str());
            throw std::runtime_error(teststr);
        }
        return ((*it).second);
    }
    CreateFunctionPtr createInstanceFactory() {
        return createInstanceFactory(mDefaultType);
    }

private:
    typedef typename std::map<String, CreateFunctionPtr> CreateFunctionMap;
    typedef typename CreateFunctionMap::iterator CreateFunctionMapIterator;

    CreateFunctionMap mCreateFunctionMap;
    String mDefaultType;
};


template<class T, class U> class Factory1 : public FactoryBase1<T*, T*, U> {
};

template<class T, class U> class SharedPtrFactory1 : public FactoryBase1<std::tr1::shared_ptr<T>, T*, U> {
};


} // namespace Meru


#endif //_FACTORY_HPP_
