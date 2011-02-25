
#ifndef __SIRIKATA_JS_WHEN_WATCHED_ITEM_STRUCT_HPP__
#define __SIRIKATA_JS_WHEN_WATCHED_ITEM_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>



namespace Sirikata {
namespace JS {

struct JSWhenWatchedItemStruct 
{
    JSWhenWatchedItemStruct(v8::Persistent<v8::Array> itemArray, JSObjectScript* jsobj);
    JSWhenWatchedItemStruct(JSObjectScript* jsobj);
    ~JSWhenWatchedItemStruct();
    
    static JSWhenWatchedItemStruct* decodeWhenWatchedItemStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);

    bool changeTrigger(String nameChanged);
    
private:
    JSObjectScript* mJSObj;

    //For js object system.x.y.z
    //name stored in mItemsToWatch as
    // [system, system.x, system.x.y, system.x.y.z]

    std::vector<String> mItemsToWatch;
};

typedef std::vector<JSWhenWatchedItemStruct*> JSWhenWatchedVec;

}  //end js namespace
}  //end sirikata namespace

#endif
