
#ifndef __SIRIKATA_JS_TIMER_STRUCT_HPP__
#define __SIRIKATA_JS_TIMER_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "JSSuspendable.hpp"


namespace Sirikata {
namespace JS {

struct JSTimerStruct : public JSSuspendable
{

    JSTimerStruct(JSObjectScript* jsobj, const Duration& dur, v8::Persistent<v8::Function>& callback,JSContextStruct* jscont, Sirikata::Network::IOService* ioserve);

    ~JSTimerStruct();

    static JSTimerStruct* decodeTimerStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);


    v8::Handle<v8::Value> struct_resetTimer(double timeInSecondsToRefire);
    void evaluateCallback(const boost::system::error_code& error);

    virtual v8::Handle<v8::Value>suspend();
    virtual v8::Handle<v8::Value>resume();
    virtual v8::Handle<v8::Value>clear();


    JSObjectScript* jsObjScript;
    v8::Persistent<v8::Function> cb;
    JSContextStruct* jsContStruct;
    Sirikata::Network::DeadlineTimer* mDeadlineTimer;
    double timeUntil; //time until the timer fires

};

typedef std::map<JSTimerStruct*,int>  TimerMap;
typedef TimerMap::iterator TimerIter;

}  //end js namespace
}  //end sirikata namespace

#endif
