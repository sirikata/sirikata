#include "JSSuspendable.hpp"
#include <v8.h>
#include "JSPresenceStruct.hpp"

namespace Sirikata{
namespace JS{


JSSuspendable::JSSuspendable()
 : isSuspended(false),
   isCleared(false)
{
}

JSSuspendable::~JSSuspendable()
{
}

v8::Handle<v8::Value> JSSuspendable::suspend()
{
    isSuspended = true;
    return getIsSuspendedV8();
}

v8::Handle<v8::Value> JSSuspendable::resume()
{
    isSuspended = false;
    return getIsSuspendedV8();
}
bool JSSuspendable::getIsSuspended()
{
    return isSuspended;
}

v8::Handle<v8::Boolean> JSSuspendable::getIsSuspendedV8()
{
    return v8::Boolean::New(isSuspended);
}

v8::Handle<v8::Boolean> JSSuspendable::getIsClearedV8()
{
    return v8::Boolean::New(isCleared);
}

bool JSSuspendable::getIsCleared()
{
    return isCleared;
}

v8::Handle<v8::Value> JSSuspendable::clear()
{
    isCleared = true;
    return getIsClearedV8();
}


} //end namespace js
} //end namespace sirikata
