#include "JSObjectsUtils.hpp"
#include <v8.h>

namespace Sirikata{
namespace JS{

const char* ToCString(const v8::String::Utf8Value& value)
{
  return *value ? *value : "<string conversion failed>";
}


}}//end namespaces
