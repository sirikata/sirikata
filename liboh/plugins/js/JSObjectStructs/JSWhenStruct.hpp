
#ifndef __SIRIKATA_JS_WHEN_STRUCT_HPP__
#define __SIRIKATA_JS_WHEN_STRUCT_HPP__

#include "../JSUtil.hpp"
#include "JSContextStruct.hpp"
#include <v8.h>
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <map>
//#include "../JSObjectStructs/JSWatchable.hpp"

namespace Sirikata {
namespace JS {

class JSObjectScript;
class JSWatchable;

struct JSWhenStruct{
    
    const static float WHEN_MIN_PERIOD = .01;
    const static float WHEN_PERIOD_NOT_SET    =  -1;

    JSWhenStruct(JSObjectScript* jsscript,Sirikata::Network::IOService* ioserve,std::map<JSWatchable*,int>predWatches,v8::Persistent<v8::Function> preder, v8::Persistent<v8::Function> callback,v8::Persistent<v8::Context> cont,float whenPeriod);
    
    
    ~JSWhenStruct();
    
    static JSWhenStruct* decodeWhenStruct(v8::Handle<v8::Value> toDecode,String& errorMessage);

    
    v8::Handle<v8::Value>struct_whenResume();
    //v8::Handle<v8::Value>struct_whenSetState(bool boolArg);
    v8::Handle<v8::Value>struct_whenGetLastPredState();
    v8::Handle<v8::Value>struct_setPeriod(double newPeriod);
    v8::Handle<v8::Value>struct_whenGetPeriod();
    v8::Handle<v8::Value>struct_whenClear();
    v8::Handle<v8::Value>struct_whenSuspend();
    v8::Handle<v8::Value>struct_isSuspended();

    void removeWatchablesFromScript();
    void addWatchablesToScript();
    void runCallback();
    bool evalPred();
    bool checkPredAndRun();
    void deadlineExpired();
    void setPredTimer();

    
    
    bool stateSuspended;   //if stateSuspended is true, we don't even evaluate
                           //the predicate, let alone fire the callback
    
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
    


};

typedef std::map<JSWhenStruct*,bool> WhenMap;
typedef WhenMap::iterator WhenMapIter;


}  //end js namespace
}  //end sirikata namespace

#endif
