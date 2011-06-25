/*  Sirikata
 *  JSInvokableUtil.cpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_JS_INVOKABLE_UTIL_HPP_
#define _SIRIKATA_JS_INVOKABLE_UTIL_HPP_

#include "JSFunctionInvokable.hpp"
#include "JSInvokableObject.hpp"
#include <sirikata/proxyobject/Invokable.hpp>
#include "JSVisible.hpp"
#include "../EmersonScript.hpp"
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "JSPresence.hpp"
#include "../JSObjectStructs/JSPresenceStruct.hpp"

namespace Sirikata {
namespace JS {
namespace InvokableUtil {

/** Converts a V8 object into a boost:any, leaving the boost::any empty if the
 *  object cannot be translated.
 */
inline boost::any V8ToAny(EmersonScript* parent, v8::Handle<v8::Value> val) {
    /* Pushing only string params for now */
    if(val->IsString())
    {
      v8::String::AsciiValue str(val);
      std::string s = std::string(*str);
      return Invokable::asAny(s);
    }
    else if(val->IsFunction())
    {
      v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(val);
      v8::Persistent<v8::Function> function_persist = v8::Persistent<v8::Function>::New(function);

      JSFunctionInvokable* invokable = new JSFunctionInvokable(function_persist, parent);
      Sirikata::Invokable* in = invokable;
      return Invokable::asAny(in);
    }
    else if (val->IsBoolean()) {
        return Invokable::asAny(val->BooleanValue());
    }
    else if (val->IsNumber()) {
        return Invokable::asAny((float64)val->NumberValue());
    }
    else if (val->IsObject()) {
        // Handle special types
        if ( JSVisible::isVisibleObject(val) ) {
            std::string errmsg;
            JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(val,errmsg);
            return Invokable::asAny((jsvis->getSporef()));
        }
        else if ( JSPresence::isPresence(val) ) {
            std::string errmsg;
            JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(val,errmsg);
            return Invokable::asAny(jspres->getSporef());
        }

        // Otherwise do normal translation
        return boost::any(); // FIXME
    }
    return boost::any();
}


/** Converts a boost::any into a V8 object, or returns undefined if the object
 *  can't be translated.
 */
inline v8::Handle<v8::Value> AnyToV8(EmersonScript* parent, const boost::any& val) {
    if(Invokable::anyIsString(val)) {        std::string s = Invokable::anyAsString(val);
        return v8::String::New(s.c_str(), s.length());
    }
    else if(Invokable::anyIsFloat(val)) {
        double s = Invokable::anyAsFloat(val);
        return v8::Number::New(s);
    }
    else if(Invokable::anyIsDouble(val)) {
        double s = Invokable::anyAsDouble(val);
        return v8::Number::New(s);
    }
    else if (val.type() == typeid(uint8)) {
        uint32 d = boost::any_cast<uint8>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(uint16)) {
        uint32 d = boost::any_cast<uint16>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(uint32)) {
        uint32 d = boost::any_cast<uint32>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(uint64)) {
        uint32 d = boost::any_cast<uint64>(val);
        return v8::Uint32::New(d);
    }
    else if (val.type() == typeid(int8)) {
        int32 d = boost::any_cast<int8>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(int16)) {
        int32 d = boost::any_cast<int16>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(int32)) {
        int32 d = boost::any_cast<int32>(val);
        return v8::Int32::New(d);
    }
    else if (val.type() == typeid(int64)) {
        int32 d = boost::any_cast<int64>(val);
        return v8::Int32::New(d);
    }
    else if(Invokable::anyIsBoolean(val)) {
        bool s = Invokable::anyAsBoolean(val);
        return v8::Boolean::New(s);
    }
    else if(Invokable::anyIsArray(val)) {
        Sirikata::Invokable::Array native_arr = Invokable::anyAsArray(val);
        v8::Local<v8::Array> arr = v8::Array::New();
        for(uint32 ii = 0; ii < native_arr.size(); ii++) {
            v8::Handle<v8::Value> rhs = AnyToV8(parent, native_arr[ii]);
            if (!rhs.IsEmpty())
                arr->Set(ii, rhs);
        }
        return arr;
    }
    else if(Invokable::anyIsDict(val)) {
        Sirikata::Invokable::Dict native_dict = Invokable::anyAsDict(val);
        v8::Local<v8::Object> dict = v8::Object::New();
        for(Sirikata::Invokable::Dict::const_iterator di = native_dict.begin(); di != native_dict.end(); di++) {
            v8::Handle<v8::Value> rhs = AnyToV8(parent, di->second);
            if (!rhs.IsEmpty())
                dict->Set(v8::String::New(di->first.c_str(), di->first.size()), rhs);
        }
        return dict;
    }
    else if(Invokable::anyIsInvokable(val)) {
        if (!parent) return v8::Handle<v8::Value>();
        Invokable* newInvokableObj = Invokable::anyAsInvokable(val);
        Local<Object> tmpObj = parent->manager()->mInvokableObjectTemplate->NewInstance();
        Persistent<Object>tmpObjP = Persistent<Object>::New(tmpObj);
        tmpObjP->SetInternalField(JSINVOKABLE_OBJECT_JSOBJSCRIPT_FIELD,External::New(parent));
        tmpObjP->SetInternalField(JSINVOKABLE_OBJECT_SIMULATION_FIELD,External::New( new JSInvokableObject::JSInvokableObjectInt(newInvokableObj) ));
        tmpObjP->SetInternalField(TYPEID_FIELD,External::New(  new String (JSINVOKABLE_TYPEID_STRING)));
        return tmpObjP;
    }
    else if(Invokable::anyIsObject(val)) {
        SpaceObjectReference obj = Invokable::anyAsObject(val);
        if (obj == SpaceObjectReference::null()) return v8::Handle<v8::Value>();
        return parent->findVisible(obj);
    }

    return v8::Handle<v8::Value>();
}

} // namespace InvokableUtil
} // namespace JS
} // namespace Sirikata

#endif //_SIRIKATA_JS_INVOKABLE_UTIL_HPP_
