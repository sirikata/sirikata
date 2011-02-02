#include "JSWhen.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "../JSSerializer.hpp"
#include "JSObjectsUtils.hpp"
#include "JSHandler.hpp"
#include "JSVec3.hpp"
#include "../JSUtil.hpp"
#include <math.h>

#include <sirikata/core/util/Random.hpp>



namespace Sirikata{
namespace JS{
namespace JSWhen{


v8::Handle<v8::Value> WhenSuspend(const v8::Arguments& args)
{
    String errorMessage = "Error in WhenSuspend of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    return jswhen->struct_whenSuspend();
}

v8::Handle<v8::Value> WhenResume(const v8::Arguments& args)
{
    String errorMessage = "Error in WhenResume of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    return jswhen->struct_whenResume();
}


v8::Handle<v8::Value> WhenGetPeriod(const v8::Arguments& args)
{
    String errorMessage = "Error in WhenGetPeriod of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    return jswhen->struct_whenGetPeriod();    
}

v8::Handle<v8::Value> WhenSetPeriod(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in WhenSetPeriod: require a new period to set (units of seconds) (or null if want to not have period)")));

    //decoding whenstruct
    String errorMessage = "Error in WhenSetPeriod of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


    // //if receive a null argument, then that means don't fire a timer
    // if (args[0]->IsNull())
    //     return jswhen->clearPeriod();
        
    //otherwise, check to make sure that 
    if (! NumericValidate(args[0]))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in WhenSetPeriod: require a new period to set (units of seconds) (or null if want to not have period)")));

    double newPeriod = NumericExtract(args[0]);

    if (newPeriod < JSWhenStruct::WHEN_MIN_PERIOD)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in WhenSetPerod: requested period is below minimum allowable (or null if want to not have period)")));

    return jswhen->struct_setPeriod(newPeriod);
}


v8::Handle<v8::Value> WhenGetMinPeriod(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    return v8::Number::New(JSWhenStruct::WHEN_MIN_PERIOD);
}


v8::Handle<v8::Value> WhenGetState(const v8::Arguments& args)
{
    //decoding whenstruct
    String errorMessage = "Error in WhenGetState of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    return jswhen->struct_whenGetState();
}


v8::Handle<v8::Value> WhenSetState(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in WhenSetState of JSWhen.cpp: requires one argument (bool).  True means that the predicate will have to evaluate to false for the condition to fire; false means that the predicate will have to evaluate to true for the condition to fire")));
    

    //decoding bool argument
    String boolErrorMessage = "Error in WhenSetState of JSWhen.cpp.  Require a single bool argument.  True means that the predicate will have to evaluate to false for the condition to fire; false means that the predicate will have to evaluate to true for the condition to fire.  ";
    bool boolArg;
    bool boolDecodeSuccessful = decodeBool(args[0],boolArg, boolErrorMessage);
    if (! boolDecodeSuccessful)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(boolErrorMessage.c_str(),boolErrorMessage.length())));

        
    
    //decoding whenstruct
    String errorMessage = "Error in WhenSetState of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    
    return jswhen->struct_whenSetState(boolArg);
}



}//JSWhen namespace
}//JS namespace
}//sirikata namespace
