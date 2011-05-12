/*  Sirikata
 *  JSVec3.cpp
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

#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include "../JSSystemNames.hpp"

using namespace v8;

namespace Sirikata {
namespace JS {


static Persistent<FunctionTemplate> Vec3ConstructorTemplate;

Handle<Value> CreateJSResult_Vec3Impl(v8::Handle<v8::Context>& ctx, const Vector3d& src)
{
    HandleScope handle_scope;
    Handle<Function> vec3_constructor = FunctionCast(
        ObjectCast(GetGlobal(ctx, JSSystemNames::UTIL_OBJECT_NAME))->Get(JS_STRING(Vec3))
    );

    Handle<Object> result = vec3_constructor->NewInstance();
    Vec3Fill(result, src);
    return result;


}
Handle<Value> CreateJSResult_Vec3Impl(v8::Handle<v8::Function>& vec3_constructor, const Vector3d& src)
{
    HandleScope handle_scope;
    Handle<Object> result = vec3_constructor->NewInstance();
    Vec3Fill(result, src);
    return result;
}

bool Vec3ValValidate(v8::Handle<v8::Value> src)
{
    if (!src->IsObject())
        return false;

    v8::Handle<v8::Object> toValidate= src->ToObject();
    return Vec3Validate(toValidate);
}

//Note: user takes responsibility that src is actually a vec object.
Vector3d Vec3ValExtract(v8::Handle<v8::Value> src)
{
    v8::Handle<v8::Object>toExtract = src->ToObject();
    return Vec3Extract(toExtract);
}

Vector3f Vec3ValExtractF(v8::Handle<v8::Value> src)
{
    v8::Handle<v8::Object> toExtract = src->ToObject();
    return Vec3ExtractF(toExtract);
}

Vector3f Vec3ExtractF(v8::Handle<v8::Object> src)
{
    Vector3f result;
    result.x = NumericExtract( src->Get(JS_STRING(x)) );
    result.y = NumericExtract( src->Get(JS_STRING(y)) );
    result.z = NumericExtract( src->Get(JS_STRING(z)) );
    return result;
}



bool Vec3Validate(Handle<Object>& src) {
    return (
        src->Has(JS_STRING(x)) && NumericValidate(src->Get(JS_STRING(x))) &&
        src->Has(JS_STRING(y)) && NumericValidate(src->Get(JS_STRING(y))) &&
        src->Has(JS_STRING(z)) && NumericValidate(src->Get(JS_STRING(z)))
    );
}

Vector3d Vec3Extract(Handle<Object>& src) {
    Vector3d result;
    result.x = NumericExtract( src->Get(JS_STRING(x)) );
    result.y = NumericExtract( src->Get(JS_STRING(y)) );
    result.z = NumericExtract( src->Get(JS_STRING(z)) );
    return result;
}

// And this is the real implementation

Handle<Value> Vec3Constructor(const Arguments& args) {
    Handle<Object> self = args.This();

    if (args.Length() == 0) {
        Vec3Fill(self, Vector3d(0.0, 0.0, 0.0));
    }
    else if (args.Length() == 1) {
        NumericCheckAndExtract(v, args[0]);
        Vec3Fill(self, Vector3d(v, v, v));
    }
    else if (args.Length() == 3) {
        NumericCheckAndExtract(x, args[0]);
        NumericCheckAndExtract(y, args[1]);
        NumericCheckAndExtract(z, args[2]);
        Vec3Fill(self, Vector3d(x, y, z));
    }
    else {
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameter list for Vec3 constructor.")) );
    }

    return self;
}

Handle<Value> Vec3ToString(const Arguments& args) {
    Handle<Object> self = args.This();
    Vec3CheckAndExtract(self_val, self);
    String self_val_str = self_val.toString();
    return v8::String::New( self_val_str.c_str(), self_val_str.size() );
}

Handle<FunctionTemplate> CreateVec3Template() {
    // Vec3 Constructor
    Local<FunctionTemplate> vec3_constructor_templ = FunctionTemplate::New(Vec3Constructor);
    Vec3ConstructorTemplate = Persistent<FunctionTemplate>::New(vec3_constructor_templ);

    // Vec3 prototype
    Local<ObjectTemplate> vec3_prototype_templ = vec3_constructor_templ->PrototypeTemplate();
    vec3_prototype_templ->Set(JS_STRING(toString), v8::FunctionTemplate::New(Vec3ToString));

    // Vec3 instance
    //Local<ObjectTemplate> vec3_instance_templ = vec3_constructor_templ->InstanceTemplate();

    return vec3_constructor_templ;
}

void DestroyVec3Template() {
    Vec3ConstructorTemplate.Clear();
}

} // namespace JS
} // namespace Sirikata
