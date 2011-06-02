#include <v8.h>

#include "JSObjectsUtils.hpp"
#include <cassert>
#include <sirikata/core/util/Platform.hpp>
#include "../JSObjectStructs/JSVisibleStruct.hpp"
#include "../JSObjectStructs/JSPresenceStruct.hpp"
#include "../JSObjectStructs/JSPositionListener.hpp"
#include <boost/lexical_cast.hpp>
#include "JSVec3.hpp"
#include "JSQuaternion.hpp"
#include "../JSLogging.hpp"


namespace Sirikata{
namespace JS{

bool decodeUint32(v8::Handle<v8::Value> toDecode, uint32& toDecodeTo, String& errMsg)
{
    if (!toDecode->IsUint32())
    {
        errMsg += "  Could not decode as uint32.";
        return false;
    }
    
    toDecodeTo = toDecode->ToUint32()->Value();

    return true;
}



v8::Handle<v8::Value> strToUint16Str(const String& toSerialize)
{
    uint16* toStoreArray = new uint16[toSerialize.size()];

    for (String::size_type s=0; s < toSerialize.size(); ++s)
        toStoreArray[s] = (uint16)(toSerialize[s]);

    v8::Handle<v8::Value> returner = v8::String::New(toStoreArray,toSerialize.size());

    delete toStoreArray;
    return returner;
}


String uint16StrToStr(v8::Handle<v8::String> toDeserialize)
{
    uint16* toReadArray = new uint16[toDeserialize->Length()];
    char* binArray = new char[toDeserialize->Length()];
    
    toDeserialize->Write(toReadArray,0, toDeserialize->Length());

    for (int s=0; s < toDeserialize->Length(); ++s)
        binArray[s] = (char)(toReadArray[s]);

    String returner (binArray, toDeserialize->Length());

    delete toReadArray;
    delete binArray;
    return returner;
}



bool decodeString(v8::Handle<v8::Value> toDecode, String& decodedValue, String& errorMessage)
{
    v8::String::Utf8Value str(toDecode);
    //can decode string
    if (*str)
    {
        decodedValue = String(*str,str.length());
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


bool decodeSporef(v8::Handle<v8::Value> toDecode, SpaceObjectReference& sporef, String& errorMessage)
{
    String sporefStr;
    bool strDecode = decodeString(toDecode,sporefStr,errorMessage);
    if (! strDecode )
        return false;

    try
    {
        sporef  = SpaceObjectReference(sporefStr);
    }
    catch (std::invalid_argument& ia)
    {
        errorMessage += "  Could not convert string to sporef when decoding sporef.";
        return false;
    }

    return true;
}

bool decodeUint64FromString(v8::Handle<v8::Value> toDecode,uint64& decodedInt, String& errorMessage)
{
    String decodedVal;
    bool strDecoded = decodeString(toDecode,decodedVal,errorMessage);
    if (!strDecoded)
    {
        errorMessage += "Could not convert value to string.  ";
        return false;
    }

    try
    {
        decodedInt = boost::lexical_cast<uint64>(decodedVal);
    }
    catch(boost::bad_lexical_cast & blc)
    {
        errorMessage += "Could not convert string to uint64.";
        return false;
    }

    return true;
}

//does not assume that toDecode is a string.  Checks it first.
bool decodeTimeFromString (v8::Handle<v8::Value> toDecode, Time& toDecodeTo, String& errorMessage)
{
    uint64 timeVal;
    bool isUint64 = decodeUint64FromString(toDecode,timeVal,errorMessage);

    if (! isUint64)
    {
        errorMessage += "Time to decode was not a uint64.  ";
        return false;
    }
    
    toDecodeTo = Time(timeVal);

    return true;
}



bool decodeTimedMotionVector(v8::Handle<v8::Value>toDecodePos, v8::Handle<v8::Value>toDecodeVel,v8::Handle<v8::Value>toDecodeTimeAsString, TimedMotionVector3f& toDecodeTo, String& errorMessage)
{
    //decode position
    bool isVec3 = Vec3ValValidate(toDecodePos);
    if (! isVec3)
    {
        errorMessage += "Could not convert position to a vector.  ";
        return false;
    }
    Vector3f position = Vec3ValExtractF(toDecodePos);

    //decode velocity
    isVec3 = Vec3ValValidate(toDecodeVel);
    if (! isVec3)
    {
        errorMessage += "Could not convert velocity to a vector.  ";
        return false;
    }
    Vector3f velocity = Vec3ValExtractF(toDecodeVel);

    //decode time.
    Time decodedTime;
    bool timeDecoded = decodeTimeFromString(toDecodeTimeAsString,decodedTime,errorMessage);
    if (! timeDecoded)
    {
        errorMessage += "Could not convert value to a time when deconding timed motion vector.  ";
        return false;
    }

    MotionVector3f motVec(position,velocity);
    toDecodeTo = TimedMotionVector3f(decodedTime,motVec);
    return true;
}

bool decodeTimedMotionQuat(v8::Handle<v8::Value> orientationQuat,v8::Handle<v8::Value> orientationVelQuat,v8::Handle<v8::Value> toDecodeTimeAsString, TimedMotionQuaternion& toDecodeTo, String& errorMessage)
{
    //decode orientation
    bool isQuat =QuaternionValValidate(orientationQuat);
    if (! isQuat)
    {
        errorMessage += "Could not convert orientation to a quaternion.  ";
        return false;
    }
    Quaternion orientation = QuaternionValExtract(orientationQuat);

    //decode orientation velocity
    isQuat = QuaternionValValidate(orientationVelQuat);
    if (! isQuat)
    {
        errorMessage += "Could not convert orientation velocity to a quaternion.  ";
        return false;
    }
    Quaternion orientationVel = QuaternionValExtract(orientationVelQuat);

    //decode time.
    Time decodedTime;
    bool timeDecoded = decodeTimeFromString(toDecodeTimeAsString,decodedTime,errorMessage);
    if (! timeDecoded)
    {
        errorMessage += "Could not convert value to a time when deconding timed motion vector.  ";
        return false;
    }

    MotionQuaternion motQuat(orientation,orientationVel);
    toDecodeTo = TimedMotionQuaternion(decodedTime,motQuat);
    return true;
}

bool decodeBoundingSphere3f(v8::Handle<v8::Value> toDecodeCenterVec, v8::Handle<v8::Value> toDecodeRadius, BoundingSphere3f& toDecodeTo, String& errMsg)
{
    bool isVec3 = Vec3ValValidate(toDecodeCenterVec);
    if (! isVec3)
    {
        errMsg += "Error decoding bounding sphere.  Position argument passed in for center of sphere is not a vec3.";
        return false;
    }

    Vector3f centerPos = Vec3ValExtractF(toDecodeCenterVec);

    bool isNumber  = NumericValidate(toDecodeRadius);
    if (! isNumber)
    {
        errMsg += "Error decoding bounding sphere.  Radius argument passed in is not a number.  ";
        return false;
    }


    double rad = NumericExtract(toDecodeRadius);


    toDecodeTo = BoundingSphere3f(centerPos, rad);
    return true;
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
        bool stringDecoded = decodeString(toPrint, strVal,errorMessage);
        if (!stringDecoded)
        {
            SILOG(js,error,errorMessage);
            return;
        }

        v8::Local<v8::Value> valueToPrint = objToPrint->Get(v8::String::New(strVal.c_str(), strVal.length()));
        errorMessage = "Error: error decoding second string in debug_checkCurrentContextX.  ";
        stringDecoded =  decodeString(valueToPrint,strVal2,errorMessage);
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
