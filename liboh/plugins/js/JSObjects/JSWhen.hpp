#ifndef __SIRIKATA_JS_WHENER_HPP__
#define __SIRIKATA_JS_WHENER_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSWhen{


v8::Handle<v8::Value> WhenSuspend(const v8::Arguments& args);
v8::Handle<v8::Value> WhenResume(const v8::Arguments& args);
v8::Handle<v8::Value> WhenGetLastPredState(const v8::Arguments& args);



} //jswhen
} //js
} //sirikata

#endif
