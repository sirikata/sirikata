
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


//should just return an object that is a whenwatcheditem.  The internal field of
//the when watched item should contain a vector of all the individual tokens
//that we have to watch out for.
v8::Handle<v8::Value> JSUtilStruct::struct_createWhenWatchedList(v8::Handle<v8::Array>arrayOfItems)
{
    std::vector<JSWhenWatchedItemStruct*> wwisVec;

    //decode all v8 objects in array as when watched item structs
    //put them in a vector to pass in to associated object script to create new
    //whenWatchedList
    for (int s=0; s < (int) arrayOfItems->Length(); ++s)
    {
        String errorMessage = "Error decoding item from array in struct_createWhenWatchedList.  Should be a whenWatchedItem, but is not.  ";
        JSWhenWatchedItemStruct* singleItem = JSWhenWatchedItemStruct::decodeWhenWatchedItemStruct(arrayOfItems->Get(s), errorMessage);
        if (singleItem == NULL)
            return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));

        wwisVec.push_back(singleItem);
    }

    return associatedObjScr->createWhenWatchedList(wwisVec);
}



v8::Handle<v8::Value> JSUtilStruct::struct_createWhenWatchedItem(v8::Handle<v8::Array>itemArray)
{
    return associatedObjScr->createWhenWatchedItem(itemArray);
}

v8::Handle<v8::Value> JSUtilStruct::struct_createWatched()
{
    return associatedObjScr->createWatched();
}

v8::Handle<v8::Value> JSUtilStruct::struct_createQuotedObject(const String& toQuote)
{
    return associatedObjScr->createQuoted(toQuote);
}


//whenPred contains the followingTypes of objects:
//quoted/re-evaluate text and dollar expressions.

//should receive
//first arg: whenPred as an array
//second arg: callback as function
v8::Handle<v8::Value> JSUtilStruct::struct_createWhen(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback)
{
    return associatedObjScr->createWhen(predArray,callback,associatedContext);
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
