/*  Sirikata
 *  JSUtil.hpp
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

#ifndef _SIRIKATA_JS_UTIL_HPP_
#define _SIRIKATA_JS_UTIL_HPP_

#include <oh/Platform.hpp>
#include <v8.h>

using namespace v8;

namespace Sirikata {
namespace JS {

#define JS_STRING(x) (v8::String::New( #x ))

bool NumericValidate(const Handle<Value>& val);
double NumericExtract(const Handle<Value>& val);
Handle<Value> CreateJSResult(Handle<Object>& orig, const double& src);
Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const double& src);

Handle<Value> CreateJSResult(Handle<Object>& orig, const float& src);
Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const float& src);

#define NumericCheckAndExtract(native, value)                           \
    if (!NumericValidate(value))                                        \
        return v8::ThrowException( v8::Exception::TypeError(v8::String::New("Value couldn't be interpreted as numeric.")) ); \
    double native = NumericExtract(value);


v8::Handle<v8::Object> ObjectCast(const v8::Handle<v8::Value>& v);

#define ObjectCheckAndCast(result, value)       \
    if (!value->IsObject())                                             \
        return v8::ThrowException( v8::Exception::TypeError(v8::String::New("Expected object.")) ); \
    Handle<Object> result = Handle<Object>::Cast(value);


v8::Handle<v8::Function> FunctionCast(const v8::Handle<v8::Value>& v);

#define FunctionCheckAndCast(result, value)                             \
    if (!value->IsFunction())                                           \
        return v8::ThrowException( v8::Exception::TypeError(v8::String::New("Expected function.")) ); \
    Handle<Function> result = Handle<Function>::Cast(value);


v8::Handle<v8::Value> GetGlobal(v8::Handle<v8::Context>& ctx, const char* obj_name);



} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_UTIL_HPP_
