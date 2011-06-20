
#ifndef __SIRIKATA_JS_TIMER_STRUCT_HPP__
#define __SIRIKATA_JS_TIMER_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "../EmersonScript.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "JSSuspendable.hpp"


namespace Sirikata {
namespace JS {

struct JSTimerStruct : public JSSuspendable
{


    JSTimerStruct(EmersonScript*eobj,Duration dur,v8::Persistent<v8::Function>& callback,JSContextStruct* jscont,Sirikata::Network::IOService* ioserve,uint32 contID, double timeRemaining, bool isSuspended,bool isCleared);
    
    ~JSTimerStruct();

    static JSTimerStruct* decodeTimerStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);


    v8::Handle<v8::Value> struct_resetTimer(double timeInSecondsToRefire);
private:
    void evaluateCallback();
public:
    virtual v8::Handle<v8::Value>suspend();
    virtual v8::Handle<v8::Value>resume();
    virtual v8::Handle<v8::Value>clear();

    v8::Handle<v8::Value> struct_getAllData();
    
    EmersonScript* emerScript;
    v8::Persistent<v8::Function> cb;
    JSContextStruct* jsContStruct;
private:
    Sirikata::Network::IOService* ios;
    Sirikata::Network::IOTimerPtr mDeadlineTimer;
public:

    Duration timeUntil; //first time create timer will fire after timeUntil seconds
    double mTimeRemaining; //when restoring a timer, will fire in this many more
                           //seconds.

    
    virtual void fixSuspendableToContext(JSContextStruct* toAttachTo);


    /**
       When the only reference to a timer are weak (ie, no emerson objects
       directly point at it, this function gets called.  The first argument
       contains the persistent object pointing to the timer.  The second should
       just be null.  
     */
    static void timerWeakReferenceCleanup(v8::Persistent<v8::Value> containsTimer, void* otherArg);

    /**
       When no longer have a reference to the Emerson object holding this timer
       and garbage collection is occurring, this function gets called.  If the
       timer has no pending events (and won't have any pending events in the
       future [eg, it isn't suspended and its parent context isn't suspended]),
       then delete the timer right then and there.  Otherwise, delete it after
       it fires.  (Mark killAfterFire to be true.)
     */
    void noReference();

    
private:
    /**
       Need to know when to clean up timer struct.  Have made Emerson objects
       that are timers weak.  This means that as soon as the objects holding
       them go out of scope, we receive a callback.  If the timer has not yet
       fired, we do not want to delete JSTimerStruct.  Therefore, we wait until
       the timer has fired before freeing the memory associated with this timer
       struct.  Setting killAfterFire to true instructs this struct to clear and
       free itself after it fires and executes its callback.
     */
    bool killAfterFire;

    /**
       True if have no callback waiting to be executed. False otherwise.
     */
    bool noTimerWaiting;
};


#define INLINE_TIMER_CONV_ERROR(toConvert,whereError,whichArg,whereWriteTo)   \
    JSTimerStruct* whereWriteTo;                                                   \
    {                                                                      \
        String _errMsg = "In " #whereError "cannot convert " #whichArg " to timer struct";     \
        whereWriteTo = JSTimerStruct::decodeTimerStruct(toConvert,_errMsg); \
        if (whereWriteTo == NULL) \
            return v8::ThrowException(v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length()))); \
    }




typedef std::map<JSTimerStruct*,int>  TimerMap;
typedef TimerMap::iterator TimerIter;

}  //end js namespace
}  //end sirikata namespace

#endif
