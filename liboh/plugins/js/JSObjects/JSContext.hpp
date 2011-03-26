#ifndef __SIRIKATA_JS_JSCONTEXT_HPP__
#define __SIRIKATA_JS_JSCONTEXT_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSContext{

v8::Handle<v8::Value> ScriptExecute(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptSuspend(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptResume(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptClear(const v8::Arguments& args);

}//jscontext
}//js
}//sirikata

#endif
