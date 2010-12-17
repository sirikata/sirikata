#include <v8.h>

#include "JSObjectsUtils.hpp"
#include "JSFields.hpp"
#include <cassert>
//#include "../JSPresenceStruct.hpp"


namespace Sirikata{
namespace JS{


const char* ToCString(const v8::String::Utf8Value& value)
{
  return *value ? *value : "<string conversion failed>";
}


}
}

