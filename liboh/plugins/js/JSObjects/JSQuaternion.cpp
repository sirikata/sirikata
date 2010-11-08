/*  Sirikata
 *  JSQuaternion.cpp
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

#include "JSQuaternion.hpp"
#include "../JSUtil.hpp"
#include "JSVec3.hpp"

using namespace v8;

namespace Sirikata {
namespace JS {


static Persistent<FunctionTemplate> QuaternionConstructorTemplate;

// These utility methods are used all over to translate to/from C++/JS quats

void QuaternionFill(Handle<Object>& dest, const Quaternion& src) {
    dest->Set(JS_STRING(x), Number::New(src.x));
    dest->Set(JS_STRING(y), Number::New(src.y));
    dest->Set(JS_STRING(z), Number::New(src.z));
    dest->Set(JS_STRING(w), Number::New(src.w));
}

// These generate a result, possibly using the first parameter as a prototype if
// the result type is Quaternion.
Handle<Value> CreateJSResult(Handle<Object>& orig, const Quaternion& src) {
    Handle<Object> result = orig->Clone();
    QuaternionFill(result, src);
    return result;
}

Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const Quaternion& src) {
    Handle<Function> quat_constructor = FunctionCast(
        ObjectCast(GetGlobal(ctx, "system"))->Get(JS_STRING(Quaternion))
    );

    Handle<Object> result = quat_constructor->NewInstance();
    QuaternionFill(result, src);
    return result;
}

bool QuaternionValidate(Handle<Object>& src) {
    return (
        src->Has(JS_STRING(x)) && NumericValidate(src->Get(JS_STRING(x))) &&
        src->Has(JS_STRING(y)) && NumericValidate(src->Get(JS_STRING(y))) &&
        src->Has(JS_STRING(z)) && NumericValidate(src->Get(JS_STRING(z))) &&
        src->Has(JS_STRING(w)) && NumericValidate(src->Get(JS_STRING(w)))
    );
}

Quaternion QuaternionExtract(Handle<Object>& src) {
    Quaternion result(
        NumericExtract( src->Get(JS_STRING(x)) ),
        NumericExtract( src->Get(JS_STRING(y)) ),
        NumericExtract( src->Get(JS_STRING(z)) ),
        NumericExtract( src->Get(JS_STRING(w)) ),
        Quaternion::XYZW()
    );
    return result;
}

// And this is the real implementation

Handle<Value> QuaternionConstructor(const Arguments& args) {
    Handle<Object> self = args.This();

    if (args.Length() == 0) {
        QuaternionFill(self, Quaternion());
    }
    else if (args.Length() == 2) {
        // Axis angle
        ObjectCheckAndCast(axis_obj, args[0]);
        Vec3CheckAndExtract(axis, axis_obj);
        NumericCheckAndExtract(angle, args[1]);
        QuaternionFill(self, Quaternion(Vector3f(axis), angle));
    }
    else if (args.Length() == 4) {
        NumericCheckAndExtract(x, args[0]);
        NumericCheckAndExtract(y, args[1]);
        NumericCheckAndExtract(z, args[2]);
        NumericCheckAndExtract(w, args[3]);
        QuaternionFill(self, Quaternion(x, y, z, w, Quaternion::XYZW()));
    }
    else {
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameter list for Quaternion constructor.")) );
    }

    return self;
}

#define DefineQuaternionUnaryOperator(cname, name, op)  \
    Handle<Value> cname(const Arguments& args) {        \
        Handle<Object> self = args.This();              \
                                                        \
        if (args.Length() != 0)                                         \
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Quaternion." #name " should take zero parameters.")) ); \
                                                                        \
        QuaternionCheckAndExtract(lhs, self);                           \
        Handle<Value> result = CreateJSResult(self, ((lhs).*op)()); \
                                                                        \
        return result;                                                  \
}

#define DefineQuaternionBinaryOperator(cname, name, op) \
    Handle<Value> cname(const Arguments& args) {        \
        Handle<Object> self = args.This();              \
                                                        \
        if (args.Length() != 1)                                         \
            return v8::ThrowException( v8::Exception::Error(v8::String::New("Quaternion." #name " should take one parameter.")) ); \
                                                                        \
        Handle<Object> rhs_obj = Handle<Object>::Cast(args[0]);         \
                                                                        \
        QuaternionCheckAndExtract(lhs, self);                           \
        QuaternionCheckAndExtract(rhs, rhs_obj);                        \
        Handle<Value> result = CreateJSResult(self, ((lhs).*op)(rhs)); \
                                                                        \
        return result;                                                  \
    }


DefineQuaternionBinaryOperator(QuaternionAdd, add, &Quaternion::operator+);
static Quaternion(Quaternion::*subtract_op)(const Quaternion&other) const = &Quaternion::operator-;
DefineQuaternionBinaryOperator(QuaternionSub, sub, subtract_op);
DefineQuaternionUnaryOperator(QuaternionNormal, normal, &Quaternion::normal);

static Quaternion(Quaternion::*neg_op)() const = &Quaternion::operator-;
DefineQuaternionUnaryOperator(QuaternionNeg, neg, neg_op);

DefineQuaternionUnaryOperator(QuaternionInv, inv, &Quaternion::inverse);


// We handle multiplication manually because rhs could be scalar, vector, or quaternion
Handle<Value> QuaternionMul(const Arguments& args) {
    Handle<Object> self = args.This();

    if (args.Length() != 1)                                             \
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Quaternion.mul should take one parameter.")) );

    QuaternionCheckAndExtract(lhs, self);

    Handle<Object> rhs_obj = Handle<Object>::Cast(args[0]);

    Handle<Value> result;

    if (NumericValidate(rhs_obj)) {
        double rhs = NumericExtract(rhs_obj);
        result = CreateJSResult(self, lhs * rhs);
    }
    else if (Vec3Validate(rhs_obj)) {
        Vector3d rhs = Vec3Extract(rhs_obj);
        result = CreateJSResult(rhs_obj, lhs * rhs);
    }
    else if (QuaternionValidate(rhs_obj)) {
        Quaternion rhs = QuaternionExtract(rhs_obj);
        result = CreateJSResult(rhs_obj, lhs * rhs);
    }
    else {
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Quaternion.mul parameter must be numeric, Vec3, or Quaternion.")) );
    }

    return result;
}

Handle<Value> QuaternionToString(const Arguments& args) {
    Handle<Object> self = args.This();
    QuaternionCheckAndExtract(self_val, self);
    String self_val_str = self_val.toString();
    return v8::String::New( self_val_str.c_str(), self_val_str.size() );
}

Handle<FunctionTemplate> CreateQuaternionTemplate() {
    // Quaternion Constructor
    Local<FunctionTemplate> quat_constructor_templ = FunctionTemplate::New(QuaternionConstructor);
    QuaternionConstructorTemplate = Persistent<FunctionTemplate>::New(quat_constructor_templ);

    // Quaternion prototype
    Local<ObjectTemplate> quat_prototype_templ = quat_constructor_templ->PrototypeTemplate();
    quat_prototype_templ->Set(JS_STRING(add), v8::FunctionTemplate::New(QuaternionAdd));
    quat_prototype_templ->Set(JS_STRING(sub), v8::FunctionTemplate::New(QuaternionSub));
    quat_prototype_templ->Set(JS_STRING(neg), v8::FunctionTemplate::New(QuaternionNeg));
    quat_prototype_templ->Set(JS_STRING(normal), v8::FunctionTemplate::New(QuaternionNormal));
    quat_prototype_templ->Set(JS_STRING(inv), v8::FunctionTemplate::New(QuaternionInv));

    quat_prototype_templ->Set(JS_STRING(mul), v8::FunctionTemplate::New(QuaternionMul));

    quat_prototype_templ->Set(JS_STRING(toString), v8::FunctionTemplate::New(QuaternionToString));

    // Quaternion instance
    Local<ObjectTemplate> quat_instance_templ = quat_constructor_templ->InstanceTemplate();

    return quat_constructor_templ;
}

void DestroyQuaternionTemplate() {
    QuaternionConstructorTemplate.Clear();
}

} // namespace JS
} // namespace Sirikata
