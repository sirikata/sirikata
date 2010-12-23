
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>


namespace Sirikata{
namespace JS{

class JSPresenceStruct;

const char* ToCString(const v8::String::Utf8Value& value);
JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args);
}}//end namespaces

#endif
