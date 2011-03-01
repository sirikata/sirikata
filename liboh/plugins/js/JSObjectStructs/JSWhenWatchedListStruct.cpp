
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "JSWhenWatchedItemStruct.hpp"
#include "JSWhenWatchedListStruct.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"

#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>

namespace Sirikata {
namespace JS {


JSWhenWatchedListStruct::JSWhenWatchedListStruct(const JSWhenWatchedVec& wwv,JSObjectScript* jsobj)
 : mJSObj(jsobj),
   mItemsToWatch(wwv)
{
}

void JSWhenWatchedListStruct::debugPrintWatchedList()
{
    JSLOG(debug,"printing all elements of when watched list.");
    for (JSWhenWatchedIter iter = mItemsToWatch.begin(); iter!= mItemsToWatch.end(); ++iter)
        (*iter)->debugPrint();

}


//runs through all the watched items to check if the change to name should
//actually cause the predicate to re-trigger.
bool JSWhenWatchedListStruct::checkTrigger(String nameChanged)
{
    for(JSWhenWatchedIter iter = mItemsToWatch.begin(); iter != mItemsToWatch.end(); ++iter)
    {
        if ((*iter)->checkTrigger(nameChanged))
            return true;
    }

    return false;
}


JSWhenWatchedListStruct::~JSWhenWatchedListStruct()
{
}



JSWhenWatchedListStruct* JSWhenWatchedListStruct::decodeWhenWatchedListStruct(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    
    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of watched object.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    
    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != WHEN_WATCHED_LIST_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of WhenWatchedList object.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsWhenWatchedList field
    v8::Local<v8::External> wrapJSWhenWatchedListObj;
    wrapJSWhenWatchedListObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(WHEN_WATCHED_LIST_TEMPLATE_FIELD));
    void* ptr = wrapJSWhenWatchedListObj->Value();

    JSWhenWatchedListStruct* returner;
    returner = static_cast<JSWhenWatchedListStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of WhenWatchedList object.  Internal field of object given cannot be casted to a JSWhenWatchedListStruct.";

    return returner;
}


} //end namespace js
} //end namespace sirikata
