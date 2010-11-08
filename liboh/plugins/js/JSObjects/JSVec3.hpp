/*  Sirikata
 *  JSVec3.hpp
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

#ifndef _SIRIKATA_JS_VEC3_HPP_
#define _SIRIKATA_JS_VEC3_HPP_

#include "../JSUtil.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {

/** Create a template for a Vec3 function. */
v8::Handle<v8::FunctionTemplate> CreateVec3Template();
void DestroyVec3Template();



template<typename VecType>
void Vec3Fill(Handle<Object>& dest, const VecType& src) {
    dest->Set(JS_STRING(x), Number::New(src.x));
    dest->Set(JS_STRING(y), Number::New(src.y));
    dest->Set(JS_STRING(z), Number::New(src.z));
}

template<typename VecType>
Handle<Value> CreateJSResult(Handle<Object>& orig, const VecType& src) {
    Handle<Object> result = orig->Clone();
    Vec3Fill(result, src);
    return result;
}

Handle<Value> CreateJSResult_Vec3Impl(v8::Handle<v8::Context>& ctx, const Vector3d& src);
template<typename VecType>
Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const VecType& src) {
    return CreateJSResult_Vec3Impl(ctx, Vector3d(src));
}

bool Vec3Validate(Handle<Object>& src);
Vector3d Vec3Extract(Handle<Object>& src);

#define Vec3CheckAndExtract(native, value)                              \
    if (!Vec3Validate(value))                                           \
        return v8::ThrowException( v8::Exception::TypeError(v8::String::New("Value couldn't be interpreted as Vec3.")) ); \
    Vector3d native = Vec3Extract(value);


} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_VEC3_HPP_
