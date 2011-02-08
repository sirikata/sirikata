
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>
#include "../JSLogging.hpp"
#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>

namespace Sirikata {
namespace JS {


JSTimerStruct::JSTimerStruct(JSObjectScript* jsobj, const Duration& dur,v8::Persistent<v8::Object>& targ, v8::Persistent<v8::Function>& callback,JSContextStruct* jscont, Sirikata::Network::IOService* ioserve)
 : JSSuspendable(),
   jsObjScript(jsobj),
   target(targ),
   cb(callback),
   jsContStruct(jscont),
   mDeadlineTimer (new Sirikata::Network::DeadlineTimer(*ioserve)),
   timeUntil(dur.toSeconds())
{
    mDeadlineTimer->expires_from_now(boost::posix_time::seconds(timeUntil));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this));

    if (jscont != NULL)
        jscont->struct_registerSuspendable(this);
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


void JSTimerStruct::evaluateCallback()
{
    if (getIsCleared())
    {
        JSLOG(info, "Error in evaluateCallback of JSTimerStruct.cpp.  Got a callback even though the timer should have been cleared.");
        return;
    }
        
    jsObjScript->handleTimeoutContext(target,cb,jsContStruct);
    if (jsContStruct != NULL)
        jsContStruct->struct_deregisterSuspendable(this);
}


v8::Handle<v8::Value> JSTimerStruct::suspend()
{
    if (getIsCleared())
    {
        JSLOG(info, "Error in suspend of JSTimerStruct.cpp.  Called suspend even though the timer had previously been cleared.");
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  Called suspend on a timer that had already been cleared.")));
    }
    
    mDeadlineTimer->cancel();
    return JSSuspendable::suspend();
}

v8::Handle<v8::Value> JSTimerStruct::resume()
{
    if (! getIsSuspended())
    {
        JSLOG(info,"Error in JSTimerStruct.  Trying to resume a timer object that was not already suspended.");
        return JSSuspendable::getIsSuspendedV8();
    }
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Trying to resume a timer object that has already been cleared.  Taking no action");
        return JSSuspendable::getIsSuspendedV8();
    }
    
    
    mDeadlineTimer->expires_from_now(boost::posix_time::seconds(timeUntil));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this));
    
    return JSSuspendable::resume();
}



v8::Handle<v8::Value> JSTimerStruct::clear()
{
    if (getIsCleared())
    {
        JSLOG(info,"Error in JSTimerStruct.  Calling clear on a timer that has already been cleared.");
        return JSSuspendable::clear();
    }
    
    mDeadlineTimer->cancel();

    if (jsContStruct != NULL)
        jsContStruct->struct_deregisterSuspendable(this);

    target.Dispose();
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
    mDeadlineTimer->expires_from_now(boost::posix_time::seconds(timeInSecondsToRefire));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this));
    
    return JSSuspendable::resume();
}



} //end namespace js
} //end namespace sirikata
