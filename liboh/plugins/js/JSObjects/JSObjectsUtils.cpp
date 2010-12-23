#include <v8.h>

#include "JSObjectsUtils.hpp"
#include "JSFields.hpp"
#include <cassert>
//#include "../JSPresenceStruct.hpp"


namespace Sirikata{
namespace JS{


const char* ToCString(const v8::String::Utf8Value& value)
{
  return *value ? *value : "<string conversion failed>";
}


JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args)
{
    v8::HandleScope handle_scope;
    v8::Local<v8::Object> v8Object = args.This();
    v8::Local<v8::External> wrapJSPresStructObj;
    if (v8Object->InternalFieldCount() > 0)
    {
        wrapJSPresStructObj = v8::Local<v8::External>::Cast(
            v8Object->GetInternalField(PRESENCE_FIELD_PRESENCE));
    }
    else
    {
        wrapJSPresStructObj = v8::Local<v8::External>::Cast(
            v8::Handle<v8::Object>::Cast(v8Object->GetPrototype())->GetInternalField(PRESENCE_FIELD_PRESENCE));
    }
    void* ptr = wrapJSPresStructObj->Value();
    JSPresenceStruct* jspres_struct = static_cast<JSPresenceStruct*>(ptr);
    
    if (jspres_struct == NULL)
        assert(false);
        
        return jspres_struct;
    return NULL;
}



}
}

