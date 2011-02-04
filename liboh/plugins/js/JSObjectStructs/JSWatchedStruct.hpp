
#ifndef __SIRIKATA_JS_WATCHED_STRUCT_HPP__
#define __SIRIKATA_JS_WATCHED_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "JSWatchable.hpp"


namespace Sirikata {
namespace JS {

struct JSWatchedStruct : public JSWatchable
{
    JSWatchedStruct(v8::Persistent<v8::Object> toWatch, JSObjectScript* jsobj);
    ~JSWatchedStruct();
    
    static JSWatchedStruct* decodeWatchedStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);

    v8::Persistent<v8::Object> mWatchedObject;
    JSObjectScript* mJSObj;

};

}  //end js namespace
}  //end sirikata namespace

#endif
