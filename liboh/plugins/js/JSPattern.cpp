/*  Sirikata
 *  JSPattern.cpp
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

#include "JSPattern.hpp"
#include "JSUtil.hpp"
#include <iostream>
#include <iomanip>
#include "JSObjects/JSObjectsUtils.hpp"

using namespace v8;

namespace Sirikata {
namespace JS {


// Pattern::Pattern(const std::string& _name,v8::Handle<v8::Value> _value,v8::Handle<v8::Value> _proto)
//  :mName(_name), mValue(_value), mPrototype(_proto)
Pattern::Pattern(const std::string& _name,v8::Handle<v8::Value> _value,v8::Handle<v8::Value> _proto)
 :mName(_name), mValue(v8::Persistent<v8::Value>::New(_value)), mPrototype(v8::Persistent<v8::Value>::New(_proto))
{
}


v8::Handle<v8::Value> Pattern::getAllData( )
{
    v8::HandleScope handle_scope;

    v8::Handle<v8::Object> returner = v8::Object::New(); 

    bool hasName = (name() == "") ? false:true;
    returner->Set(v8::String::New("hasName"), v8::Boolean::New( hasName));
    returner->Set(v8::String::New("name"), v8::String::New(name().c_str()));
    returner->Set(v8::String::New("hasValue"), v8::Boolean::New(hasValue()));
    if (hasValue())
        returner->Set(v8::String::New("value"),value());

    returner->Set(v8::String::New("hasPrototype"), v8::Boolean::New(hasPrototype()));
    if (hasPrototype())
        returner->Set(v8::String::New("prototype"),prototype());
    
    return handle_scope.Close(returner);
}


bool Pattern::matches(v8::Handle<v8::Object> obj) const
{
    if (mName == "")
        return true;
    
    if (!obj->Has(v8::String::New(mName.c_str())))
    {
        return false;
    }
    
    if (hasValue())
    {
        Handle<Value> field = obj->Get(v8::String::New(mName.c_str()));

        if (!field->Equals(mValue))
            return false;
    }

    if (hasPrototype()) {
        // FIXME check prototype
    }

    return true;
}


//prints a pattern
void Pattern::printPattern() const
{
    //name
    std::cout<<"Name: "<<mName.c_str()<<"\n";

    if (hasValue())
    {
        //std::cout<<" Having hard time with value\n";
        v8::String::Utf8Value stringValue(mValue);
        const char* strval = ToCString(stringValue);
        std::string stringVal (strval);
        std::cout<<"  Value: "<<stringVal<<"\n";
    }
    else
        std::cout<<"  Value: ANY\n";

    //FIXME: Prototype still needs to be printed.
}





static Persistent<FunctionTemplate> PatternConstructorTemplate;

// These utility methods are used all over to translate to/from C++/JS patterns

void PatternFill(Handle<Object>& dest, const Pattern& src) {
    dest->Set(JS_STRING(name), v8::String::New(src.name().c_str(), src.name().size()));

    if (src.hasValue())
        dest->Set(JS_STRING(value), src.value());
    if (src.hasPrototype())
        dest->Set(JS_STRING(proto), src.prototype());
}

// These generate a result, possibly using the first parameter as a prototype if
// the result type is Pattern.
Handle<Value> CreateJSResult(Handle<Object>& orig, const Pattern& src) {
    Handle<Object> result = orig->Clone();
    PatternFill(result, src);
    return result;
}

Handle<Value> CreateJSResult(v8::Handle<v8::Context>& ctx, const Pattern& src) {
    Handle<Function> pattern_constructor = FunctionCast(
        ObjectCast(GetGlobal(ctx, "system"))->Get(JS_STRING(Pattern))
    );

    Handle<Object> result = pattern_constructor->NewInstance();
    PatternFill(result, src);
    return result;
}

bool PatternValidate(Handle<Value>& src) {
    if (!src->IsObject())
        return false;
    Handle<Object> src_obj = src->ToObject();
    return PatternValidate(src_obj);
}

bool PatternValidate(Handle<Object>& src_obj)
{
    if (!src_obj->Has(JS_STRING(name)) || !StringValidate(src_obj->Get(JS_STRING(name))))
        return false;
    
    return true;
}

Pattern PatternExtract(Handle<Value>& src) {
    Handle<Object> src_obj = src->ToObject();
    return PatternExtract(src_obj);
}
Pattern PatternExtract(Handle<Object>& src_obj) {
    std::string   name  = StringExtract( src_obj->Get(JS_STRING(name)) );
    Handle<Value> val   = src_obj->Has(JS_STRING(value)) ? src_obj->Get(JS_STRING(value)) : Handle<Value>();
    Handle<Value> proto = src_obj->Has(JS_STRING(proto)) ? src_obj->Get(JS_STRING(proto)) : Handle<Value>();

    return Pattern(name, val, proto);
}

v8::Handle<v8::Value>getAllData(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  No args should be passed to pattern's getAllData.")));


    v8::Handle<v8::Value> mThis = args.This();
    if (! PatternValidate(mThis))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error decoding pattern in getAllData")));

    Pattern mPattern = PatternExtract(mThis);
    
    return mPattern.getAllData();
        
}


// And this is the real implementation
Handle<Value> PatternConstructor(const Arguments& args) {
    Handle<Object> self = args.This();

    String name = "";
    if (args.Length() > 0)
    {
        //check that the first argument is a string
        String errorMessage  = "Error decoding first arg string of pattern.  ";
        bool decodeSuccessful =  decodeString(args[0],name,errorMessage);
        if (! decodeSuccessful)
            return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())));
    }
    

    Handle<Value> val   =  (args.Length() > 1) ? args[1] : Handle<Value>();
    Handle<Value> proto =  (args.Length() > 2) ? args[2] : Handle<Value>();

    
    PatternFill(self, Pattern(name, val, proto));

    return v8::Undefined();
}

Handle<Value> PatternToString(const Arguments& args) {
    Handle<Object> self = args.This();
    PatternCheckAndExtract(self_val, self);
    String self_val_str = self_val.toString();
    return v8::String::New( self_val_str.c_str(), self_val_str.size() );
}

Handle<FunctionTemplate> CreatePatternTemplate() {
    // Pattern Constructor
    Local<FunctionTemplate> pattern_constructor_templ = FunctionTemplate::New(PatternConstructor);
    PatternConstructorTemplate = Persistent<FunctionTemplate>::New(pattern_constructor_templ);

    // Pattern prototype
    Local<ObjectTemplate> pattern_prototype_templ = pattern_constructor_templ->PrototypeTemplate();

    pattern_prototype_templ->Set(JS_STRING(toString), v8::FunctionTemplate::New(PatternToString));
    pattern_prototype_templ->Set(JS_STRING(getAllData), v8::FunctionTemplate::New(getAllData));
    
    // Pattern instance
    Local<ObjectTemplate> pattern_instance_templ = pattern_constructor_templ->InstanceTemplate();

    return pattern_constructor_templ;
}

void DestroyPatternTemplate() {
    PatternConstructorTemplate.Clear();
}



} // namespace JS
} // namespace Sirikata
