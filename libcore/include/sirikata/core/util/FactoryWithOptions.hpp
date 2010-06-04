/*  Sirikata Utilities -- Sirikata Utilities
 *  FactoryWithOptions.hpp
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

#ifndef _SIRIKATA_FACTORY_WITH_OPTIONS_HPP_
#define _SIRIKATA_FACTORY_WITH_OPTIONS_HPP_
#include "Factory.hpp"
namespace Sirikata {

template<class T, class Ftype>
class FactoryWithOptionsImpl :protected FactoryImpl<T,Ftype> {
    typedef std::tr1::function<OptionSet*(const String&)> Otype;
    typedef std::tr1::unordered_map<String, Otype> OptionMap;
    OptionMap mOptionParsers;
    Otype mNop;
    
    static OptionSet* staticNop() {
        return NULL;
    }

public:
	FactoryWithOptionsImpl():mNop(std::tr1::bind(&staticNop)){}
    bool unregisterConstructor(const String& name) {
        if (FactoryImpl<T,Ftype>::unregisterConstructor(name)) {
            typename OptionMap::iterator where=mOptionParsers.find(name);
            if (where==mOptionParsers.end())
                return false;
            mOptionParsers.erase(where);
        }
        return true;
    }
    bool hasConstructor(const String&name)const {
        return FactoryImpl<T,Ftype>::hasConstructor(name);
    }
    const String& getDefault()const {
        return FactoryImpl<T,Ftype>::getDefault();
    }
    bool registerConstructor(const String& name,
                             const Ftype &constructor,
                             const Otype &optionParser, bool defaultValue) {
        if (FactoryImpl<T,Ftype>::registerConstructor(name,constructor, defaultValue)) {
            mOptionParsers[name]=optionParser;
            return true;
        }
        return false;
    }
    const Otype &getOptionParser(const String&name)const {
        typename OptionMap::const_iterator where=mOptionParsers.find(name);
        if (where==mOptionParsers.end()) {
            if (name.length()==0&&getDefault().length()) {
                return getDefaultOptionParser();
            }
            return mNop;
        }

        return where->second;
        
    }
    const Otype& getDefaultOptionParser()const {
        return getOptionParser(getDefault());
    }
    const Ftype &getConstructor(const String&name)const {
        return FactoryImpl<T,Ftype>::getConstructor(name);
    }
    const Ftype &getDefaultConstructor()const {
        return FactoryImpl<T,Ftype>::getDefaultConstructor();
    }
    
};

template <class T>class FactoryWithOptions:public FactoryWithOptionsImpl<T,std::tr1::function<T()> >{};

template <class T, typename A>class FactoryWithOptions1:public FactoryWithOptionsImpl<T,std::tr1::function<T(A)> >{};

template <class T, typename A, typename B>class FactoryWithOptions2:public FactoryWithOptionsImpl<T,std::tr1::function<T(A,B)> >{};

template <class T, typename A, typename B, typename C>class FactoryWithOptions3:public FactoryWithOptionsImpl<T,std::tr1::function<T(A,B,C)> >{};

template <class T, typename A, typename B, typename C, typename D>class FactoryWithOptions4:public FactoryWithOptionsImpl<T,std::tr1::function<T(A,B,C,D)> >{};

template <class T, typename A, typename B, typename C, typename D, typename E>class FactoryWithOptions5:public FactoryWithOptionsImpl<T,std::tr1::function<T(A,B,C,D,E)> >{};

template <class T, typename A, typename B, typename C, typename D, typename E, typename F>class FactoryWithOptions6:public FactoryWithOptionsImpl<T,std::tr1::function<T(A,B,C,D,E,F)> >{};

}

#endif
