
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>
#include <sirikata/core/util/Platform.hpp>

namespace Sirikata{
namespace JS{

bool decodeString(v8::Handle<v8::Value> toDecode, String& decodedValue, String& errorMessage);
bool decodeBool(v8::Handle<v8::Value> toDecode, bool& decodedValue, std::string& errorMessage);
void debug_checkCurrentContextX(v8::Handle<v8::Context> ctx, std::string additionalMessage);
void printAllPropertyNames(v8::Handle<v8::Object> objToPrint);


// class JSObjectScript;
// template<typename WithHolderType>
// JSObjectScript* GetTargetJSObjectScript(const WithHolderType& with_holder);



} //end namespace js
} //end namespace sirikata

#endif
