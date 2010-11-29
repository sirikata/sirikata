
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>

namespace Sirikata{
namespace JS{

const char* ToCString(const v8::String::Utf8Value& value);




}}//end namespaces

#endif
