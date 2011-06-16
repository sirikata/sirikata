/*  Sirikata
 *  JSUtil.cpp
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
#include <iostream>
#include <iomanip>
#include "JSUtil.hpp"
#include "JSLogging.hpp"

using namespace v8;

namespace Sirikata {
namespace JS {

bool StringValidate(const Handle<Value>& val) {
    return (val->IsString());
}

std::string StringExtract(const Handle<Value>& val) {
    v8::String::Utf8Value utf8val(val);
    return std::string(*utf8val, utf8val.length());
}

bool NumericValidate(const Handle<Value>& val) {
    return (val->IsUint32() || val->IsInt32() || val->IsNumber());
}

double NumericExtract(const Handle<Value>& val) {
    if (val->IsUint32()) {
        uint32 native_val = val->ToUint32()->Value();
        return native_val;
    }
    else if (val->IsInt32()) {
        int32 native_val = val->ToInt32()->Value();
        return native_val;
    }
    else if (val->IsNumber()) {
        double native_val = val->NumberValue();
        return native_val;
    }

    JSLOG(error, "Calling numeric extract on non-numeric.  Returning -1.");
    return -1;
}

//FIXME: may need to add a vec3 versioin of CreateJSResult so that can return
//positions and velocities

Handle<Value> CreateJSResult(Handle<Object>& orig, const double& src)
{
    HandleScope handle_scope;
    return Number::New(src);
}

Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const double& src)
{
    HandleScope handle_scope;
    return Number::New(src);
}

Handle<Value> CreateJSResult(Handle<Object>& orig, const float& src)
{
    HandleScope handle_scope;
    return Number::New(src);
}

Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const float& src)
{
    HandleScope handle_scope;
    return Number::New(src);
}

v8::Handle<v8::Object> ObjectCast(const v8::Handle<v8::Value>& v)
{
    HandleScope handle_scope;
    return v8::Handle<v8::Object>::Cast(v);
}

v8::Handle<v8::Function> FunctionCast(const v8::Handle<v8::Value>& v)
{
    HandleScope handle_scope;
    return v8::Handle<v8::Function>::Cast(v);
}



Handle<Value> GetGlobal(v8::Handle<v8::Context>& ctx, const char* obj_name)
{
    HandleScope handle_scope;
    return ctx->Global()->Get(v8::String::New(obj_name));
}

} // namespace JS
} // namespace Sirikata
