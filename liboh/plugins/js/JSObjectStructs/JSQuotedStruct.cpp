


#include <v8.h>
#include "JSQuotedStruct.hpp"
#include "../JSObjects/JSFields.hpp"



namespace Sirikata{
namespace JS{


JSQuotedStruct::JSQuotedStruct(const String& toQuote)
 : mString(toQuote)
{
}


JSQuotedStruct::~JSQuotedStruct()
{
}



String JSQuotedStruct::getQuote()
{
    return mString;
}

//decodes quoted struct object
JSQuotedStruct* JSQuotedStruct::decodeQuotedStruct(v8::Handle<v8::Value> toDecode ,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    
    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of JSQuotedStruct.  Should have received an object to decode.";
        return NULL;
    }
        
    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();
        
    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != QUOTED_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of JSQuotedStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }
        
    //now actually try to decode each.
    //decode the jsVisibleStruct field
    v8::Local<v8::External> wrapJSQuotedStructObj;
    wrapJSQuotedStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(QUOTED_QUOTESTRUCT_FIELD));
    void* ptr = wrapJSQuotedStructObj->Value();


    JSQuotedStruct* returner;
    returner = static_cast<JSQuotedStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSQuotedStruct.  Internal field of object given cannot be casted to a JSQuotedStruct.";

    return returner;
}



} //end namespace JS
} //end namespace Sirikata
