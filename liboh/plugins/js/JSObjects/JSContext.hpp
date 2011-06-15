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

void setNullContext(const v8::Arguments& args);


#define INLINE_DECODE_CONTEXT_ERROR(toConvert,whereError,whereWriteTo)            \
    JSContextStruct* whereWriteTo = NULL;                                         \
    {                                                                           \
        String _errMsg = "In " #whereError " of timer.  Cannot complete because likely already cleared this sandbox."; \
        whereWriteTo = JSContextStruct::decodeContextStruct(toConvert,_errMsg);   \
        if (whereWriteTo == NULL)                                               \
        {              \
            return v8::ThrowException(v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length()))); \
        } \
    }




}//jscontext
}//js
}//sirikata

#endif
