/*  Sirikata Utilities -- Sirikata Utilities
 *  Any.hpp
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
#ifndef _ANY_HPP_
#define _ANY_HPP_
#include <typeinfo>
namespace Sirikata {
class Any {
    class Holder{
        friend class Any;
    protected:
        virtual ~Holder(){};

        virtual Holder * clone()const=0;
        virtual const std::type_info& typeOf()const=0;
    };
    template <class T> class SubHolder:public Holder{
        friend class Any;
        T mValue;
    public:
        SubHolder(const T&val):mValue(val) {}
        virtual ~SubHolder(){}
        virtual const std::type_info&typeOf()const {
            return typeid(T);
        }
        virtual Holder *clone()const {
            return new SubHolder<T>(mValue);
        }
    };
    Holder * mHolder;
public:
    Any() {
        mHolder=NULL;
    }
    template <class T> Any&operator =(const T&other) {
        delete mHolder;
        mHolder = new SubHolder<T>(other);
        return *this;
    }
    Any&operator =(const Any&other) {
        mHolder=other.mHolder?other.mHolder->clone():NULL;
        return *this;
    }
    Any(const Any&other) {
        mHolder=other.mHolder?other.mHolder->clone():NULL;
    }
    Any(const Any*other) {
        mHolder=other?(other->mHolder?other->mHolder->clone():NULL):NULL;
    }
    Any(Any*other) {
        mHolder=other?(other->mHolder?other->mHolder->clone():NULL):NULL;
    }
    template <class T> Any(const T&other) {
        mHolder = new SubHolder<T>(other);
    }
    ~Any() {
        delete mHolder;
    }
    bool empty() {
        return mHolder==NULL;
    }
    /**
     * If some other item has ownership of the value
     * And this item must be reset without removing the other.
     * Used in Option.hpp to allow for threadsafe reads
     */
    Any& newAndDoNotFree(const Any&value){
        mHolder=value.mHolder?value.mHolder->clone():NULL;
        return *this;
    }
    const std::type_info&typeOf() const {
        return mHolder?mHolder->typeOf():typeid(void);
    }
    template <class T> T& as () {
        return dynamic_cast<SubHolder<T>*>(mHolder)->mValue;
    }
    template <class T> T& unsafeAs () {
        return static_cast<SubHolder<T>*>(mHolder)->mValue;
    }
    template <class T> const T& as () const{
        return dynamic_cast<const SubHolder<T>*>(mHolder)->mValue;
    }
    template <class T> const T& unsafeAs () const{
        return static_cast<const SubHolder<T>*>(mHolder)->mValue;
    }
};
}
#endif
