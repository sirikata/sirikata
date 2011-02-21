
#ifndef __SIRIKATA_JS_WHEN_STRUCT_HPP__
#define __SIRIKATA_JS_WHEN_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <map>
#include "JSSuspendable.hpp"


namespace Sirikata {
namespace JS {

class JSObjectScript;
class JSWatchable;

struct JSWhenStruct : public JSSuspendable
{
    
    const static float WHEN_MIN_PERIOD = .01;
    const static float WHEN_PERIOD_NOT_SET    =  -1;

//    JSWhenStruct(JSObjectScript* jsscript,Sirikata::Network::IOService* ioserve,std::map<JSWatchable*,int>predWatches,v8::Persistent<v8::Function> preder, v8::Persistent<v8::Function> callback,v8::Persistent<v8::Context> cont,float whenPeriod, JSContextStruct* jscontextstr);


    JSWhenStruct(v8::Handle<v8::Array>predArray, v8::Local<v8::Function> callback,JSObjectScript* jsobj);
    ~JSWhenStruct();
    
    
    static JSWhenStruct* decodeWhenStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);


    v8::Handle<v8::Value>struct_whenGetPeriod();
    v8::Handle<v8::Value>struct_setPeriod(double newPeriod);
    
    virtual v8::Handle<v8::Value>clear();
    virtual v8::Handle<v8::Value>suspend();
    virtual v8::Handle<v8::Value>resume();    

    bool checkPredAndRun();
    v8::Handle<v8::Value>struct_whenGetLastPredState();


private:


    void deadlineExpired();
    void removeWatchablesFromScript();
    void addWatchablesToScript();
    void addWhenToWatchables();
    void setPredTimer();
    bool evalPred();
    void runCallback();
    void addWhenToContext();    
    
    bool predState;        //predState is true if the predicate was true in the
                           //previous iteration.  Do not fire a callback if the
                           //predicate is true.  Only fire callback if predicate
                           //switches from false to true.
    double currentPeriod;

  
    JSObjectScript* mObjScript;
    Sirikata::Network::DeadlineTimer* mDeadlineTimer;
    std::map<JSWatchable*,int> mWatchables;
    v8::Persistent<v8::Function> mPred;
    v8::Persistent<v8::Function> mCB;
    v8::Persistent<v8::Context> mContext;
    JSContextStruct* jscont;
};


typedef std::map<JSWhenStruct*,bool> WhenMap;
typedef WhenMap::iterator WhenMapIter;


}  //end js namespace
}  //end sirikata namespace

#endif
