


#include <v8.h>
#include "JSUtilStruct.hpp"
#include "JSContextStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "../JSObjectScript.hpp"


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



String JSUtilStruct::whenCreateQuoted(v8::Handle<v8::Array>predArray)
{
    lkjs;
    String whenPredAsString  = "";
    std::vec<String> dependentParts;
    
    for (int s=0; s < predArray->Length(); ++s)
    {
        JSQuoteStruct* fromPredArray = JSQuoteStruct::decodeQuote(predArray->Get(s));
        if (JSQuoteStruct == NULL)
        {
            //create a new value in the context that is equal to this object
            String newName = createNewValueInContext(predArray->Get(s));
            
            whenPredAsString += newName;
        }
        else
        {
            dependentParts.push_back(parseParts(fromPredArray->getQuote()));
            whenPredAsString += fromPredArray->getQuote();
        }
    }
    lkjs;
}

//whenPred contains the followingTypes of objects:
//quoted/re-evaluate text and dollar expressions.

//should receive
//first arg: whenPred as an array
//second arg: callback as function
v8::Handle<v8::Value> JSUtilStruct::struct_createWhen(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback)
{
    
    JSWhenStruct* jswhen = new JSWhenStruct(predArray,callback);
    

    

    
    check is dollar object;
    lkjs;
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
    wrapJSUtilStructObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(UTIL_TEMAPLATE_FIELD));
    void* ptr = wrapJSUtilStructObj->Value();


    JSUtilStruct* returner;
    returner = static_cast<JSUtilStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of JSUtilStruct.  Internal field of object given cannot be casted to a JSUtilStruct.";

    return returner;
}



} //end namespace JS
} //end namespace Sirikata
