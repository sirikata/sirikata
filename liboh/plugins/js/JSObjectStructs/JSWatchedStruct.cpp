
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "JSWatchedStruct.hpp"
#include "JSWatchable.hpp"

#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>

namespace Sirikata {
namespace JS {

JSWatchedStruct::JSWatchedStruct(v8::Persistent<v8::Object> toWatch,JSObjectScript* jsobj)
 : JSWatchable(),
   mWatchedObject(toWatch),
   mJSObj(jsobj)
{
}

JSWatchedStruct::~JSWatchedStruct()
{
}


JSWatchedStruct* JSWatchedStruct::decodeWatchedStruct(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    
    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of watched object.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count

    if (toDecodeObject->InternalFieldCount() != WATCHED_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of watched object.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsWatched field
    v8::Local<v8::External> wrapJSWatchedObj;
    wrapJSWatchedObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(WATCHED_TEMPLATE_FIELD));
    void* ptr = wrapJSWatchedObj->Value();

    JSWatchedStruct* returner;
    returner = static_cast<JSWatchedStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of watched object.  Internal field of object given cannot be casted to a JSWatchedStruct.";


    return returner;
}




} //end namespace js
} //end namespace sirikata
