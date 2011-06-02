#ifndef __SIRIKATA_JS_UTIL_STRUCT_HPP__
#define __SIRIKATA_JS_UTIL_STRUCT_HPP__

#include <sirikata/oh/HostedObject.hpp>
#include <v8.h>



namespace Sirikata {
namespace JS {

//need to forward-declare this so that can reference this inside
class JSContextStruct;
class JSVisibleStruct;
class JSPresenceStruct;

struct JSUtilStruct
{
    JSUtilStruct(JSContextStruct* jscont);
    ~JSUtilStruct();

    static JSUtilStruct* decodeUtilStruct(v8::Handle<v8::Value> toDecode ,std::string& errorMessage);

    v8::Handle<v8::Value> struct_createVec3(Vector3d& toCreate );

private:
    //associated data
    JSContextStruct* associatedContext;
};


}//end namespace js
}//end namespace sirikata

#endif
