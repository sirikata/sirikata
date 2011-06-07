#include "JSSystem.hpp"

#include <sirikata/oh/Platform.hpp>

#include "../JSObjectScriptManager.hpp"
#include "../JSObjectScript.hpp"

#include "../JSSerializer.hpp"
#include "../JSPattern.hpp"
#include "../JSObjectStructs/JSContextStruct.hpp"
#include "JSFields.hpp"
#include "JSObjectsUtils.hpp"
#include "../JSSystemNames.hpp"
#include "../JSObjectStructs/JSSystemStruct.hpp"
#include "../JSEntityCreateInfo.hpp"
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include "JSVec3.hpp"
#include "JSQuaternion.hpp"
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include "JSFields.hpp"

namespace Sirikata {
namespace JS {
namespace JSGlobal {

v8::Handle<v8::Value>checkResources(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in checkResources.  Requires no arguments.")));

    if (!args.This()->IsObject())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in checkResources.  This isn't an object.")));

    v8::Handle<v8::Object> globObjB = args.This()->ToObject();
    v8::Handle<v8::Object> globObj  = v8::Local<v8::Object>::Cast(globObjB->GetPrototype());
    
    //now check internal field count
    if (globObj->InternalFieldCount() != CONTEXT_GLOBAL_TEMPLATE_FIELD_COUNT)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in checkResources.  Object given does not have adequate number of internal fields for decode.")));

    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSObjScript;
    wrapJSObjScript = v8::Local<v8::External>::Cast(globObj->GetInternalField(CONTEXT_GLOBAL_JSOBJECT_SCRIPT_FIELD));
    
    void* ptr = wrapJSObjScript->Value();

    JSObjectScript* jsobj = NULL;
    jsobj = static_cast<JSObjectScript*>(ptr);
    if (jsobj == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in checkResources.  Cannot decode jsobjectscript")));
    
    return jsobj->checkResources();
}


}//end jsglobal namespace
}//end js namespace
}//end sirikata
