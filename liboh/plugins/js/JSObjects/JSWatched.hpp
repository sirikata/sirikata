#ifndef __SIRIKATA_JS_WHEN_HPP__
#define __SIRIKATA_JS_WHEN_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include <v8.h>

namespace Sirikata {
namespace JS {
namespace JSWatched{

v8::Handle<v8::Value>WatchedGet(v8::Local<v8::String> name,const AccessorInfo& info);
v8::Handle<v8::Value>WatchedSet(v8::Local<v8::String> name,v8::Local<v8::Value> value,const v8::AccessorInfo& info);




} //jswhen
} //js
} //sirikata

#endif
