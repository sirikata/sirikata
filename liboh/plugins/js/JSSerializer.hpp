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
private:
	static std::string serializeFunction(v8::Handle<v8::Function> v8Func);
public:

    static std::string serializeObject(v8::Handle<v8::Value> v8Val);;
    static bool deserializeObject( std::string strDecode,v8::Local<v8::Object>& deserializeTo);
    static bool deserializeRegularObject(JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo);
    static bool deserializeObject(JSObjectScript*, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo);
    static void serializeInternalFields(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&);
    static void serializeVisible(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&);
    static bool deserializeVisible(JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo);
};

}}//end namespaces

#endif
