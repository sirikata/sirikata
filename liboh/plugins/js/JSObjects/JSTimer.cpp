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



namespace Sirikata{
namespace JS{
namespace JSTimer{

//this function takes in a single argument: when the callback should actually
//fire from now.
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


//actually turns the timer off until call reset on it.
v8::Handle<v8::Value> clear(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Incorrect number of arguments for clear function of JSTimer.cpp.  Clear should not take in any arguments.")) );

    //decode associated timerstruct
    String errorMessage = "Error in clear of JSTimer.cpp trying to decode jstimer.  ";
    JSTimerStruct* jstimer = JSTimerStruct::decodeTimerStruct(args.This(),errorMessage);
    if (jstimer == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str())));
    
    return jstimer->clear();
}

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


}//JSTimer namespace
}//JS namespace
}//sirikata namespace
