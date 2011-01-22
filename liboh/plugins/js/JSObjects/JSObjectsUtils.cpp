#include <v8.h>

#include "JSObjectsUtils.hpp"
#include <cassert>
#include <sirikata/core/util/Platform.hpp>


namespace Sirikata{
namespace JS{




bool decodeString(v8::Handle<v8::Value> toDecode, String& decodedValue, String& errorMessage)
{
    v8::String::Utf8Value str(toDecode);

    //can decode string
    if (*str)
    {
        decodedValue = String(*str);
        return true;
    }

    errorMessage += "Error decoding string in decodeString of JSObjectUtils.cpp.  ";
    return false;
}


//returns whether the decode operation was successful or not.  if successful,
//updates the value in decodeValue to the decoded value, errorMessage contains
//string associated with failure if decoding fales
bool decodeBool(v8::Handle<v8::Value> toDecode, bool& decodedValue, std::string& errorMessage)
{
    if (! toDecode->IsBoolean())
    {
        errorMessage += "  Error in decodeBool in JSObjectUtils.cpp.  Not given a boolean emerson object to decode.";
        return false;
    }

    v8::Handle<v8::Boolean> boolean = toDecode->ToBoolean();
    decodedValue = boolean->Value();
    return true;
}


}//namespace js
}//namespace sirikata

