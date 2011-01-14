#ifndef __SIRIKATA_JS_JSCONTEXT_HPP__
#define _SIRIKATA_JS_JSCONTEXT_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSContext{


v8::Handle<v8::Value> ScriptTimeout(const v8::Arguments& args);
v8::Handle<v8::Value> Print(const v8::Arguments& args);
v8::Handle<v8::Value> ScriptExecute(const v8::Arguments& args);


template<typename WithHolderType>
JSObjectScript* GetTargetJSObjectScript(const WithHolderType& with_holder);


}//jscontext
}//js
}//sirikata

#endif
