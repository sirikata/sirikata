
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
