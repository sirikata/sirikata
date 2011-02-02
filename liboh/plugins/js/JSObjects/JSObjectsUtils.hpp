
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>
#include <sirikata/core/util/Platform.hpp>

namespace Sirikata{
namespace JS{

class JSWatchable;

bool decodeString(v8::Handle<v8::Value> toDecode, String& decodedValue, String& errorMessage);
bool decodeBool(v8::Handle<v8::Value> toDecode, bool& decodedValue, String& errorMessage);
void debug_checkCurrentContextX(v8::Handle<v8::Context> ctx, String additionalMessage);
void printAllPropertyNames(v8::Handle<v8::Object> objToPrint);
JSWatchable* decodeWatchable(v8::Handle<v8::Value> toDecode, String & errorMessage);




} //end namespace js
} //end namespace sirikata

#endif
