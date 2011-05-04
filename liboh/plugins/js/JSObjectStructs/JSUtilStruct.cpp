
#include <v8.h>
#include "JSUtilStruct.hpp"
#include "JSContextStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSObjectScript.hpp"
#include "JSPresenceStruct.hpp"
#include "JSUtilStruct.hpp"

namespace Sirikata{
namespace JS{


JSUtilStruct::JSUtilStruct ( JSContextStruct* jscont, JSObjectScript* jsObj)
 : associatedContext(jscont),
   associatedObjScr(jsObj)
{
}


JSUtilStruct::~JSUtilStruct()
{
}


v8::Handle<v8::Value> JSUtilStruct::struct_createVec3(Vector3d& toCreate )
{
    return associatedContext->struct_createVec3(toCreate);
}



//decodes util struct object
JSUtilStruct* JSUtilStruct::decodeUtilStruct(v8::Handle<v8::Value> toDecode ,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of JSUtilStruct.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != UTIL_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of JSUtilStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSUtilStructObj;
    wrapJSUtilStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(UTIL_TEMPLATE_UTILSTRUCT_FIELD));
    void* ptr = wrapJSUtilStructObj->Value();


    JSUtilStruct* returner;
    returner = static_cast<JSUtilStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSUtilStruct.  Internal field of object given cannot be casted to a JSUtilStruct.";

    return returner;
}



} //end namespace JS
} //end namespace Sirikata
