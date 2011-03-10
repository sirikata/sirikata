#ifndef __SIRIKATA_JS_UTIL_STRUCT_HPP__
#define __SIRIKATA_JS_UTIL_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>



namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSContextStruct;
class JSVisibleStruct;
class JSPresenceStruct;

struct JSUtilStruct
{
    JSUtilStruct(JSContextStruct* jscont, JSObjectScript* jsObj);
    ~JSUtilStruct();

    static JSUtilStruct* decodeUtilStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    v8::Handle<v8::Value> struct_createQuotedObject(const String& toQuote);
    v8::Handle<v8::Value> struct_createWhen(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback);
    v8::Handle<v8::Value> struct_createWhenWatchedItem(v8::Handle<v8::Array>itemArray);
    v8::Handle<v8::Value> struct_createWhenWatchedList(v8::Handle<v8::Array>arrayOfItems);

    
    v8::Handle<v8::Value> struct_createWhenTimeoutLT(
        JSPresenceStruct* presStruct_LHS_1,JSVisibleStruct* visStruct_LHS_1,Vector3d& vec3_LHS_1,
        JSPresenceStruct* presStruct_LHS_2,JSVisibleStruct* visStruct_LHS_2,Vector3d& vec3_LHS_2,
        double ltRHS
    );
    
private:

    
    
    //associated data 
    JSContextStruct* associatedContext;
    JSObjectScript* associatedObjScr;
};


}//end namespace js
}//end namespace sirikata

#endif
