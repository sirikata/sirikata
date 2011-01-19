
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>
#include "../JSContextStruct.hpp"

namespace Sirikata{
namespace JS{

class JSPresenceStruct;

const char* ToCString(const v8::String::Utf8Value& value);
JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args);
JSContextStruct* getContStructFromArgs(const v8::Arguments& args);

bool decodeBool(v8::Handle<v8::Value> toDeocde, bool& decodedValue, std::string& errorMessage);



}}//end namespaces

#endif
