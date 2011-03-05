#ifndef __SIRIKATA_JS_SERIALIZE_HPP__
#define __SIRIKATA_JS_SERIALIZE_HPP__

#include <sirikata/oh/Platform.hpp>

#include <string>
#include "JS_JSMessage.pbj.hpp"
#include "JSObjectScript.hpp"


#include <v8.h>

namespace Sirikata {
namespace JS {


class JSSerializer
{
    static void serializeFunction(v8::Local<v8::Function> v8Func, Sirikata::JS::Protocol::JSMessage&);
public:
    static std::string serializeObject(v8::Local<v8::Value> v8Val);;
    static void serializeObjectInternal(v8::Local<v8::Value> v8Val, Sirikata::JS::Protocol::IJSMessage&);
    static bool deserializeObject(JSObjectScript*, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo);
    static void serializeInternalFields(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&);
    static void serializeAddressable(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&);
    static void serializeVisible(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&);
};

}}//end namespaces

#endif
