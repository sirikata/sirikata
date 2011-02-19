#ifndef __SIRIKATA_JS_UTIL_STRUCT_HPP__
#define __SIRIKATA_JS_UTIL_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>



namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSObjectScript;
class JSContextStruct;

struct JSUtilStruct
{
    JSUtilStruct(JSContextStruct* jscont, JSObjectScript* jsObj)
    ~JSUtilStruct();

    static JSUtilStruct* decodeUtilStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

private:

    String whenCreateQuoted(v8::Handle<v8::Array>predArray);
    
    //associated data 
    JSContextStruct* associatedContext;
    JSObjectScript* associatedObjScr;
};


}//end namespace js
}//end namespace sirikata

#endif
