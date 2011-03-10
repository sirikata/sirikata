
#ifndef __SIRIKATA_JS_WHEN_WATCHED_LIST_STRUCT_HPP__
#define __SIRIKATA_JS_WHEN_WATCHED_LIST_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>



namespace Sirikata {
namespace JS {

class JSObjectScript;

struct JSWhenWatchedListStruct 
{
    JSWhenWatchedListStruct(const JSWhenWatchedVec& wwv,JSObjectScript* jsobj);
    ~JSWhenWatchedListStruct();
    
    static JSWhenWatchedListStruct* decodeWhenWatchedListStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);

    bool checkTrigger(String nameChanged);

    void debugPrintWatchedList();
    
private:
    JSObjectScript* mJSObj;

    //For js object system.x.y.z
    //name stored in mItemsToWatch as
    // [system, system.x, system.x.y, system.x.y.z]
    JSWhenWatchedVec mItemsToWatch;
};


}  //end js namespace
}  //end sirikata namespace

#endif
