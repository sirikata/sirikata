
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


void debug_checkCurrentContextX(v8::Handle<v8::Context> ctx, String additionalMessage);
void printAllPropertyNames(v8::Handle<v8::Object> objToPrint);




} //end namespace js
} //end namespace sirikata

#endif
