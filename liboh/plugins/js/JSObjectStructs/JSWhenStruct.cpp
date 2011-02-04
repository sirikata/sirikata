#include "JSWhenStruct.hpp"
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include "JSWatchable.hpp"
#include <v8.h>

#include "../JSLogging.hpp"
#include "../JSObjects/JSObjectsUtils.hpp"

#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>


namespace Sirikata {
namespace JS {

JSWhenStruct::JSWhenStruct(JSObjectScript* jsscript,Sirikata::Network::IOService* ioserve,std::map<JSWatchable*,int>predWatches,v8::Persistent<v8::Function> preder, v8::Persistent<v8::Function> callback,v8::Persistent<v8::Context> cont,float whenPeriod)
 : mObjScript(jsscript),
   mDeadlineTimer (new Sirikata::Network::DeadlineTimer(*ioserve)),
   mPred(preder),
   mCB(callback),
   mContext(cont),
   stateSuspended(false),
   predState(false),
   currentPeriod(whenPeriod)
{
    setPredTimer();
    addWatchablesToScript();
}


void JSWhenStruct::setPredTimer()
{
    if (stateSuspended)
        return;
    
    if (currentPeriod !=  WHEN_PERIOD_NOT_SET)
    {
        if (currentPeriod < WHEN_MIN_PERIOD)
        {
            JSLOG(error,"Error in setPredTimer of JSWhenStruct.cpp: trying to set when period below its min period.  Setting predicate checking period to the min checking period.");
            currentPeriod = WHEN_MIN_PERIOD;
        }

        mDeadlineTimer->expires_from_now(boost::posix_time::seconds(currentPeriod));
        mDeadlineTimer->async_wait(std::tr1::bind(&JSWhenStruct::deadlineExpired,this));
    }
}


void JSWhenStruct::deadlineExpired()
{
    if (stateSuspended)
    {
        JSLOG(error,"Error in deadlineExpired of JSWhenStruct.  Should have been in a suspended state, but received a callback from timer constructor of JSWhenStruct.cpp: trying to set when statement below its min period.  Setting predicate checking period to the min checking period.");
        return;
    }

    checkPredAndRun();
    setPredTimer();
}


bool JSWhenStruct::checkPredAndRun()
{
    if (stateSuspended)
        return false;

    //updateVisibles();
    bool prevPredState=predState;

    bool predState = evalPred();
    if (predState && !prevPredState)
    {
        runCallback();
        return true;
    }
    return false;
}



//this function evaluates the predicate within the context
bool JSWhenStruct::evalPred()
{
    v8::HandleScope handle_scope;
    
    //the function passed in shouldn't take any arguments
    v8::Handle<v8::Value> predReturner = mObjScript->executeInContext(mContext,mPred,0,NULL);

    String dummyErrorMessage = "";
    bool decodedVal;
    bool returnedBool = decodeBool(predReturner,decodedVal,dummyErrorMessage);
    if (! returnedBool)
    {
        JSLOG(error,"Error in evalPred of JSWhenStruct.cpp.  Predicate did not return bool.  Suspending when statement");
        struct_whenSuspend();
        return false;
    }

    return returnedBool;
}


void JSWhenStruct::runCallback()
{
    v8::HandleScope handle_scope;
    
    //the function passed in shouldn't take any arguments
    mObjScript->executeInContext(mContext,mCB,0,NULL);
}



v8::Handle<v8::Value>JSWhenStruct::struct_whenGetLastPredState()
{
    v8::HandleScope handle_scope;
    return v8::Boolean::New(predState);
}


v8::Handle<v8::Value>JSWhenStruct::struct_setPeriod(double newPeriod)
{
    currentPeriod = newPeriod;
    //cancel current timer, and reset it
    mDeadlineTimer->cancel();
    setPredTimer();
    return v8::Undefined();
}

v8::Handle<v8::Value>JSWhenStruct::struct_whenGetPeriod()
{
    v8::HandleScope handle_scope;
    return v8::Number::New(currentPeriod);
}

v8::Handle<v8::Value>JSWhenStruct::struct_isSuspended()
{
    v8::HandleScope handle_scope;
    return v8::Boolean::New(stateSuspended);
}

void JSWhenStruct::removeWatchablesFromScript()
{
    for (WatchableIter iter = mWatchables.begin(); iter != mWatchables.end(); ++iter)
        mObjScript->removeWatchable(iter->first);
}

//call on resume and in constructor;
void JSWhenStruct::addWatchablesToScript()
{
    for (WatchableIter iter = mWatchables.begin(); iter != mWatchables.end(); ++iter)
        mObjScript->addWatchable(iter->first);
}

v8::Handle<v8::Value>JSWhenStruct::struct_whenSuspend()
{
    if (! stateSuspended)
        removeWatchablesFromScript(); //remove watchables from script (should
                                      //add them again when when is resumed).
        
    stateSuspended = true;
    mDeadlineTimer->cancel();
    
    return v8::Undefined();
}


v8::Handle<v8::Value>JSWhenStruct::struct_whenResume()
{
    if (currentPeriod == WHEN_PERIOD_NOT_SET)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Please set the when period before continuing (via setPeriod)")));

    if (stateSuspended)
        addWatchablesToScript();  //add watchables to script if we were
                                  //previously in the state suspended state
        
    stateSuspended = false;
    setPredTimer();

    
    return v8::Undefined();
}




} //end namespace js
} //end namespace sirikata
