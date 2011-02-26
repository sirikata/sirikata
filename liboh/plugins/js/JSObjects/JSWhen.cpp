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

    return jswhen->suspend();
}

v8::Handle<v8::Value> WhenResume(const v8::Arguments& args)
{
    String errorMessage = "Error in WhenResume of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    return jswhen->resume();
}



v8::Handle<v8::Value> WhenGetLastPredState(const v8::Arguments& args)
{
    //decoding whenstruct
    String errorMessage = "Error in WhenGetState of JSWhen.cpp.  Cannot decode jswhenstruct.  ";
    JSWhenStruct* jswhen = JSWhenStruct::decodeWhenStruct(args.This(),errorMessage);
    if (jswhen == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

    return jswhen->struct_whenGetLastPredState();
}





}//JSWhen namespace
}//JS namespace
}//sirikata namespace
