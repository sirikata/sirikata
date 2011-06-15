
#ifndef __SIRIKATA_JS_JSOBJECTUTILS_HPP__
#define __SIRIKATA_JS_JSOBJECTUTILS_HPP__

#include <v8.h>
#include <sirikata/core/util/Platform.hpp>
#include "../JSObjectStructs/JSPositionListener.hpp"

namespace Sirikata{
namespace JS{

class JSWatchable;


JSPositionListener* decodeJSPosListener(v8::Handle<v8::Value> toDecode,String& errorMessage);
bool decodeString(v8::Handle<v8::Value> toDecode, String& decodedValue, String& errorMessage);
bool decodeBool(v8::Handle<v8::Value> toDecode, bool& decodedValue, String& errorMessage);
bool decodeSporef(v8::Handle<v8::Value> toDecode, SpaceObjectReference& sporef, String& errorMessage);
bool decodeUint64FromString(v8::Handle<v8::Value> toDecode,uint64& decodedInt, String& errorMessage);
bool decodeTimedMotionVector(v8::Handle<v8::Value>toDecodePos, v8::Handle<v8::Value>toDecodeVel,v8::Handle<v8::Value>toDecodeTimeAsString, TimedMotionVector3f& toDecodeTo, String& errorMessage);
bool decodeTimedMotionQuat(v8::Handle<v8::Value> orientationQuat,v8::Handle<v8::Value> orientationVelQuat,v8::Handle<v8::Value> toDecodeTimeAsString, TimedMotionQuaternion& toDecodeTo, String& errMsg);
bool decodeBoundingSphere3f(v8::Handle<v8::Value> toDecodeCenterVec, v8::Handle<v8::Value> toDecodeRadius, BoundingSphere3f& toDecodeTo, String& errMsg);

bool decodeUint32(v8::Handle<v8::Value> toDecode, uint32& toDecodeTo, String& errMsg);
bool decodeSolidAngle(v8::Handle<v8::Value> toDecode, SolidAngle& toDecodeTo, String& errMsg);


void debug_checkCurrentContextX(v8::Handle<v8::Context> ctx, String additionalMessage);
void printAllPropertyNames(v8::Handle<v8::Object> objToPrint);

String uint16StrToStr(v8::Handle<v8::String> toDeserialize);
v8::Handle<v8::Value> strToUint16Str(const String& toSerialize);





#define INLINE_SA_CONV_ERROR(toConvert,whereError,whichArg,whereWriteTo)   \
    SolidAngle whereWriteTo;                                                   \
    {                                                                      \
        String _errMsg = "In " #whereError "cannot convert arg " #whichArg " to solid angle";     \
        if (!decodeSolidAngle(toConvert,whereWriteTo,_errMsg))                 \
            return v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length())); \
    }

/**
   @param toConvert v8::Handle<v8::Value> that we will try to decode as a uint32
   @param whereWriteTo should be the name of the uint32 that we want to decode
   the uint32 to.  Shouldn't already be declared.  Gets declared in the
   following #define.
 */
#define INLINE_DECODE_UINT_32(toConvert,whereWriteTo)     \
    uint32 whereWriteTo; \
    { \
        String _errMsg;                                                 \
        if (! decodeUint32(toConvert,whereWriteTo,_errMsg))             \
            JSLOG(error, "error.  cannot decode the uint32 in INLINE_DECODE_UINT_32"); \
    }



/**
   @param toConvert is a v8::Handle<v8::Value> that we are trying to read as a
   string
   @param whereError should be the name of the function that running this in.
   Will be part of error message if cannot decode string.
   @param whichArg should be a number that corresponds to which arg couldn't
   decode.  Will be part of error message if cannot decode string.
   @param whereWriteTo should be the name of the string that we want to decode
   the string to.  shouldn't already be declared.  Becomes declared in the #define.
*/
#define INLINE_STR_CONV_ERROR(toConvert,whereError,whichArg,whereWriteTo)     \
    String whereWriteTo;                                                   \
    {                                                                      \
        String _errMsg = "In " #whereError "cannot convert arg " #whichArg " to string";     \
        if (!decodeString(toConvert,whereWriteTo,_errMsg))                 \
            return v8::Exception::Error(v8::String::New(_errMsg.c_str(), _errMsg.length())); \
    }

/**
   @param toConvert v8::Handle<v8::Value> that we are trying to read as a string
   @param whereWriteTo should be the name of the string that we want to decode
   the string to.  shouldn't already be declared.  Becomes declared in the
   #define.  If string conversion failed, log error, and write default value into string.
   @param defaultArg If string conversion fails, set whereWriteTo string to this string.
 */
#define INLINE_STR_CONV(toConvert,whereWriteTo,defaultArg)            \
    String whereWriteTo;                                               \
    {                                                                  \
        String _errMsg;                                                \
        if (!decodeString(toConvert,whereWriteTo,_errMsg))             \
        {                                                              \
            JSLOG(error,"error.  cannot decode the string value in INLINE_STR_CONV"); \
            whereWriteTo = defaultArg;                                 \
        }                                                              \
    }


} //end namespace js
} //end namespace sirikata

#endif
