#ifndef __SIRIKATA_JS_MATH_HPP__
#define _SIRIKATA_JS_MATH_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSMath{


v8::Handle<v8::Value> ScriptSqrtFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptAcosFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptAsinFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptSinFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptCosFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptRandFunction(const v8::Arguments& args);

v8::Handle<v8::Value> ScriptPowFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptAbsFunction(const v8::Arguments& args);

} //jsmath
} //js
} //sirikata

#endif
