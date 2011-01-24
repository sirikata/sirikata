#ifndef __SIRIKATA_JS_TIMER_HPP__
#define _SIRIKATA_JS_TIMER_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSTimer{

v8::Handle<v8::Value> resetTimer(const v8::Arguments& args);
v8::Handle<v8::Value> clear(const v8::Arguments& args);

} //jstimer
} //js
} //sirikata

#endif
