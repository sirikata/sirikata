
#include "../JSUtil.hpp"
#include "../JSObjectScript.hpp"
#include "JSTimerStruct.hpp"
#include "../JSObjects/JSFields.hpp"
#include <v8.h>

#include <sirikata/core/network/IOTimer.hpp>
#include <sirikata/core/network/IOService.hpp>

namespace Sirikata {
namespace JS {


JSTimerStruct::JSTimerStruct(JSObjectScript* jsobj, const Duration& dur,v8::Persistent<v8::Object>& targ, v8::Persistent<v8::Function>& callback,JSContextStruct* jscont, Sirikata::Network::IOService* ioserve)
 : jsObjScript(jsobj),
   target(targ),
   cb(callback),
   jsContStruct(jscont),
   mDeadlineTimer (new Sirikata::Network::DeadlineTimer(*ioserve))
{
    mDeadlineTimer->expires_from_now(boost::posix_time::seconds(dur.toSeconds()));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this));

    if (jscont != NULL)
        jscont->struct_registerTimeout(this);
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
        jsContStruct->struct_deregisterTimeout(this);
    
    delete mDeadlineTimer;
}


void JSTimerStruct::evaluateCallback()
{
    jsObjScript->handleTimeoutContext(target,cb,jsContStruct);
}



v8::Handle<v8::Value> JSTimerStruct::struct_clear()
{
    mDeadlineTimer->cancel();
    return v8::Undefined();
}

v8::Handle<v8::Value> JSTimerStruct::struct_resetTimer(double timeInSecondsToRefire)
{
    mDeadlineTimer->cancel();
    mDeadlineTimer->expires_from_now(boost::posix_time::seconds(timeInSecondsToRefire));
    mDeadlineTimer->async_wait(std::tr1::bind(&JSTimerStruct::evaluateCallback,this));
    
    return v8::Undefined();
}



} //end namespace js
} //end namespace sirikata
