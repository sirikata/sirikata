#ifndef __SIRIKATA_JS_SERIALIZE_HPP__
#define __SIRIKATA_JS_SERIALIZE_HPP__

#include <sirikata/oh/Platform.hpp>

#include <string>
#include "JS_JSMessage.pbj.hpp"
#include "JSObjectScript.hpp"
#include "EmersonScript.hpp"
#include <vector>
#include <map>

#include <v8.h>

namespace Sirikata {
namespace JS {

const static int32 JSSERIALIZER_TOKEN= 30130;
const static char* JSSERIALIZER_TOKEN_FIELD_NAME = "__JSSERIALIZER_TOKEN_FIELD_NAME&*^%$__";

//typedef std::map<int32,v8::Handle<v8::Object> >
typedef std::vector<v8::Handle<v8::Object > > ObjectVec;
typedef ObjectVec::iterator ObjectVecIter;

struct LoopedObjPointer
{
    LoopedObjPointer()
    {}
    LoopedObjPointer(const v8::Handle<v8::Object>& p, const String& n, int32 l)
     : parent(p),
       name(n),
       label(l)
    {}

    v8::Handle<v8::Object> parent;
    String name;
    int32 label;
};

typedef std::map<int32, v8::Handle<v8::Object> > ObjectMap;
typedef ObjectMap::iterator ObjectMapIter;

typedef std::map<int32, LoopedObjPointer> FixupMap;
typedef FixupMap::iterator FixupMapIter;

void debug_printSerialized(Sirikata::JS::Protocol::JSMessage jm, String prepend);


class JSSerializer
{
    static void pointOtherObject(int32 int32ToPointTo,Sirikata::JS::Protocol::IJSFieldValue& jsf_value);

    static void annotateObject(ObjectVec& objVec, v8::Handle<v8::Object> v8Obj,int32 toStampWith);

    static void unmarkSerialized(ObjectVec& toUnmark);

    static void serializeFunction(v8::Local<v8::Function> v8Func, Sirikata::JS::Protocol::JSMessage&,int32& toStampWith,ObjectVec& allObjs);

    static void serializeFunctionInternal(v8::Local<v8::Function> funcToSerialize, Sirikata::JS::Protocol::IJSFieldValue& field_to_put_in, int32& toStampWith);


    static void serializeVisible(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs);
    static void fillVisible(Sirikata::JS::Protocol::IJSMessage&, const SpaceObjectReference& listenTo);// Reused by serializePresence
    static void serializePresence(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs);
    static void serializeSystem(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs);
    static void serializeObjectInternal(v8::Local<v8::Value> v8Val, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs );

    static void serializeInternalFields(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&,ObjectVec& allObjs);
    static void serializeAddressable(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&,ObjectVec& allObjs);


    static bool deserializePerformFixups(ObjectMap& labeledObjs, FixupMap& toFixUp);


    static bool deserializeObjectInternal( EmersonScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo, ObjectMap& labeledObjs,FixupMap& toFixUp);


public:
    static std::string serializeObject(v8::Local<v8::Value> v8Val,int32 toStamp = 0);
    static bool deserializeObject(EmersonScript*, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo);


};

}}//end namespaces

#endif
