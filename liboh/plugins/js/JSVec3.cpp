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
#include "JSUtil.hpp"

using namespace v8;

namespace Sirikata {
namespace JS {


static Persistent<FunctionTemplate> Vec3ConstructorTemplate;

bool Vec3Validate(Handle<Object>& src) {
    return (
        src->Has(JS_STRING(x)) && ValidateNumericValue(src->Get(JS_STRING(x))) &&
        src->Has(JS_STRING(y)) && ValidateNumericValue(src->Get(JS_STRING(y))) &&
        src->Has(JS_STRING(z)) && ValidateNumericValue(src->Get(JS_STRING(z)))
    );
}

Vector3d Vec3Extract(Handle<Object>& src) {
    Vector3d result;
    result.x = GetNumericValue( src->Get(JS_STRING(x)) );
    result.y = GetNumericValue( src->Get(JS_STRING(y)) );
    result.z = GetNumericValue( src->Get(JS_STRING(z)) );
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

#define DefineVec3UnaryOperator(cname, name, op)      \
    Handle<Value> cname(const Arguments& args) {        \
        Handle<Object> self = args.This();              \
                                                        \
        if (args.Length() != 0)                                         \
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Vec3." #name " should take zero parameters.")) ); \
                                                                        \
        Vec3CheckAndExtract(lhs, self);                                 \
        Handle<Value> result = Vec3CloneAndFill(self, ((lhs).*op)());  \
                                                                        \
        return result;                                                  \
    }

#define DefineVec3BinaryOperator(cname, name, op)       \
    Handle<Value> cname(const Arguments& args) {      \
        Handle<Object> self = args.This();              \
                                                        \
        if (args.Length() != 1)                                         \
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Vec3." #name " should take one parameter.")) ); \
                                                                        \
        Handle<Object> rhs_obj = Handle<Object>::Cast(args[0]);         \
                                                                        \
        Vec3CheckAndExtract(lhs, self);                                 \
        Vec3CheckAndExtract(rhs, rhs_obj);                              \
        Handle<Value> result = Vec3CloneAndFill(self, ((lhs).*op)(rhs)); \
                                                                        \
        return result;                                                  \
    }


DefineVec3BinaryOperator(Vec3Add, add, &Vector3d::operator+);
static Vector3d(Vector3d::*subtract_op)(const Vector3d&other) const = &Vector3d::operator-;
DefineVec3BinaryOperator(Vec3Sub, sub, subtract_op);
DefineVec3BinaryOperator(Vec3Cross, cross, &Vector3d::cross);
DefineVec3UnaryOperator(Vec3Normal, normal, &Vector3d::normal);
DefineVec3BinaryOperator(Vec3Reflect, reflect, &Vector3d::reflect);
DefineVec3BinaryOperator(Vec3Max, max, &Vector3d::max);
DefineVec3BinaryOperator(Vec3Min, min, &Vector3d::min);
DefineVec3BinaryOperator(Vec3ComponentMultiply, componentMultiply, &Vector3d::componentMultiply);

DefineVec3BinaryOperator(Vec3Dot, dot, &Vector3d::dot);
DefineVec3UnaryOperator(Vec3Length, length, &Vector3d::length);
DefineVec3UnaryOperator(Vec3LengthSquared, lengthSquared, &Vector3d::lengthSquared);

static Vector3d(Vector3d::*neg_op)() const = &Vector3d::operator-;
DefineVec3UnaryOperator(Vec3Neg, neg, neg_op);


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
    vec3_prototype_templ->Set(JS_STRING(add), v8::FunctionTemplate::New(Vec3Add));
    vec3_prototype_templ->Set(JS_STRING(sub), v8::FunctionTemplate::New(Vec3Sub));
    vec3_prototype_templ->Set(JS_STRING(neg), v8::FunctionTemplate::New(Vec3Neg));
    vec3_prototype_templ->Set(JS_STRING(cross), v8::FunctionTemplate::New(Vec3Cross));
    vec3_prototype_templ->Set(JS_STRING(normal), v8::FunctionTemplate::New(Vec3Normal));
    vec3_prototype_templ->Set(JS_STRING(reflect), v8::FunctionTemplate::New(Vec3Reflect));
    vec3_prototype_templ->Set(JS_STRING(max), v8::FunctionTemplate::New(Vec3Max));
    vec3_prototype_templ->Set(JS_STRING(min), v8::FunctionTemplate::New(Vec3Min));
    vec3_prototype_templ->Set(JS_STRING(componentMul), v8::FunctionTemplate::New(Vec3ComponentMultiply));

    vec3_prototype_templ->Set(JS_STRING(dot), v8::FunctionTemplate::New(Vec3Dot));
    vec3_prototype_templ->Set(JS_STRING(length), v8::FunctionTemplate::New(Vec3Length));
    vec3_prototype_templ->Set(JS_STRING(lengthSquared), v8::FunctionTemplate::New(Vec3LengthSquared));

    vec3_prototype_templ->Set(JS_STRING(toString), v8::FunctionTemplate::New(Vec3ToString));

    // Vec3 instance
    Local<ObjectTemplate> vec3_instance_templ = vec3_constructor_templ->InstanceTemplate();

    return vec3_constructor_templ;
}

void DestroyVec3Template() {
    Vec3ConstructorTemplate.Clear();
}

} // namespace JS
} // namespace Sirikata
