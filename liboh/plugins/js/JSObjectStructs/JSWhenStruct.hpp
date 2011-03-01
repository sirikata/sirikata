
#ifndef __SIRIKATA_JS_WHEN_STRUCT_HPP__
#define __SIRIKATA_JS_WHEN_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <map>
#include "JSSuspendable.hpp"
#include "JSWhenWatchedItemStruct.hpp"
#include "JSWhenWatchedListStruct.hpp"


namespace Sirikata {
namespace JS {

class JSObjectScript;


struct JSWhenStruct : public JSSuspendable
{
    JSWhenStruct(v8::Handle<v8::Array>predArray, v8::Handle<v8::Function> callback,JSObjectScript* jsobj, JSContextStruct* jscontextstr);
    ~JSWhenStruct();
    
    
    static JSWhenStruct* decodeWhenStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);


    v8::Handle<v8::Value>struct_whenGetPeriod();
    v8::Handle<v8::Value>struct_setPeriod(double newPeriod);
    
    virtual v8::Handle<v8::Value>clear();
    virtual v8::Handle<v8::Value>suspend();
    virtual v8::Handle<v8::Value>resume();    

    
    bool checkPredAndRun();

    void buildWatchedItems(const String& whenPredAsString);
    v8::Handle<v8::Value>struct_whenGetLastPredState();


private:

    String createNewValueIncontext(v8::Handle<v8::Value> toRenameInContext);
    void whenCreatePredFunc(v8::Handle<v8::Array>predArray);
    void whenCreateCBFunc(v8::Handle<v8::Function>callback);
    bool evalPred();
    void runCallback();
    void addWhenToContext();
    void addWhenToScript();    
    
    bool predState;        //predState is true if the predicate was true in the
                           //previous iteration.  Do not fire a callback if the
                           //predicate is true.  Only fire callback if predicate
                           //switches from false to true.


    JSObjectScript* mObjScript;
    v8::Persistent<v8::Function> mPred;
    v8::Persistent<v8::Function> mCB;
    v8::Persistent<v8::Context>  mContext;
    JSContextStruct* jscont;
    JSWhenWatchedListStruct* mWWLS;


};


typedef std::map<JSWhenStruct*,bool> WhenMap;
typedef WhenMap::iterator WhenMapIter;


}  //end js namespace
}  //end sirikata namespace

#endif
