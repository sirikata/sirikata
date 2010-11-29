/*  Sirikata
 *  JSPattern.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_JS_PATTERN_HPP_
#define _SIRIKATA_JS_PATTERN_HPP_

#include "JSUtil.hpp"
#include <v8.h>
#include <iostream>
#include <iomanip>

namespace Sirikata {
namespace JS {

/** Patterns are single rules for matching an object. A field name
 *  must be specified, and either or both of a prototype and value may
 *  be specified.
 */
class Pattern {
public:
    Pattern(const std::string& _name,v8::Handle<v8::Value> _value = v8::Handle<v8::Value>(),v8::Handle<v8::Value> _proto = v8::Handle<v8::Value>());
    

    std::string name() const { return mName; }

    bool hasValue() const { return !mValue.IsEmpty(); }
    v8::Handle<v8::Value> value() const { assert(hasValue()); return mValue; }

    bool hasPrototype() const { return !mPrototype.IsEmpty(); }
    v8::Handle<v8::Value> prototype() const { assert(hasPrototype()); return mPrototype; }

    String toString() const { return "[Pattern]"; }

    bool matches(v8::Handle<v8::Object> obj) const;
    void printPattern() const;
    
private:
    std::string mName;
    v8::Persistent<v8::Value> mValue;
    v8::Persistent<v8::Value> mPrototype;
};

typedef std::vector<Pattern> PatternList;

/** Create a template for a Pattern function. */
v8::Handle<v8::FunctionTemplate> CreatePatternTemplate();
void DestroyPatternTemplate();

void PatternFill(Handle<Object>& dest, const Pattern& src);
Handle<Value> CreateJSResult(Handle<Object>& orig, const Pattern& src);
Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const Pattern& src);
bool PatternValidate(Handle<Value>& src);
bool PatternValidate(Handle<Object>& src);
Pattern PatternExtract(Handle<Value>& src);
Pattern PatternExtract(Handle<Object>& src);

#define PatternCheckAndExtract(native, value)                              \
    if (!PatternValidate(value))                                           \
        return v8::ThrowException( v8::Exception::TypeError(v8::String::New("Value couldn't be interpreted as Pattern.")) ); \
    Pattern native = PatternExtract(value);

} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_PATTERN_HPP_
