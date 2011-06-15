#ifndef __SIRIKATA_JS_TIMER_HPP__
#define __SIRIKATA_JS_TIMER_HPP__


#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSTimer{

v8::Handle<v8::Value> resetTimer(const v8::Arguments& args);
v8::Handle<v8::Value> clear(const v8::Arguments& args);
v8::Handle<v8::Value> suspend(const v8::Arguments& args);
v8::Handle<v8::Value> resume(const v8::Arguments& args);
v8::Handle<v8::Value> isSuspended(const v8::Arguments& args);
v8::Handle<v8::Value> getAllData(const v8::Arguments& args);
v8::Handle<v8::Value> getType(const v8::Arguments& args);

void setNullTimer(const v8::Arguments& args);



#define INLINE_DECODE_TIMER_ERROR(toConvert,whereError,whereWriteTo)            \
    JSTimerStruct* whereWriteTo = NULL;                                         \
    {                                                                           \
        String _errMsg = "In " #whereError " of timer.  Cannot complete because likely already cleared this timer."; \
        whereWriteTo = JSTimerStruct::decodeTimerStruct(toConvert,_errMsg);   \
        if (whereWriteTo == NULL)                                               \
        {              \
            return v8::ThrowException(v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length()))); \
        } \
    }



} //jstimer
} //js
} //sirikata

#endif
