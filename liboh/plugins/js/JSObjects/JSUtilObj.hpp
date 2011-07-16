#ifndef __SIRIKATA_JS_UTILOBJ_HPP__
#define __SIRIKATA_JS_UTILOBJ_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSUtilObj{

v8::Handle<v8::Value> ScriptSqrtFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptAcosFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptAsinFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptSinFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptCosFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptRandFunction(const v8::Arguments& args);

v8::Handle<v8::Value> ScriptPowFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptExpFunction(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptAbsFunction(const v8::Arguments& args);

v8::Handle<v8::Value> ScriptPlus(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptMinus(const v8::Arguments& args);

v8::Handle<v8::Value> ScriptDiv(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptMult(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptMod(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptEqual(const v8::Arguments& args);

v8::Handle<v8::Value> ScriptSporef(const v8::Arguments& args);

v8::Handle<v8::Value> Base64Encode(const v8::Arguments& args);
v8::Handle<v8::Value> Base64EncodeURL(const v8::Arguments& args);
v8::Handle<v8::Value> Base64Decode(const v8::Arguments& args);
v8::Handle<v8::Value> Base64DecodeURL(const v8::Arguments& args);


} //jsutilobj
} //js
} //sirikata

#endif
