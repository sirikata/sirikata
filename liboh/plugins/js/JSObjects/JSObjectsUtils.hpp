
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>
#include "../JSObjectStructs/JSContextStruct.hpp"

namespace Sirikata{
namespace JS{

class JSPresenceStruct;

const char* ToCString(const v8::String::Utf8Value& value);
bool decodeBool(v8::Handle<v8::Value> toDecode, bool& decodedValue, std::string& errorMessage);


} //end namespace js
} //end namespace sirikata

#endif
