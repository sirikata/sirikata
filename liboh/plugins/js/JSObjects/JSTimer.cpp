//#include "JSMath.hpp"

#include <v8.h>
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include <math.h>
#include "../JSObjectStructs/JSTimerStruct.hpp"
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/util/Random.hpp>
#include "JSTimer.hpp"


namespace Sirikata{
namespace JS{
namespace JSTimer{

/**
   @param numeric, specifying how long from now to go before re-firing the timer.
   @return boolean.

   This function takes in a single argument: when the callback should actually
   fire from now.
*/
v8::Handle<v8::Value> resetTimer(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Incorrect number of arguments for resetTimer function of JSTimer.cpp.  Requires a single number for when timer should fire.")) );

    // Duration
    v8::Handle<v8::Value> dur = args[0];
    double native_dur = 0;
    if (dur->IsNumber())
        native_dur = dur->NumberValue();
    else if (dur->IsInt32())
        native_dur = dur->Int32Value();
    else
        return v8::ThrowException( v8::Exception::Error(v8::String::New("In resetTimer of JSTimer.cpp.  First argument incorrect: duration cannot be cast to float.")) );

    //now decode associated timerstruct
    String errorMessage = "Error in resetTimer of JSTimer.cpp trying to decode jstimerstruct.  ";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(args.This(),errorMessage);
    if (jstimer == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return jstimer->struct_resetTimer(native_dur);
}

v8::Handle<v8::Value> getAllData(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("getAllData requires 0 arguments.  Aborting.")) );

    String errorMessage = "Error in getAllData of JSTimer.cpp trying to decode jstimerstruct.  ";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(args.This(),errorMessage);
    if (jstimer == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    return handle_scope.Close(jstimer->struct_getAllData());
}


/**
   Calling clear prevents the timer from ever re-firing again.
 */
v8::Handle<v8::Value> clear(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Incorrect number of arguments for clear function of JSTimer.cpp.  Clear should not take in any arguments.")) );

    //decode associated timerstruct
    INLINE_DECODE_TIMER_ERROR(args.This(),clear,jstimer);

    setNullTimer(args);
    return jstimer->clear();
}

/**
   Calling suspend prevents the timer from firing again until it is resumed.
 */
v8::Handle<v8::Value> suspend(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Incorrect number of arguments for suspend function of JSTimer.cpp.  Requires exactly zero arguments.")) );

    //decode associated timerstruct
    String errorMessage = "Error in suspend of JSTimer.cpp trying to decode jstimer.  ";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(args.This(),errorMessage);
    if (jstimer == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));
    
    return jstimer->suspend();
    
}

v8::Handle<v8::Value> getType(const v8::Arguments& args)
{
    return v8::String::New("timer");
}


/**
   Calling this function on a timer makes it so that the timer's associated
   callback will fire however many seconds from now the timer was initialized with.
 */
v8::Handle<v8::Value> resume(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Incorrect number of arguments for resume function of JSTimer.cpp.  Requires zero arguments.")) );

    //decode associated timerstruct
    String errorMessage = "Error in resume of JSTimer.cpp trying to decode jstimer.  ";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(args.This(),errorMessage);
    if (jstimer == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));
    
    return jstimer->resume();

}

/**
   @return Boolean corresponding to whether the timer is currently suspended.
 */
v8::Handle<v8::Value> isSuspended(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Incorrect number of arguments for isSuspended function of JSTimer.cpp.  Requires zero arguments.")) );

    String errorMessage = "Error in isSuspended of JSTimer.cpp trying to decode jstimer.  ";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(args.This(),errorMessage);
    if (jstimer == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));

    
    return jstimer->getIsSuspendedV8();
}

/**
   Sets args.This to point to null instead of a timer object.
 */
void setNullTimer(const v8::Arguments& args)
{
    v8::Handle<v8::Object> mTimer = args.This();

    //grabs the internal pattern
    //(which has been saved as a pointer to JSTimerStruct
    if (mTimer->InternalFieldCount() > 0)
        mTimer->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD,External::New(NULL));
    else
        v8::Handle<v8::Object>::Cast(mTimer->GetPrototype())->SetInternalField(TIMER_JSTIMERSTRUCT_FIELD, External::New(NULL));
}

}//JSTimer namespace
}//JS namespace
}//sirikata namespace
