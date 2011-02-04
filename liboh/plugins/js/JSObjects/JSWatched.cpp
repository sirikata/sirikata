#include "JSWatched.hpp"
#include <v8.h>
#include "../JSObjectScript.hpp"
#include "JSObjectsUtils.hpp"
#include "../JSUtil.hpp"
#include "../JSObjectStructs/JSWatchedStruct.hpp"


namespace Sirikata{
namespace JS{
namespace JSWatched{


v8::Handle<v8::Value>WatchedGet(v8::Local<v8::String> name,const AccessorInfo& info)
{
    //don't care about this;
    v8::Handle<v8::Object> actualObject  = info.Holder();
    if (!actualObject->Has(name))
    {
        String nameString,dummyError,errorMessage;
        decodeString(name,nameString,dummyError);
        errorMessage = "Error in WatchedGet of JSWatched.cpp: object does not contain field " + nameString;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );
    }

    return actualObject->Get(name);
}

v8::Handle<v8::Value>WatchedSet(v8::Local<v8::String> name,v8::Local<v8::Value> value,const v8::AccessorInfo& info)
{
    v8::Handle<v8::Object>actualObject = info.Holder();

    String errorMessage = "Error in WathcedSet of JSWatched.cpp: cannot decode jswatcedstruct.  ";
    JSWatchedStruct* jswatched = JSWatchedStruct::decodeWatchedStruct(actualObject,errorMessage);
    if (jswatched == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );

    
    if (!actualObject->Has(name))
    {
        String nameString,dummyError,errorMessage;
        decodeString(name,nameString,dummyError);
        errorMessage = "Error in WatchedGet of JSWatched.cpp: object does not contain field " + nameString;
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())) );
    }


    actualObject->Set(name,value);
    
    jswatched->setFlag();
    return value;
}




}//JSWatched namespace
}//JS namespace
}//sirikata namespace
