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
const static char* JSSERIALIZER_ROOT_OBJ_TOKEN   = "__JSDESERIALIZER_ROOT_OBJECT_NAME__";


//Mult identify prototype of objects uniquely.  Cannot use "prototype" because
//functions have prototype objects.  Must use a word that cannot be used to name
//any other field.  Going with "this".
const static char* JSSERIALIZER_PROTOTYPE_NAME= "this";

const static char* FUNCTION_CONSTRUCTOR_TEXT = "function Function() { [native code] }";

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

typedef std::vector<LoopedObjPointer> LoopedObjPointerList;
typedef std::map<int32, LoopedObjPointerList> FixupMap;
typedef FixupMap::iterator FixupMapIter;

void debug_printSerialized(Sirikata::JS::Protocol::JSMessage jm, String prepend);
void debug_printSerializedFieldVal(Sirikata::JS::Protocol::JSFieldValue jsfieldval, String prepend,String name);

class JSSerializer
{
    static void pointOtherObject(int32 int32ToPointTo,Sirikata::JS::Protocol::IJSFieldValue& jsf_value);

    static void annotateObject(ObjectVec& objVec, v8::Handle<v8::Object> v8Obj,int32 toStampWith);

    static void unmarkSerialized(ObjectVec& toUnmark);
    static void unmarkDeserialized(ObjectMap& objMap);

    static void setPrototype(v8::Handle<v8::Object> toSetProtoOf, v8::Handle<v8::Object> toSetTo);
    
    static void serializeVisible(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs);
    static void fillVisible(Sirikata::JS::Protocol::IJSMessage&, const SpaceObjectReference& listenTo);// Reused by serializePresence
    static void serializePresence(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs);
    static void serializeSystem(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs);
    static void serializeObjectInternal(v8::Local<v8::Value> v8Val, Sirikata::JS::Protocol::IJSMessage&,int32& toStampWith,ObjectVec& allObjs );

    static void serializeInternalFields(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&,ObjectVec& allObjs);
    static void serializeAddressable(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::JSMessage&,ObjectVec& allObjs);


    static void shallowCopyFields(v8::Handle<v8::Object> dst, v8::Handle<v8::Object> src);
    static bool deserializePerformFixups(ObjectMap& labeledObjs, FixupMap& toFixUp);

    static void serializeFieldValueInternal(Sirikata::JS::Protocol::IJSFieldValue& jsf_value,v8::Handle<v8::Value> prop_val,int32 & toStampWith,ObjectVec& objVec);

    
    static bool deserializeObjectInternal( EmersonScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo, ObjectMap& labeledObjs,FixupMap& toFixUp);

    static v8::Handle<v8::Value> deserializeFieldValue(EmersonScript* emerScript,
        Sirikata::JS::Protocol::JSFieldValue jsvalue, ObjectMap& labeledObjs,FixupMap& toFixUp,
        int32& toLoopTo);


public:
    
    //deprecated
    static std::string serializeObject(v8::Local<v8::Value> v8Val,int32 toStamp = 0);
    static std::string serializeMessage(v8::Local<v8::Value> v8Val, int32 toStamp=0);
    
    static v8::Handle<v8::Value> deserializeMessage( EmersonScript* emerScript, Sirikata::JS::Protocol::JSFieldValue jsfieldval,bool& deserializeSuccessful);
    
    static v8::Handle<v8::Object> deserializeObject( EmersonScript* emerScript, Sirikata::JS::Protocol::JSMessage jsmessage,bool& deserializeSuccessful);
};

}}//end namespaces

#endif
