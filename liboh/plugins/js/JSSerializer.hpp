#ifndef __SIRIKATA_JS_SERIALIZE_HPP__
#define __SIRIKATA_JS_SERIALIZE_HPP__

#include <string>
#include "JS_JSMessage.pbj.hpp"

//#include <sirikata/core/util/RoutableMessageHeader.hpp>
//#include <sirikata/core/util/RoutableMessageBody.hpp>
//#include <sirikata/core/util/RoutableMessageBody.hpp>
#include <v8.h>

namespace Sirikata {
namespace JS {

/*
  FIXME: Need to Re-name these files to JSSerializer
 */

class JSSerializer
{
public:
    static std::string serializeObject(v8::Local<v8::Object> v8Obj);;
    static bool deserializeObject( std::string strDecode,v8::Local<v8::Object>& deserializeTo);
    static bool deserializeObject( MemoryReference payload,v8::Local<v8::Object>& deserializeTo);
};

}}//end namespaces

#endif
