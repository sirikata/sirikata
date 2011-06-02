
#include "../JSUtil.hpp"
#include "../EmersonScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "../JSLogging.hpp"
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/network/IOService.hpp>
#include "JSSuspendable.hpp"

namespace Sirikata {
namespace JS {



JSTimerStruct::JSTimerStruct(EmersonScript*eobj,Duration dur,v8::Persistent<v8::Function>& callback,JSContextStruct* jscont,Sirikata::Network::IOService* ioserve,uint32 contID, double timeRemaining, bool isSuspended,bool isCleared)
 :JSSuspendable(),
  emerScript(eobj),
  cb(callback),
  jsContStruct(jscont),
  mDeadlineTimer(NULL),
  timeUntil(dur.toSeconds()),
  mTimeRemaining(timeRemaining)
{
    if (isCleared)
    {
        clear();
        return;
    }

    mDeadlineTimer =new Sirikata::Network::DeadlineTimer(*ioserve);
    if (isSuspended)
        suspend();

    
    if (contID != jscont->getContextID())
    {
        eobj->registerFixupSuspendable(this,contID);
    }
    else
    {
        if (timeRemaining == 0)
            mDeadlineTimer->expires_from_now(boost::posix_time::microseconds(timeUntil*1000000));
        else
            mDeadlineTimer->expires_from_now(boost::posix_time::microseconds(timeRemaining*1000000));
    
        mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this,_1));


        if (jscont != NULL)
            jscont->struct_registerSuspendable(this);
    }
}

void JSTimerStruct::fixSuspendableToContext(JSContextStruct* toAttachTo)
{
    jsContStruct = toAttachTo;

    if (mTimeRemaining == 0)
        mDeadlineTimer->expires_from_now(boost::posix_time::microseconds(timeUntil*1000000));
    else
        mDeadlineTimer->expires_from_now(boost::posix_time::microseconds(mTimeRemaining*1000000));
    
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this,_1));
    
    jsContStruct->struct_registerSuspendable(this);
}


JSTimerStruct* JSTimerStruct::decodeTimerStruct(v8::Handle<v8::Value> toDecode,String& errorMessage)
{
    v8::HandleScope handle_scope;  //for garbage collection.

    if (! toDecode->IsObject())
    {
        errorMessage += "Error in decode of timer object in JSTimerStruct.cpp.  Should have received an object to decode.";
        return NULL;
    }

    v8::Handle<v8::Object> toDecodeObject = toDecode->ToObject();

    //now check internal field count
    if (toDecodeObject->InternalFieldCount() != TIMER_JSTIMER_TEMPLATE_FIELD_COUNT)
    {
        errorMessage += "Error in decode of timer object in JSTimerStruct.  Object given does not have adequate number of internal fields for decode.";
        return NULL;
    }

    //now actually try to decode each.
    //decode the jsTimerStruct field
    v8::Local<v8::External> wrapJSTimerStruct;
    wrapJSTimerStruct = v8::Local<v8::External>::Cast(toDecodeObject->GetInternalField(TIMER_JSTIMERSTRUCT_FIELD));
    void* ptr = wrapJSTimerStruct->Value();

    JSTimerStruct* returner;
    returner = static_cast<JSTimerStruct*>(ptr);
    if (returner == NULL)
        errorMessage += "Error in decode of timer object in JSTimerStruct.cpp.  Internal field of object given cannot be casted to a JSTimerStruct.";

    return returner;
}


JSTimerStruct::~JSTimerStruct()
{
    if (jsContStruct != NULL)
        jsContStruct->struct_deregisterSuspendable(this);

    delete mDeadlineTimer;
}

//returning all data necessary to re-generate timer
//  uint32   contextId
//  double period
//  double timeUntil timer expires
//  bool  isSuspended
//  bool  isCleared
//  func  cb
v8::Handle<v8::Value> JSTimerStruct::struct_getAllData()
{
    v8::HandleScope handle_scope;
    
    uint32  contId   = jsContStruct->getContextID();
    bool    issusp   = getIsSuspended();
    bool    isclear  = getIsCleared();
    double  period   = -1;
    double  tUntil   = -1;

    v8::Handle<v8::Function> cbFunc;
    
    if (! isclear)
    {
        cbFunc = cb;
        period = timeUntil;
        if (issusp)
            tUntil = timeUntil;
        else
        {
            boost::posix_time::time_duration pt = mDeadlineTimer->expires_from_now();
            tUntil = (double)(pt.total_microseconds()/1000000.);
        }
    }

    v8::Handle<v8::Object> returner = v8::Object::New();

    if (isclear)
    {
        returner->Set(v8::String::New("isCleared"),v8::Boolean::New(isclear));
        returner->Set(v8::String::New("contextId"), v8::Integer::NewFromUnsigned(contId));
        return handle_scope.Close(returner);
    }
    
    returner->Set(v8::String::New("period"), v8::Number::New(period));    
    returner->Set(v8::String::New("callback"),cbFunc);
    returner->Set(v8::String::New("timeRemaining"),v8::Number::New(tUntil));
    returner->Set(v8::String::New("isSuspended"),v8::Boolean::New(issusp));

    
    return handle_scope.Close(returner);
}



void JSTimerStruct::evaluateCallback(const boost::system::error_code& error)
{
    if (! error )
        emerScript->handleTimeoutContext(cb,jsContStruct);

}


v8::Handle<v8::Value> JSTimerStruct::suspend()
{
    if (getIsCleared())
    {
        JSLOG(info, "Error in suspend of JSTimerStruct.cpp.  Called suspend even though the timer had previously been cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Called suspend on a timer that had already been cleared.")));
    }

    JSLOG(insane,"suspending timer");
    mDeadlineTimer->cancel();
    return JSSuspendable::suspend();
}


//has more of a reset-type functionality than resume
//if the time has not been cleared, then, cancel the current timer,
//and start a new countdown to execute the callback.
v8::Handle<v8::Value> JSTimerStruct::resume()
{
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Trying to resume a timer object that has already been cleared.  Taking no action");
        return JSSuspendable::getIsSuspendedV8();
    }

    mDeadlineTimer->cancel();


    mDeadlineTimer->expires_from_now(boost::posix_time::microseconds(timeUntil*1000000));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this,_1));

    return JSSuspendable::resume();
}



v8::Handle<v8::Value> JSTimerStruct::clear()
{
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Calling clear on a timer that has already been cleared.");
        return JSSuspendable::clear();
    }

    if (mDeadlineTimer != NULL)
        mDeadlineTimer->cancel();

    if (jsContStruct != NULL)
        jsContStruct->struct_deregisterSuspendable(this);

    if (! cb.IsEmpty())
        cb.Dispose();
    return JSSuspendable::clear();
}


v8::Handle<v8::Value> JSTimerStruct::struct_resetTimer(double timeInSecondsToRefire)
{
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Calling reset on a timer that has already been cleared.");
        return JSSuspendable::clear();
    }

    mDeadlineTimer->cancel();
    mDeadlineTimer->expires_from_now(boost::posix_time::microseconds(timeInSecondsToRefire*1000000));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this,_1));

    return JSSuspendable::resume();
}



} //end namespace js
} //end namespace sirikata
