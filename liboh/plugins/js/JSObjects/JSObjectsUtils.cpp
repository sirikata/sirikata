#include <v8.h>

#include "JSObjectsUtils.hpp"
#include <cassert>
#include <sirikata/core/util/Platform.hpp>
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "../JSObjectStructs/JSPresenceStruct.hpp"
#include "../JSObjectStructs/JSWhenStruct.hpp"
#include "../JSObjectStructs/JSPositionListener.hpp"

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

//Tries to decode toDecode as either a presence struct or a visible struct (the
//two classes that inherit from jsposition listener).
JSPositionListener* decodeJSPosListener(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    //try to decode as presence first;
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(toDecode,errorMessage);
    if (jspres != NULL)
        return jspres;
        
    JSVisibleStruct* jsvis = JSVisibleStruct::decodeVisible(toDecode,errorMessage);
    if (jsvis != NULL)
        return jsvis;

    return NULL;
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

//This function just prints out all properties associated with context ctx has
//additional parameter additionalMessage which prints out at the top of this
//function's debugging message.  additionalMessage arose as a nice way to print
//the context multiple times and still be able to keep straight which printout
//was associated with which call to debug_checkCurrentContextX by specifying
//unique additionalMessages each time the function was called.
void debug_checkCurrentContextX(v8::Handle<v8::Context> ctx, std::string additionalMessage)
{
    v8::HandleScope handle_scope;
    std::cout<<"\n\n\nDoing checkCurrentContext with value passed in of: "<<additionalMessage<<"\n\n";
    printAllPropertyNames(ctx->Global());
    std::cout<<"\n\n";
}


void printAllPropertyNames(v8::Handle<v8::Object> objToPrint)
{
   v8::Local<v8::Array> allProps = objToPrint->GetPropertyNames();

    std::vector<v8::Local<v8::Object> > propertyNames;
    for (int s=0; s < (int)allProps->Length(); ++s)
    {
        v8::Local<v8::Object>toPrint= v8::Local<v8::Object>::Cast(allProps->Get(s));
        String errorMessage = "Error: error decoding first string in debug_checkCurrentContextX.  ";
        String strVal, strVal2;
        bool stringDecoded = decodeString(toPrint, errorMessage, strVal);
        if (!stringDecoded)
        {
            SILOG(js,error,errorMessage);
            return;
        }
        
        v8::Local<v8::Value> valueToPrint = objToPrint->Get(v8::String::New(strVal.c_str(), strVal.length()));
        errorMessage = "Error: error decoding second string in debug_checkCurrentContextX.  ";
        stringDecoded =  decodeString(valueToPrint, errorMessage, strVal2);
        if (!stringDecoded)
        {
            SILOG(js,error,errorMessage);
            return;
        }
        std::cout<<"      property "<< s <<": "<<strVal <<": "<<strVal2<<"\n";
    }
}



}//namespace js
}//namespace sirikata

