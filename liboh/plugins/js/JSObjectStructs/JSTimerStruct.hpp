
#ifndef __SIRIKATA_JS_TIMER_STRUCT_HPP__
#define __SIRIKATA_JS_TIMER_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>



namespace Sirikata {
namespace JS {

struct JSTimerStruct{
    

    JSTimerStruct(JSObjectScript* jsobj, const Duration& dur,v8::Persistent<v8::Object>& targ, v8::Persistent<v8::Function>& callback,JSContextStruct* jscont, Sirikata::Network::IOService* ioserve);
    
    ~JSTimerStruct();
    
    static JSTimerStruct* decodeTimerStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);
    
    v8::Handle<v8::Value> struct_clear();
    v8::Handle<v8::Value> struct_resetTimer(double timeInSecondsToRefire);
    void evaluateCallback();
    
    Sirikata::Network::DeadlineTimer* mDeadlineTimer;
    JSObjectScript* jsObjScript;
    v8::Persistent<v8::Object> target;
    v8::Persistent<v8::Function> cb;

    JSContextStruct* jsContStruct;


};

}  //end js namespace
}  //end sirikata namespace

#endif
