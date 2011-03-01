
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "JSWhenWatchedItemStruct.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"

#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>

namespace Sirikata {
namespace JS {


JSWhenWatchedItemStruct::JSWhenWatchedItemStruct(v8::Handle<v8::Array> itemArray,JSObjectScript* jsobj)
 : mJSObj(jsobj)
{
    for (int s = 0; s < (int) itemArray->Length(); ++s)
    {
        String decodedVal;
        String errorMessage = "Error decoding string associated with watched itemStruct.  ";
        bool strDecodeSuccessful =  decodeString( itemArray->Get(s), decodedVal,errorMessage);
        if (! strDecodeSuccessful)
        {
            JSLOG(error,errorMessage);
            break;
        }
        
        if (s==0)
        {
            mItemsToWatch.push_back(decodedVal);
            continue;
        }

        mItemsToWatch.push_back( mItemsToWatch[s-1] + "." + decodedVal);
    }
}

void JSWhenWatchedItemStruct::debugPrint()
{
    JSLOG(debug, "****inside of individual when watched item******");
    for (std::vector<String>::iterator iter = mItemsToWatch.begin(); iter != mItemsToWatch.end(); ++iter)
        JSLOG(debug,*iter);
}


//function returns true if the item in nameChanged should cause the predicate to
//re-fire
//let's say that this item corresponds to system.x.y.z, then the mItemsToWatch
//vector contains [system, system.x, system.x.y, and system.x.y.z]
// if have an assignment to system.x, will receive "system.x" in name changed,
// and return true.
bool JSWhenWatchedItemStruct::checkTrigger(String nameChanged)
{
    for (std::vector<String>::iterator iter = mItemsToWatch.begin(); iter != mItemsToWatch.end(); ++iter)
    {
        if (*iter == nameChanged)
            return true;
    }
    return false;
}


JSWhenWatchedItemStruct::~JSWhenWatchedItemStruct()
{
}



JSWhenWatchedItemStruct* JSWhenWatchedItemStruct::decodeWhenWatchedItemStruct(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.
    
    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of watched object.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    
    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != WHEN_WATCHED_ITEM_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of WhenWatchedItem object.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsWhenWatchedItem field
    v8::Local<v8::External> wrapJSWhenWatchedItemObj;
    wrapJSWhenWatchedItemObj = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(WHEN_WATCHED_ITEM_TEMPLATE_FIELD));
    void* ptr = wrapJSWhenWatchedItemObj->Value();

    JSWhenWatchedItemStruct* returner;
    returner = static_cast<JSWhenWatchedItemStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of WhenWatchedItem object.  Internal field of object given cannot be casted to a JSWhenWatchedItemStruct.";

    return returner;
}


} //end namespace js
} //end namespace sirikata
